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

#ifndef ProtectedRTPSender_h
#define ProtectedRTPSender_h

#include <boost/scoped_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <webrtc/modules/rtp_rtcp/source/bitrate.h>
#include <webrtc/modules/rtp_rtcp/source/producer_fec.h>
#include <webrtc/modules/rtp_rtcp/source/rtp_packet_history.h>

namespace woogeen_base {

static const uint32_t NUMBER_OF_SR = 60;
static const uint32_t NUMBER_OF_PROTECTED_FRAMES = 10;

class ProtectedRTPSender {
    DECLARE_LOGGER();

public:
    ProtectedRTPSender(uint32_t id, erizo::RTPDataReceiver*);
    virtual ~ProtectedRTPSender();

    void sendPacket(char*, uint32_t payloadLength, uint32_t headerLength, erizo::DataType);

    void enableEncapsulatedRTPData(bool enable) { m_encapsulateRTPData = enable; }
    bool encapsulatedRTPDataEnabled() { return m_encapsulateRTPData; }

    void outgoingVideoFrame(bool isKey);
    void protectVideoPacket(char*, uint32_t rtpPayloadLength, uint32_t rtpHeaderLength, uint32_t packetId);
    int32_t setFecParameters(const webrtc::FecProtectionParams& deltaParams, const webrtc::FecProtectionParams& keyParams);
    void setFecStatus(bool enable) { m_fecEnabled = enable; }
    bool fecEnabled() { return m_fecEnabled; }

    void resendPacket(uint16_t sequenceNumber, uint32_t minResendInterval, erizo::DataType);
    void setNACKStatus(bool enable);
    bool nackEnabled();

    void sendSenderReport(char*, uint32_t length, erizo::DataType);
    uint64_t sendTimeOfSenderReport(uint32_t senderReport);

    uint32_t incrementSequenceNumber();
    // The input packetId should not be wrapped.
    uint32_t produceSequenceNumberForPacket(uint32_t packetId);

    uint32_t packetsSent() { return m_packetsSent; }
    uint32_t payloadBytesSent() { return m_payloadBytesSent; }

    void processBitrate();
    uint32_t mediaSentBitrate();
    uint32_t nackSentBitrate();
    uint32_t fecSentBitrate();

    void restart();

private:
    typedef struct {
        uint32_t lastPacketId;
        uint32_t numberSentFECPackets;
    } FECProtectedFrame;

    uint32_t m_id;
    bool m_encapsulateRTPData;
    webrtc::Clock* m_clock;

    std::atomic<uint32_t> m_highestSeqNumber;
    // m_packetsSent doesn't include re-transmitted packets.
    std::atomic<uint32_t> m_packetsSent;
    // m_payloadBytesSent doesn't include re-transmitted bytes.
    std::atomic<uint32_t> m_payloadBytesSent;
    // The start sequence number.
    std::atomic<uint32_t> m_initialSeqNumber;
    // The first packet id for a newly started or restarted stream.
    std::atomic<uint32_t> m_initialPacketId;

    // These are used to generate the receiver report.
    uint32_t m_lastSenderReport[NUMBER_OF_SR];
    uint64_t m_lastRTCPTime[NUMBER_OF_SR];
    boost::shared_mutex m_rtcpMutex;

    // These are used to support NACK at the sender side.
    webrtc::RTPPacketHistory m_packetHistory;
    boost::scoped_ptr<webrtc::Bitrate> m_nackBitrate;

    // These are used to support FEC at the sender side.
    bool m_fecEnabled;
    FECProtectedFrame* m_fecProtectedFrames;
    webrtc::FecProtectionParams m_deltaParams;
    webrtc::FecProtectionParams m_keyParams;
    boost::shared_mutex m_fecMutex;
    boost::scoped_ptr<webrtc::ForwardErrorCorrection> m_fec;
    boost::scoped_ptr<webrtc::ProducerFec> m_fecProducer;
    boost::scoped_ptr<webrtc::Bitrate> m_fecBitrate;

    boost::scoped_ptr<webrtc::Bitrate> m_mediaBitrate;

    // The RTP receiver that we send packets to.
    erizo::RTPDataReceiver* m_rtpReceiver;
};

} /* namespace woogeen_base */

#endif /* ProtectedRTPSender_h */
