/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#include "VCMInputProcessor.h"

#include "ACMMediaProcessor.h"
#include "AVSyncModule.h"
#include "AVSyncTaskRunner.h"

using namespace webrtc;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(VCMInputProcessor, "media.VCMInputProcessor");

VCMInputProcessor::VCMInputProcessor(int index)
{
    vcm_ = NULL;
    index_ = index;
}

VCMInputProcessor::~VCMInputProcessor()
{
    recorder_->Stop();
    Trace::ReturnTrace();

    boost::unique_lock<boost::mutex> lock(m_rtpReceiverMutex);
    rtp_receiver_.reset();
    lock.unlock();

    if (taskRunner_) {
        taskRunner_->unregisterModule(avSync_.get());
        taskRunner_->unregisterModule(rtp_rtcp_.get());
        taskRunner_->unregisterModule(vcm_);
    }

    if (vcm_)
        VideoCodingModule::Destroy(vcm_), vcm_ = NULL;
}

bool VCMInputProcessor::init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<BufferManager> bufferManager, boost::shared_ptr<InputFrameCallback> frameReadyCB, boost::shared_ptr<TaskRunner> taskRunner)
{
    Trace::CreateTrace();
    Trace::SetTraceFile("webrtc.trace.txt");
    Trace::set_level_filter(webrtc::kTraceAll);

    bufferManager_ = bufferManager;
    frameReadyCB_ = frameReadyCB;
    taskRunner_ = taskRunner;

    vcm_ = VideoCodingModule::Create(index_);
    if (vcm_) {
        vcm_->InitializeReceiver();
        vcm_->RegisterReceiveCallback(this);
        taskRunner_->registerModule(vcm_);
    } else
        return false;

    rtp_header_parser_.reset(RtpHeaderParser::Create());
    rtp_payload_registry_.reset(new RTPPayloadRegistry(index_, RTPPayloadStrategy::CreateStrategy(false)));
    rtp_receiver_.reset(RtpReceiver::CreateVideoReceiver(index_, Clock::GetRealTimeClock(), this, NULL,
                                rtp_payload_registry_.get()));

    m_videoTransport.reset(transport);
    m_receiveStatistics.reset(ReceiveStatistics::Create(Clock::GetRealTimeClock()));

    RtpRtcp::Configuration configuration;
    configuration.id = 002;
    configuration.audio = false;  // Video.
    configuration.outgoing_transport = transport;	// for sending RTCP feedback to the publisher
    configuration.receive_statistics = m_receiveStatistics.get();
    rtp_rtcp_.reset(RtpRtcp::CreateRtpRtcp(configuration));
    rtp_rtcp_->SetRTCPStatus(webrtc::kRtcpCompound);
    rtp_rtcp_->SetKeyFrameRequestMethod(kKeyFrameReqPliRtcp);
    taskRunner_->registerModule(rtp_rtcp_.get());

    //register codec
    VideoCodec video_codec;
    if (VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &video_codec) != VCM_OK) {
      return false;
    }

    setReceiveVideoCodec(video_codec);

    if (video_codec.codecType != kVideoCodecRED &&
        video_codec.codecType != kVideoCodecULPFEC) {
      // Register codec type with VCM, but do not register RED or ULPFEC.
      if (vcm_->RegisterReceiveCodec(&video_codec, 1, true) != VCM_OK) {
        return true;
      }
    }

    avSync_.reset(new AVSyncModule(vcm_, index_));
    recorder_.reset(new DebugRecorder());
    recorder_->Start("/home/qzhang8/webrtc/webrtc.frame.i420");
    return true;
}

void VCMInputProcessor::close() {
}

void VCMInputProcessor::bindAudioInputProcessor(boost::shared_ptr<ACMInputProcessor> aip)
{
    aip_ = aip;
    if (avSync_) {
        avSync_->ConfigureSync(aip_, rtp_rtcp_.get(), rtp_receiver_.get());
        taskRunner_->registerModule(avSync_.get());
    }
}

int32_t VCMInputProcessor::FrameToRender(I420VideoFrame& videoFrame)
{
    ELOG_DEBUG("Got decoded frame from %d\n", index_);
    frameReadyCB_->handleInputFrame(videoFrame, index_);
    return 0;
}

int32_t VCMInputProcessor::OnReceivedPayloadData(
    const uint8_t* payloadData,
    const uint16_t payloadSize,
    const WebRtcRTPHeader* rtpHeader)
{
    vcm_->IncomingPacket(payloadData, payloadSize, *rtpHeader);
    int32_t ret = vcm_->Decode(0);
    ELOG_DEBUG("OnReceivedPayloadData index= %d, decode result = %d",  index_, ret);

    return 0;
}

bool VCMInputProcessor::setReceiveVideoCodec(const VideoCodec& video_codec)
{
    int8_t old_pltype = -1;
    if (rtp_payload_registry_->ReceivePayloadType(video_codec.plName,
        kVideoPayloadTypeFrequency,
        0,
        video_codec.maxBitrate,
        &old_pltype) != -1) {
        rtp_payload_registry_->DeRegisterReceivePayload(old_pltype);
    }

    return rtp_receiver_->RegisterReceivePayload(video_codec.plName,
        video_codec.plType,
        kVideoPayloadTypeFrequency,
        0,
        video_codec.maxBitrate) == 0;
}


void VCMInputProcessor::receiveRtpData(char* rtp_packet, int rtp_packet_length, erizo::DataType type , uint32_t streamId)
{
    assert(type == erizo::VIDEO);

    webrtc::RTPHeader header;
    if (!rtp_header_parser_->Parse(reinterpret_cast<uint8_t*>(rtp_packet), rtp_packet_length, &header)) {
        ELOG_DEBUG("Incoming packet: Invalid RTP header");
        return;
    }
    int payload_length = rtp_packet_length - header.headerLength;
    header.payload_type_frequency = kVideoPayloadTypeFrequency;
    bool in_order = true; /*IsPacketInOrder(header)*/;
    rtp_payload_registry_->SetIncomingPayloadType(header);
    PayloadUnion payload_specific;
    if (!rtp_payload_registry_->GetPayloadSpecifics(header.payloadType, &payload_specific)) {
       return;
    }
    boost::unique_lock<boost::mutex> lock(m_rtpReceiverMutex);
    if (rtp_receiver_) {
        rtp_receiver_->IncomingRtpPacket(header, reinterpret_cast<uint8_t*>(rtp_packet + header.headerLength) , payload_length,
            payload_specific, in_order);
    }

    bool isRetransmitted = false; // IsPacketRetransmitted(header, in_order);
    if (m_receiveStatistics)
        m_receiveStatistics->IncomingPacket(header, rtp_packet_length, isRetransmitted);
}

}

