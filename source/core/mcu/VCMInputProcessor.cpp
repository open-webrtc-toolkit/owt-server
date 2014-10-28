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
#include "TaskRunner.h"
#include <rtputils.h>

using namespace webrtc;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(VCMInputProcessor, "media.VCMInputProcessor");

VCMInputProcessor::VCMInputProcessor(int index)
    : m_index(index)
    , m_vcm(nullptr)
{
}

VCMInputProcessor::~VCMInputProcessor()
{
    m_videoReceiver->StopReceive();
    m_recorder->Stop();
    Trace::ReturnTrace();

    if (m_taskRunner) {
        m_taskRunner->DeRegisterModule(m_avSync.get());
        m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
        m_taskRunner->DeRegisterModule(m_vcm);
    }

    if (m_vcm) {
        VideoCodingModule::Destroy(m_vcm);
        m_vcm = nullptr;
    }
}

bool VCMInputProcessor::init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<BufferManager> bufferManager, boost::shared_ptr<InputFrameCallback> frameReadyCB, boost::shared_ptr<TaskRunner> taskRunner)
{
    Trace::CreateTrace();
    Trace::SetTraceFile("webrtc.trace.txt");
    Trace::set_level_filter(webrtc::kTraceAll);

    m_videoTransport.reset(transport);
    m_bufferManager = bufferManager;
    m_frameReadyCB = frameReadyCB;
    m_taskRunner = taskRunner;

    m_vcm = VideoCodingModule::Create(m_index);
    if (m_vcm) {
        m_vcm->InitializeReceiver();
        m_vcm->RegisterReceiveCallback(this);
    } else
        return false;

    m_remoteBitrateObserver.reset(new DummyRemoteBitrateObserver());
    m_remoteBitrateEstimator.reset(RemoteBitrateEstimatorFactory().Create(m_remoteBitrateObserver.get(), Clock::GetRealTimeClock(), kMimdControl, 0));
    m_videoReceiver.reset(new ViEReceiver(m_index, m_vcm, m_remoteBitrateEstimator.get(), nullptr));

    RtpRtcp::Configuration configuration;
    configuration.id = 002;
    configuration.audio = false;  // Video.
    configuration.outgoing_transport = transport; // For sending RTCP feedback to the publisher
    configuration.remote_bitrate_estimator = m_remoteBitrateEstimator.get();
    configuration.receive_statistics = m_videoReceiver->GetReceiveStatistics();
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));
    m_rtpRtcp->SetRTCPStatus(webrtc::kRtcpCompound);
    // There're 3 options of Intra frame requests: PLI, FIR in RTCP and FIR in RTP (RFC 2032).
    // Since currently our MCU only claims FIR support in SDP, we choose FirRtcp for now.
    m_rtpRtcp->SetKeyFrameRequestMethod(kKeyFrameReqFirRtcp);

    m_videoReceiver->SetRtpRtcpModule(m_rtpRtcp.get());

    // Register codec.
    VideoCodec video_codec;
    if (VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &video_codec) != VCM_OK)
        return false;

    if (video_codec.codecType != kVideoCodecRED &&
        video_codec.codecType != kVideoCodecULPFEC) {
        // Register codec type with VCM, but do not register RED or ULPFEC.
        if (m_vcm->RegisterReceiveCodec(&video_codec, 1, true) != VCM_OK)
            return false;
    }

    m_videoReceiver->SetReceiveCodec(video_codec);

    m_avSync.reset(new AVSyncModule(m_vcm, m_index));
    m_recorder.reset(new DebugRecorder());
    m_recorder->Start("webrtc.frame.i420");

    m_taskRunner->RegisterModule(m_vcm);
    m_taskRunner->RegisterModule(m_rtpRtcp.get());
    m_videoReceiver->StartReceive();
    return true;
}

void VCMInputProcessor::bindAudioInputProcessor(boost::shared_ptr<ACMInputProcessor> aip)
{
    m_aip = aip;
    if (m_avSync) {
        m_avSync->ConfigureSync(m_aip, m_rtpRtcp.get(), m_videoReceiver->GetRtpReceiver());
        m_taskRunner->RegisterModule(m_avSync.get());
    }
}

int32_t VCMInputProcessor::FrameToRender(I420VideoFrame& videoFrame)
{
    ELOG_DEBUG("Got decoded frame from %d\n", m_index);
    m_frameReadyCB->handleInputFrame(videoFrame, m_index);
    return 0;
}

int VCMInputProcessor::deliverVideoData(char* buf, int len, erizo::MediaSource*)
{
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT) { // Sender Report
        m_videoReceiver->ReceivedRTCPPacket(buf, len);
        return len;
    }

    PacketTime current;
    if (m_videoReceiver->ReceivedRTPPacket(buf, len, current) != -1) {
        int32_t ret = m_vcm->Decode(0);
        ELOG_DEBUG("receivedRtpData index= %d, decode result = %d",  m_index, ret);
        return len;
    }

    return 0;
}

int VCMInputProcessor::deliverAudioData(char* buf, int len, erizo::MediaSource*)
{
    assert(false);
    return 0;
}

}

