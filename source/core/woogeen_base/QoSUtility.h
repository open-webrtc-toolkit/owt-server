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

#ifndef QoSUtility_h
#define QoSUtility_h

#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <webrtc/modules/bitrate_controller/include/bitrate_controller.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>

namespace woogeen_base {

class ProtectedRTPSender;

/*
 * Observer class for the encoders, each encoder should implement this class
 * to get the target bitrate. It also get the fraction loss and rtt to
 * optimize its settings for this type of network. |target_bitrate| is the
 * target media/payload bitrate excluding packet headers, measured in bits
 * per second.
 */
// Callback class used for telling the user about how to configure the FEC,
// and the rates sent the last second is returned to the VCM.
class VideoFeedbackReactor : public webrtc::BitrateObserver, public webrtc::VCMProtectionCallback {
    DECLARE_LOGGER();
public:
    VideoFeedbackReactor(uint32_t id, boost::shared_ptr<ProtectedRTPSender>);
    ~VideoFeedbackReactor();

    virtual void OnNetworkChanged(
        const uint32_t target_bitrate,
        const uint8_t fraction_loss,
        const uint32_t rtt);

    virtual int ProtectionRequest(
        const webrtc::FecProtectionParams* delta_params,
        const webrtc::FecProtectionParams* key_params,
        uint32_t* sent_video_rate_bps,
        uint32_t* sent_nack_rate_bps,
        uint32_t* sent_fec_rate_bps);

private:
    boost::shared_ptr<ProtectedRTPSender> m_rtpSender;
    webrtc::VideoCodingModule* m_vcm;
};

class WebRTCRtpData : public webrtc::RtpData {
public:
    WebRTCRtpData(uint32_t streamId, erizo::RTPDataReceiver*);
    virtual ~WebRTCRtpData();

    virtual int32_t OnReceivedPayloadData(
        const uint8_t* payloadData,
        const uint16_t payloadSize,
        const webrtc::WebRtcRTPHeader*);

    virtual bool OnRecoveredPacket(const uint8_t* packet, int packet_length);

private:
    uint32_t m_streamId;
    erizo::RTPDataReceiver* m_rtpReceiver;
};

} // namespace woogeen_base
#endif /* QoSUtility_h */
