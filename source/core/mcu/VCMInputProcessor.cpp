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
#include <rtputils.h>

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
    m_videoReceiver->StopReceive();
    recorder_->Stop();
    Trace::ReturnTrace();

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
    } else
        return false;

    m_remoteBitrateObserver.reset(new DummyRemoteBitrateObserver());
    m_remoteBitrateEstimator.reset(RemoteBitrateEstimatorFactory().Create(m_remoteBitrateObserver.get(), Clock::GetRealTimeClock(), kMimdControl, 0));
    m_videoReceiver.reset(new ViEReceiver(index_, vcm_, m_remoteBitrateEstimator.get(), nullptr));
    m_videoTransport.reset(transport);

    RtpRtcp::Configuration configuration;
    configuration.id = 002;
    configuration.audio = false;  // Video.
    configuration.outgoing_transport = transport;	// for sending RTCP feedback to the publisher
    configuration.remote_bitrate_estimator = m_remoteBitrateEstimator.get();
    configuration.receive_statistics = m_videoReceiver->GetReceiveStatistics();
    rtp_rtcp_.reset(RtpRtcp::CreateRtpRtcp(configuration));
    rtp_rtcp_->SetRTCPStatus(webrtc::kRtcpCompound);
    rtp_rtcp_->SetKeyFrameRequestMethod(kKeyFrameReqPliRtcp);

    m_videoReceiver->SetRtpRtcpModule(rtp_rtcp_.get());

    // Register codec.
    VideoCodec video_codec;
    if (VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &video_codec) != VCM_OK) {
      return false;
    }

    if (video_codec.codecType != kVideoCodecRED &&
        video_codec.codecType != kVideoCodecULPFEC) {
      // Register codec type with VCM, but do not register RED or ULPFEC.
      if (vcm_->RegisterReceiveCodec(&video_codec, 1, true) != VCM_OK) {
        return false;
      }
    }

    m_videoReceiver->SetReceiveCodec(video_codec);

    avSync_.reset(new AVSyncModule(vcm_, index_));
    recorder_.reset(new DebugRecorder());
    recorder_->Start("/home/qzhang8/webrtc/webrtc.frame.i420");

    taskRunner_->registerModule(vcm_);
    taskRunner_->registerModule(rtp_rtcp_.get());
    m_videoReceiver->StartReceive();
    return true;
}

void VCMInputProcessor::close() {
}

void VCMInputProcessor::bindAudioInputProcessor(boost::shared_ptr<ACMInputProcessor> aip)
{
    aip_ = aip;
    if (avSync_) {
        avSync_->ConfigureSync(aip_, rtp_rtcp_.get(), m_videoReceiver->GetRtpReceiver());
        taskRunner_->registerModule(avSync_.get());
    }
}

int32_t VCMInputProcessor::FrameToRender(I420VideoFrame& videoFrame)
{
    ELOG_DEBUG("Got decoded frame from %d\n", index_);
    frameReadyCB_->handleInputFrame(videoFrame, index_);
    return 0;
}

void VCMInputProcessor::receiveRtpData(char* rtp_packet, int rtp_packet_length, erizo::DataType type , uint32_t streamId)
{
    assert(type == erizo::VIDEO);

    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(rtp_packet);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT) { // Sender Report
        m_videoReceiver->ReceivedRTCPPacket(rtp_packet, rtp_packet_length);
        return;
    }

    PacketTime current;
    if (m_videoReceiver->ReceivedRTPPacket(rtp_packet, rtp_packet_length, current) != -1) {
        int32_t ret = vcm_->Decode(0);
        ELOG_DEBUG("receivedRtpData index= %d, decode result = %d",  index_, ret);
    }
}

}

