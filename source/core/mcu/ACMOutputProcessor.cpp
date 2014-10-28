/*
 * Copyright (c) 2012, The WebRTC project authors. All rights reserved.
 * Copyright (c) 2014, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:

 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of Google nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ACMOutputProcessor.h"

#include "TaskRunner.h"
#include <webrtc/system_wrappers/interface/trace.h>
#include <webrtc/voice_engine/include/voe_errors.h>

using namespace erizo;
using namespace webrtc;

namespace mcu {

DEFINE_LOGGER(ACMOutputProcessor, "media.ACMOutputProcessor");

ACMOutputProcessor::ACMOutputProcessor(uint32_t instanceId, woogeen_base::WoogeenTransport<erizo::AUDIO>* transport):
        amixer_(AudioConferenceMixer::Create(instanceId)),
        apm_(NULL),
        instanceId_(instanceId),
        audio_coding_(AudioCodingModule::Create(instanceId)),
        m_isClosing(false),
        _outputFileRecorderPtr(nullptr),
        _outputFileRecording(false)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, instanceId,
                 "OutputMixer::OutputMixer() - ctor");

    if ((amixer_->RegisterMixedStreamCallback(*this) == -1) ||
        (amixer_->RegisterMixerStatusCallback(*this, 100) == -1))
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId,-1),
                     "OutputMixer::OutputMixer() failed to register mixer"
                     "callbacks");
    }

    m_audioTransport.reset(transport);
    RtpRtcp::Configuration configuration;
    configuration.outgoing_transport = transport;
    configuration.id = VoEModuleId(instanceId_, instanceId_);
    configuration.audio = true;
/*
    configuration.rtcp_feedback = this;
    configuration.receive_statistics = rtp_receive_statistics_.get();
*/
    _rtpRtcpModule.reset(RtpRtcp::CreateRtpRtcp(configuration));

}

ACMOutputProcessor::~ACMOutputProcessor() {
    WEBRTC_TRACE(kTraceDebug, kTraceVoice, instanceId_,
                 "OutputMixer::~OutputMixer() - dtor");

    amixer_->UnRegisterMixerStatusCallback();
    amixer_->UnRegisterMixedStreamCallback();

    // According to the boost document, if the timer has already expired when
    // cancel() is called, then the handlers for asynchronous wait operations
    // can no longer be cancelled, and therefore are passed an error code
    // that indicates the successful completion of the wait operation.
    // This means we cannot rely on the operation_aborted error code in the handlers
    // to know if the timer is being cancelled, thus an additional flag is provided.
    m_isClosing = true;
    if (timer_)
        timer_->cancel();
    if (audioMixingThread_)
        audioMixingThread_->join();

    this->StopRecordingPlayout();
    if (taskRunner_) {
        taskRunner_->DeRegisterModule(_rtpRtcpModule.get());
    }
    delete amixer_;
}

bool ACMOutputProcessor::init(boost::shared_ptr<TaskRunner> taskRunner) {

    WEBRTC_TRACE(kTraceDebug, kTraceVoice, instanceId_,
                 "ACMOutputProcessor::init");

    taskRunner_ = taskRunner;

    if ((audio_coding_->InitializeSender() == -1))
    {
           WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(instanceId_,instanceId_),
            "Channel::Init() unable to initialize the ACM - 1");
        return false;
    }

    audio_coding_->RegisterTransportCallback(this);

    CodecInst codec;
    const uint8_t nSupportedCodecs = AudioCodingModule::NumberOfCodecs();

    for (int idx = 0; idx < nSupportedCodecs; idx++)
    {
        // Open up the RTP/RTCP receiver for all supported codecs
        if ((audio_coding_->Codec(idx, &codec) == -1))
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(instanceId_,instanceId_),
                         "Channel::Init() unable to register %s (%d/%d/%d/%d) "
                         "to RTP/RTCP receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }
        else
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                         VoEId(instanceId_,instanceId_),
                         "Channel::Init() %s (%d/%d/%d/%d) has been added to "
                         "the RTP/RTCP receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }

        // Ensure that PCMU is used as default codec on the sending side
        if (!STR_CASE_CMP(codec.plname, "PCMU") && (codec.channels == 1))
        {
            SetSendCodec(codec);
        }

        if (!STR_CASE_CMP(codec.plname, "CN"))
        {
            if ((audio_coding_->RegisterSendCodec(codec) == -1) ||
                /*(audio_coding_->RegisterReceiveCodec(codec) == -1) ||*/
                (_rtpRtcpModule->RegisterSendPayload(codec) == -1))
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                             VoEId(instanceId_,instanceId_),
                             "Channel::Init() failed to register CN (%d/%d) "
                             "correctly - 1",
                             codec.pltype, codec.plfreq);
            }
        }
    }

    if (taskRunner_) {
        taskRunner_->RegisterModule(_rtpRtcpModule.get());
    }

    this->StartRecordingPlayout("mixer.audio", NULL);
    timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::milliseconds(10)));
    timer_->async_wait(boost::bind(&ACMOutputProcessor::MixActiveChannels, this, boost::asio::placeholders::error));

    audioMixingThread_.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)));
    return true;
}

