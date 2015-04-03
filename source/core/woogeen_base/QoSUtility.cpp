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

#include "QoSUtility.h"

#include "ProtectedRTPSender.h"

namespace woogeen_base {

DEFINE_LOGGER(VideoFeedbackReactor, "woogeen.VideoFeedbackReactor");

VideoFeedbackReactor::VideoFeedbackReactor(uint32_t id, boost::shared_ptr<ProtectedRTPSender> rtpSender)
    : m_rtpSender(rtpSender)
{
    m_vcm = webrtc::VideoCodingModule::Create();
    m_vcm->RegisterProtectionCallback(this);

    bool nackEnabled = rtpSender->nackEnabled();
    bool fecEnabled = rtpSender->fecEnabled();
    if (nackEnabled && fecEnabled)
        m_vcm->SetVideoProtection(webrtc::kProtectionNackFEC, true);
    else {
        m_vcm->SetVideoProtection(webrtc::kProtectionNackSender, nackEnabled);
        m_vcm->SetVideoProtection(webrtc::kProtectionFEC, fecEnabled);
        m_vcm->SetVideoProtection(webrtc::kProtectionNackFEC, false);
    }
}

VideoFeedbackReactor::~VideoFeedbackReactor()
{
    webrtc::VideoCodingModule::Destroy(m_vcm);
}

void VideoFeedbackReactor::OnNetworkChanged(
    const uint32_t target_bitrate,
    const uint8_t fraction_loss,
    const int64_t rtt)
{
    ELOG_DEBUG("Received new Bitrate requests: target bitrate %u, fraction loss %u, rtt %lu", target_bitrate, fraction_loss, rtt);
    m_vcm->SetChannelParameters(target_bitrate, fraction_loss, rtt);
}

int VideoFeedbackReactor::ProtectionRequest(
    const webrtc::FecProtectionParams* delta_params,
    const webrtc::FecProtectionParams* key_params,
    uint32_t* sent_video_rate_bps,
    uint32_t* sent_nack_rate_bps,
    uint32_t* sent_fec_rate_bps)
{
    ELOG_DEBUG("Received video protection request: FEC rate (key frames %u, delta frames %u); Max #frames (key frames %u, delta frames %u)", key_params->fec_rate, delta_params->fec_rate, key_params->max_fec_frames, delta_params->max_fec_frames);
    m_rtpSender->setFecParameters(*delta_params, *key_params);
    *sent_video_rate_bps = m_rtpSender->mediaSentBitrate();
    *sent_nack_rate_bps = m_rtpSender->nackSentBitrate();
    *sent_fec_rate_bps = m_rtpSender->fecSentBitrate();
    ELOG_DEBUG("Got bitrate information from the rtp sender: video bitrate %u, NACK bitrate %u, FEC bitrate %u)", *sent_video_rate_bps, *sent_nack_rate_bps, *sent_fec_rate_bps);
    return 0;
}

WebRTCRtpData::WebRTCRtpData(uint32_t streamId, erizo::RTPDataReceiver* receiver)
    : m_streamId(streamId)
    , m_rtpReceiver(receiver)
{
}

WebRTCRtpData::~WebRTCRtpData()
{
}

int32_t WebRTCRtpData::OnReceivedPayloadData(
    const uint8_t* payloadData,
    const size_t payloadSize,
    const webrtc::WebRtcRTPHeader* rtpHeader)
{
    return -1;
}

bool WebRTCRtpData::OnRecoveredPacket(const uint8_t* packet, size_t packet_length)
{
    m_rtpReceiver->receiveRtpData(const_cast<char*>(reinterpret_cast<const char*>(packet)), packet_length, erizo::VIDEO, m_streamId);
    return true;
}

} // namespace woogeen_base
