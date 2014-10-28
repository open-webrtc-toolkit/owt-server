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

#ifndef ACMOutputProcessor_h
#define ACMOutputProcessor_h

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <WoogeenTransport.h>
#include <webrtc/modules/audio_coding/main/interface/audio_coding_module.h>
#include <webrtc/modules/audio_conference_mixer/interface/audio_conference_mixer.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/modules/utility/interface/file_recorder.h>
#include <webrtc/system_wrappers/interface/scoped_ptr.h>
#include <webrtc/voice_engine/voice_engine_defines.h>

using namespace webrtc;

namespace mcu {

class TaskRunner;

class ACMOutputProcessor: public AudioPacketizationCallback,
                          public AudioMixerStatusReceiver,
                          public AudioMixerOutputReceiver,
                          public FileCallback{
    DECLARE_LOGGER();
public:
    ACMOutputProcessor(uint32_t instanceId, woogeen_base::WoogeenTransport<erizo::AUDIO>* transport);
    virtual ~ACMOutputProcessor();
    bool init(boost::shared_ptr<TaskRunner>);

    int32_t SetAudioProcessingModule(
        AudioProcessing* audioProcessingModule) {
        apm_ = audioProcessingModule;
        return 0;
    }

    int32_t MixActiveChannels(const boost::system::error_code& ec);

    int32_t SetMixabilityStatus(MixerParticipant& participant,
                                bool mixable) {
        return amixer_->SetMixabilityStatus(participant, mixable);
    }

    int32_t SetAnonymousMixabilityStatus(MixerParticipant& participant,
                                         bool mixable) {
        return amixer_->SetAnonymousMixabilityStatus(participant, mixable);
    }

    int32_t SetSendCodec(const CodecInst& codec);
    // Implements AudioPacketizationCallback.
    virtual int32_t SendData(
          FrameType frame_type,
          uint8_t payload_type,
          uint32_t timestamp,
          const uint8_t* payload_data,
          uint16_t payload_len_bytes,
          const RTPFragmentationHeader* fragmentation);
    // Implements AudioMixerOutputReceiver.
    virtual void NewMixedAudio(const int32_t id,
                               const AudioFrame& generalAudioFrame,
                               const AudioFrame** uniqueAudioFrames,
                               const uint32_t size);
    // Callback function that provides an array of ParticipantStatistics for the
    // participants that were mixed last mix iteration.
    virtual void MixedParticipants(
        const int32_t id,
        const ParticipantStatistics* participantStatistics,
        const uint32_t size) {};
    // Callback function that provides an array of the ParticipantStatistics for
    // the participants that had a positiv VAD last mix iteration.
    virtual void VADPositiveParticipants(
        const int32_t id,
        const ParticipantStatistics* participantStatistics,
        const uint32_t size) {};
    // Callback function that provides the audio level of the mixed audio frame
    // from the last mix iteration.
    virtual void MixedAudioLevel(
        const int32_t  id,
        const uint32_t level) {};

    // This function is called by MediaFile when a file has been playing for
    // durationMs ms. id is the identifier for the MediaFile instance calling
    // the callback.
    virtual void PlayNotification(const int32_t id,
                                  const uint32_t durationMs);

    // This function is called by MediaFile when a file has been recording for
    // durationMs ms. id is the identifier for the MediaFile instance calling
    // the callback.
    virtual void RecordNotification(const int32_t id,
                                    const uint32_t durationMs);

    // This function is called by MediaFile when a file has been stopped
    // playing. id is the identifier for the MediaFile instance calling the
    // callback.
    virtual void PlayFileEnded(const int32_t id);

    // This function is called by MediaFile when a file has been stopped
    // recording. id is the identifier for the MediaFile instance calling the
    // callback.
    virtual void RecordFileEnded(const int32_t id);

    int StartRecordingPlayout(const char* fileName,  const CodecInst* codecInst);
    int StopRecordingPlayout();
    FileRecorder* getOutputFileRecorder() {
        return _outputFileRecorderPtr;
    }

private:
    AudioConferenceMixer* amixer_;
    AudioProcessing* apm_;
    AudioFrame audioFrame_;
    uint32_t instanceId_;
    boost::shared_ptr<TaskRunner> taskRunner_;
    scoped_ptr<RtpRtcp> _rtpRtcpModule;
    scoped_ptr<AudioCodingModule> audio_coding_;
    scoped_ptr<woogeen_base::WoogeenTransport<erizo::AUDIO>> m_audioTransport;    // owned by this ACMOutputProcessor

    boost::scoped_ptr<boost::thread> audioMixingThread_;
    boost::asio::io_service io_service_;
    boost::scoped_ptr<boost::asio::deadline_timer> timer_;
    std::atomic<bool> m_isStopping;

    FileRecorder* _outputFileRecorderPtr;
    bool _outputFileRecording;
};

} /* namespace mcu */

#endif /* ACMOutputProcessor_h */
