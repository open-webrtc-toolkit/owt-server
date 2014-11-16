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

#ifndef ProtectedRTPReceiver_h
#define ProtectedRTPReceiver_h

#include "QoSUtility.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>
#include <set>
#include <MediaDefinitions.h>
#include <webrtc/modules/rtp_rtcp/interface/fec_receiver.h>
#include <webrtc/modules/rtp_rtcp/interface/receive_statistics.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h>
#include <webrtc/system_wrappers/interface/clock.h>

namespace woogeen_base {

class ProtectedRTPReceiver: public erizo::MediaSink, public erizo::FeedbackSource {
    DECLARE_LOGGER();
public:
    ProtectedRTPReceiver(boost::shared_ptr<erizo::RTPDataReceiver>);
    ~ProtectedRTPReceiver();

    // Implement the MediaSink interfaces.
    int deliverAudioData(char*, int len);
    int deliverVideoData(char*, int len);
    bool acceptEncapsulatedRTPData() { return true; }
    bool acceptFEC() { return true; }
    bool acceptResentData() { return false; }

    void setNACKStatus(bool enable);

    // Generate the RTCP packet.
    uint32_t generateRTCPFeedback(char*, uint32_t bufferSize, bool nack = false);

    uint32_t packetsReceived() { return m_packetsReceived; }
    uint32_t packetsLost() { return m_packetsLost; }

private:
    typedef struct {
        uint32_t midNTP;
        uint32_t arrivialSecs;
        uint32_t arrivialFrac;
    } LastSR;

    class SequenceNumberOlderThan {
    public:
        bool operator() (const uint16_t& first, const uint16_t& second) const
        {
            return webrtc::IsNewerSequenceNumber(second, first);
        }
    };

    typedef std::set<uint16_t, SequenceNumberOlderThan> SequenceNumberSet;

    struct ReceivedSequenceNumberInfo {
        ReceivedSequenceNumberInfo()
            : latestSequenceNumber(0)
            , isFirstSequenceNumber(true)
        {
        }

        SequenceNumberSet lostSequenceNumbers;
        uint16_t latestSequenceNumber;
        bool isFirstSequenceNumber;
    };

    void processRtcpData(char*, int len, bool isAudio);
    webrtc::FecReceiver* getFecReceiver(uint32_t ssrc);
    void updateLostSequenceNumbers(uint16_t currentReceived, uint32_t ssrc);
    uint32_t generateNACK(SequenceNumberSet&, char*, uint32_t maxLength, uint32_t sourceSSRC);
    uint32_t generateNACKs(char*, uint32_t maxLength);
    uint32_t generateReceiverReport(char*, uint32_t maxLength);

    webrtc::Clock* m_clock;
    boost::scoped_ptr<webrtc::RtpHeaderParser> m_rtpHeaderParser;

    // These are used for receiver statistics and receiver report generation.
    std::map<uint32_t, LastSR> m_lastSR;
    boost::shared_mutex m_lastSRMutex;
    boost::scoped_ptr<webrtc::ReceiveStatistics> m_receiveStatistics;

    // These are used for FEC packets handling.
    std::vector<boost::shared_ptr<WebRTCRtpData>> m_webRTCRtpData;
    std::map<uint32_t, boost::shared_ptr<webrtc::FecReceiver>> m_fecReceivers;
    boost::shared_mutex m_fecMutex;

    // These are used to count the lost sequence numbers.
    // If we want to send NACKs to the sender we need this information.
    std::map<uint32_t, boost::shared_ptr<ReceivedSequenceNumberInfo>> m_sequenceNumberInfo;
    boost::shared_mutex m_sequenceNumberMutex;
    bool m_nackEnabled;

    std::atomic<uint32_t> m_packetsReceived;
    std::atomic<uint32_t> m_packetsLost;

    // This is the RTP receiver that we will pass the decapsulated RTP data to.
    boost::shared_ptr<erizo::RTPDataReceiver> m_decapsulatedRTPReceiver;
};

} // namespace woogeen_base

#endif /* ProtectedRTPReceiver_h */