int32_t
ACMOutputProcessor::SetSendCodec(const CodecInst& codec)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(instanceId_,instanceId_),
                 "Channel::SetSendCodec()");

    if (audio_coding_->RegisterSendCodec(codec) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId_,instanceId_),
                     "SetSendCodec() failed to register codec to ACM");
        return -1;
    }

    if (_rtpRtcpModule->RegisterSendPayload(codec) != 0)
    {
        _rtpRtcpModule->DeRegisterSendPayload(codec.pltype);
        if (_rtpRtcpModule->RegisterSendPayload(codec) != 0)
        {
            WEBRTC_TRACE(
                    kTraceError, kTraceVoice, VoEId(instanceId_,instanceId_),
                    "SetSendCodec() failed to register codec to"
                    " RTP/RTCP module");
            return -1;
        }
    }

    if (_rtpRtcpModule->SetAudioPacketSize(codec.pacsize) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId_,instanceId_),
                     "SetSendCodec() failed to set audio packet size");
        return -1;
    }

    return 0;
}

int32_t ACMOutputProcessor::SendData(FrameType frame_type, uint8_t payload_type,
        uint32_t timestamp, const uint8_t* payload_data,
        uint16_t payload_len_bytes,
        const RTPFragmentationHeader* fragmentation) {
    // Push data from ACM to RTP/RTCP-module to deliver audio frame for
    // packetization.
    // This call will trigger Transport::SendPacket() from the RTP/RTCP module.
    if (_rtpRtcpModule->SendOutgoingData((FrameType&)frame_type,
                                        payload_type,
                                        timestamp,
                                        // Leaving the time when this frame was
                                        // received from the capture device as
                                        // undefined for voice for now.
                                        -1,
                                        payload_data,
                                        payload_len_bytes,
                                        fragmentation) == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId_,instanceId_),
                "Channel::SendData() failed to send data to RTP/RTCP module");
        return -1;
    }

    return 0;
}

void ACMOutputProcessor::NewMixedAudio(const int32_t id,
        const AudioFrame& generalAudioFrame,
        const AudioFrame** uniqueAudioFrames, const uint32_t size)
{
    if (_outputFileRecording && _outputFileRecorderPtr)
        _outputFileRecorderPtr->RecordAudioToFile(generalAudioFrame, NULL);

    audio_coding_->Add10MsData(generalAudioFrame);
    audio_coding_->Process();
}

int32_t ACMOutputProcessor::MixActiveChannels(const boost::system::error_code& ec) {
    if (!ec) {
        amixer_->Process();
        if (!m_isClosing) {
              timer_->expires_at(timer_->expires_at() + boost::posix_time::milliseconds(10));
              timer_->async_wait(boost::bind(&ACMOutputProcessor::MixActiveChannels, this, boost::asio::placeholders::error));
        }
    } else {
        ELOG_INFO("ACMOutputProcessor timer error: %s", ec.message().c_str());
    }
    return 0;
}

void ACMOutputProcessor::PlayNotification(int32_t id, uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::PlayNotification(id=%d, durationMs=%d)",
                 id, durationMs);
    // Not implement yet
}

void ACMOutputProcessor::RecordNotification(int32_t id,
                                     uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::RecordNotification(id=%d, durationMs=%d)",
                 id, durationMs);

    // Not implement yet
}

void ACMOutputProcessor::PlayFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::PlayFileEnded(id=%d)", id);

    // not needed
}

void ACMOutputProcessor::RecordFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::RecordFileEnded(id=%d)", id);
    assert(id == instanceId_);

    _outputFileRecording = false;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::RecordFileEnded() =>"
                 "output file recorder module is shutdown");
}

int ACMOutputProcessor::StartRecordingPlayout(const char* fileName,
                                       const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::StartRecordingPlayout(fileName=%s)", fileName);

    if (_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
                     "StartRecordingPlayout() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0);
    CodecInst dummyCodec={100,"L16",16000,320,1,320000};

    if ((codecInst != NULL) &&
      ((codecInst->channels < 1) || (codecInst->channels > 2)))
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
                "StartRecordingPlayout() invalid compression");
        return(-1);
    }
    if(codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst=&dummyCodec;
    }
    else if((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    }
    else
    {
        format = kFileFormatCompressedFile;
    }


    // Destroy the old instance
    if (_outputFileRecorderPtr)
    {
        _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
    }

    _outputFileRecorderPtr = FileRecorder::CreateFileRecorder(
        instanceId_,
        (const FileFormats)format);
    if (_outputFileRecorderPtr == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
                "StartRecordingPlayout() fileRecorder format isnot correct");
        return -1;
    }

    if (_outputFileRecorderPtr->StartRecordingAudioFile(
        fileName,
        (const CodecInst&)*codecInst,
        notificationTime) != 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
                "StartRecordingAudioFile() failed to start file recording");
        _outputFileRecorderPtr->StopRecording();
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
        return -1;
    }
    _outputFileRecorderPtr->RegisterModuleFileCallback(this);
    _outputFileRecording = true;

    return 0;
}


int ACMOutputProcessor::StopRecordingPlayout()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::StopRecordingPlayout()");

    if (!_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId_,-1),
                     "StopRecordingPlayout() file isnot recording");
        return -1;
    }

    if (_outputFileRecorderPtr->StopRecording() != 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
                "StopRecording(), could not stop recording");
        return -1;
    }
    _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
    FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
    _outputFileRecorderPtr = NULL;
    _outputFileRecording = false;

    return 0;
}

} /* namespace mcu */
