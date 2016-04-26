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

#ifndef WebRTCFeedbackProcessor_h
#define WebRTCFeedbackProcessor_h

#include <atomic>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>
#include <rtputils.h>
#include <webrtc/modules/bitrate_controller/include/bitrate_controller.h>
#include <webrtc/system_wrappers/interface/clock.h>

namespace woogeen_base {

class ProtectedRTPSender;

typedef struct {
    uint32_t totalNumberOfPackets;
    uint32_t aggregatedFracLost;
} PacketLossInfo;

class IntraFrameCallback {
public:
    // TODO: Different kinds of Intra Frame requests may need different handlings,
    // e.g., PLI and FIR.
    virtual void handleIntraFrameRequest() = 0;
};

class WebRTCFeedbackProcessor : public erizo::FeedbackSink {
    DECLARE_LOGGER();
public:
    WebRTCFeedbackProcessor(uint32_t id);
    virtual ~WebRTCFeedbackProcessor();

    virtual int deliverFeedback(char*, int len);
    bool acceptNACK() { return true; }

    void reset();

    void initVideoFeedbackReactor(uint32_t streamId, uint32_t ssrc, boost::shared_ptr<ProtectedRTPSender>, boost::shared_ptr<IntraFrameCallback>);
    void initAudioFeedbackReactor(uint32_t streamId, uint32_t ssrc, boost::shared_ptr<ProtectedRTPSender>);
    void resetVideoFeedbackReactor();
    void resetAudioFeedbackReactor();

    void resetPacketLoss();
    PacketLossInfo packetLoss();

    uint32_t lastRTT() { return m_lastRTT; }

    ProtectedRTPSender* videoRTPSender() { return m_videoRTPSender.get(); }
    ProtectedRTPSender* audioRTPSender() { return m_audioRTPSender.get(); }
    uint32_t mediaSourceId() { return m_id; }

private:
    void handleReceiverReport(RTCPHeader*, uint32_t rtcpLength);
    void handleNACK(RTCPHeader*, uint32_t rtcpLength);
    void handlePLI(RTCPHeader*, uint32_t rtcpLength);
    void handleFIR(RTCPHeader*, uint32_t rtcpLength);
    uint32_t calculateRTT(uint32_t lsr, uint32_t dlsr);
    void updatePacketLoss(ReportBlock*);

    uint32_t m_id;
    webrtc::Clock* m_clock;

    PacketLossInfo m_packetLoss;
    std::map<uint32_t, uint32_t> m_lastHighSeqNumbers;
    boost::shared_mutex m_lossMutex;

    boost::shared_ptr<ProtectedRTPSender> m_videoRTPSender;
    boost::shared_ptr<IntraFrameCallback> m_iFrameCallback;
    boost::shared_ptr<ProtectedRTPSender> m_audioRTPSender;
    boost::scoped_ptr<webrtc::BitrateObserver> m_bitrateObserver;
    boost::scoped_ptr<webrtc::BitrateController> m_bitrateController;
    boost::scoped_ptr<webrtc::RtcpBandwidthObserver> m_bandwidthObserver;
    uint32_t m_videoSSRC;
    uint32_t m_audioSSRC;
    std::atomic<uint32_t> m_lastRTT;
    boost::shared_mutex m_feedbackMutex;
};
} // namespace woogeen_base

#endif /* WebRTCFeedbackProcessor_h */
