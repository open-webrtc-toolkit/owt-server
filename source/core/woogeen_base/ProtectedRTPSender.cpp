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

#include "ProtectedRTPSender.h"

#include <rtputils.h>

using namespace erizo;

namespace woogeen_base {

DEFINE_LOGGER(ProtectedRTPSender, "woogeen.ProtectedRTPSender");

static const int PACKET_HISTORY_SIZE = 600;

ProtectedRTPSender::ProtectedRTPSender(uint32_t id, RTPDataReceiver* rtpReceiver)
    : m_id(id)
    , m_encapsulateRTPData(false)
    , m_clock(webrtc::Clock::GetRealTimeClock())
    , m_highestSeqNumber(0)
    , m_packetsSent(0)
    , m_payloadBytesSent(0)
    , m_initialSeqNumber(0)
    , m_initialPacketId(-1)
    , m_packetHistory(m_clock)
    , m_fecEnabled(false)
    , m_fecProtectedFrames(nullptr)
    , m_rtpReceiver(rtpReceiver)
{
    memset(m_lastSenderReport, 0, sizeof(m_lastSenderReport));
    memset(m_lastRTCPTime, 0, sizeof(m_lastRTCPTime));
    memset(&m_deltaParams, 0, sizeof(webrtc::FecProtectionParams));
    memset(&m_keyParams, 0, sizeof(webrtc::FecProtectionParams));
    m_mediaBitrate.reset(new webrtc::Bitrate(m_clock, nullptr));
}

ProtectedRTPSender::~ProtectedRTPSender()
{
    boost::lock_guard<boost::shared_mutex> fecLock(m_fecMutex);
    delete[] m_fecProtectedFrames;;
    m_fecProtectedFrames = nullptr;
    boost::lock_guard<boost::shared_mutex> rtcpLock(m_rtcpMutex);
}

uint64_t ProtectedRTPSender::sendTimeOfSenderReport(uint32_t senderReport)
{
    boost::shared_lock<boost::shared_mutex> lock(m_rtcpMutex);
    if (m_lastSenderReport[0] == 0 || senderReport == 0)
        return 0;

    for (uint32_t i = 0; i < NUMBER_OF_SR; ++i) {
        if (m_lastSenderReport[i] == senderReport)
            return m_lastRTCPTime[i];
    }

    return 0;
}

void ProtectedRTPSender::sendSenderReport(char* buf, uint32_t length, erizo::DataType type)
{
    m_rtpReceiver->receiveRtpData(buf, length, type, m_id);

    SenderReport* report = reinterpret_cast<SenderReport*>(buf);
    uint32_t ntpSec = report->getNTPTimestampHighBits();
    uint32_t ntpFrac = report->getNTPTimestampLowBits();
    uint64_t now = m_clock->CurrentNtpInMilliseconds();

    boost::lock_guard<boost::shared_mutex> lock(m_rtcpMutex);
    for (int i = NUMBER_OF_SR - 2; i >= 0 ; i--) {
        m_lastSenderReport[i + 1] = m_lastSenderReport[i];
        m_lastRTCPTime[i + 1] = m_lastRTCPTime[i];
    }

    m_lastSenderReport[0] = (ntpSec << 16) + (ntpFrac >> 16);
    m_lastRTCPTime[0] = now;
}

void ProtectedRTPSender::setNACKStatus(bool enable)
{
    m_packetHistory.SetStorePacketsStatus(enable, PACKET_HISTORY_SIZE);

    if (enable && !m_nackBitrate)
        m_nackBitrate.reset(new webrtc::Bitrate(m_clock, nullptr));
}

bool ProtectedRTPSender::nackEnabled()
{
    return m_packetHistory.StorePackets();
}

void ProtectedRTPSender::sendPacket(char* buf, uint32_t payloadLength, uint32_t headerLength, erizo::DataType type)
{
    m_rtpReceiver->receiveRtpData(buf, headerLength + payloadLength, type, m_id);
    m_packetHistory.PutRTPPacket(reinterpret_cast<uint8_t*>(buf), headerLength + payloadLength, MAX_DATA_PACKET_SIZE, 0, webrtc::kAllowRetransmission);
    m_payloadBytesSent += payloadLength;
    ++m_packetsSent;
    m_mediaBitrate->Update(payloadLength + headerLength);
}

void ProtectedRTPSender::resendPacket(uint16_t sequenceNumber, int64_t minResendInterval, erizo::DataType type)
{
    uint8_t buffer[MAX_DATA_PACKET_SIZE];
    size_t length = MAX_DATA_PACKET_SIZE;
    int64_t time = 0;
    if (m_packetHistory.GetPacketAndSetSendTime(sequenceNumber, minResendInterval, true, buffer, &length, &time)) {
        ELOG_DEBUG("Resending packet for stream %u, sequence number %u, length %zu", m_id, sequenceNumber, length);
        m_rtpReceiver->receiveRtpData(reinterpret_cast<char*>(buffer), length, type, m_id);
        assert(m_nackBitrate);
        m_nackBitrate->Update(length);
    }
}

int32_t ProtectedRTPSender::setFecParameters(const webrtc::FecProtectionParams& deltaParams, const webrtc::FecProtectionParams& keyParams)
{
    boost::lock_guard<boost::shared_mutex> lock(m_fecMutex);
    m_deltaParams = deltaParams;
    m_keyParams = keyParams;

    if (!m_fecProducer && m_fecEnabled) {
        m_fec.reset(new webrtc::ForwardErrorCorrection());
        m_fecProducer.reset(new webrtc::ProducerFec(m_fec.get()));
        m_fecProtectedFrames = new FECProtectedFrame[NUMBER_OF_PROTECTED_FRAMES];
        memset(m_fecProtectedFrames, 0, sizeof(m_fecProtectedFrames));
        m_fecBitrate.reset(new webrtc::Bitrate(m_clock, nullptr));
    }

    return 0;
}

void ProtectedRTPSender::outgoingVideoFrame(bool isKey)
{
    if (m_fecProducer) {
        if (isKey)
            m_fecProducer->SetFecParameters(&m_keyParams, 0);
        else
            m_fecProducer->SetFecParameters(&m_deltaParams, 0);
    }
}

void ProtectedRTPSender::protectVideoPacket(char* buf, uint32_t rtpPayloadLength, uint32_t rtpHeaderLength, uint32_t packetId)
{
    if (m_fecEnabled && m_fecProducer
        && m_fecProducer->AddRtpPacketAndGenerateFec(reinterpret_cast<uint8_t*>(buf), rtpPayloadLength, rtpHeaderLength) == 0) {
        uint32_t numberCurrentFECPackets = 0;
        while (m_fecProducer->FecAvailable()) {
            webrtc::RedPacket* fecPacket = m_fecProducer->GetFecPacket(RED_90000_PT, ULP_90000_PT, ++m_highestSeqNumber, rtpHeaderLength);
            ELOG_DEBUG("Sending FEC packet for stream %u, packet %u, sequence number %u, length %zu", m_id, packetId, static_cast<uint16_t>(m_highestSeqNumber), fecPacket->length());
            m_rtpReceiver->receiveRtpData(reinterpret_cast<char*>(fecPacket->data()), fecPacket->length(), VIDEO, m_id);
            m_packetHistory.PutRTPPacket(fecPacket->data(), fecPacket->length(), MAX_DATA_PACKET_SIZE, 0, webrtc::kAllowRetransmission);
            m_payloadBytesSent += (fecPacket->length() - rtpHeaderLength);
            assert(m_fecBitrate);
            m_fecBitrate->Update(fecPacket->length());
            delete fecPacket;
            fecPacket = nullptr;
            ++numberCurrentFECPackets;
        }

        if (numberCurrentFECPackets > 0) {
            boost::lock_guard<boost::shared_mutex> lock(m_fecMutex);
            for (int i = NUMBER_OF_PROTECTED_FRAMES - 2; i >= 0; --i)
                m_fecProtectedFrames[i + 1] = m_fecProtectedFrames[i];

            m_fecProtectedFrames[0].lastPacketId = packetId;
            m_fecProtectedFrames[0].numberSentFECPackets = m_fecProtectedFrames[1].numberSentFECPackets + numberCurrentFECPackets;
            m_packetsSent += numberCurrentFECPackets;
        }
    }
}

uint32_t ProtectedRTPSender::incrementSequenceNumber()
{
    return ++m_highestSeqNumber;
}

// The purpose of this function is to generate sequence numbers based on the
// input packet ids. It's useful when the packets handed over to the RTPSender
// are not continuous (e.g., packet id holes exist) or the packets are out of
// order (e.g., due to unstable network condition or packet re-transmission).
// But there's a problem when FEC is enabled, in which case the sequence number
// doesn't keep the same increasing speed as that of the packet id, because FEC
// packets are inserted in-between the normal RTP packets with the following
// sequence numbers assigned, if the packet id wraps at a certain threshold we
// may produce huge sequence number holes, which confuses the receiver greatly.
//
// Based on the consideration mentioned above, this method assumes that the
// input packet id will NOT be wrapped (except that it overflows a uint32_t),
// and it's the user's responsibility to be aware of this and input the
// unwrapped packet id when using this interface/method.
uint32_t ProtectedRTPSender::produceSequenceNumberForPacket(uint32_t packetId)
{
    if (m_initialPacketId == static_cast<uint32_t>(-1)) {
        // This is the first packet we're going to send.
        m_initialPacketId = packetId;
    }

    // In the corner case where current packet is older than the first packet,
    // we just set its sequence number to be the initial sequence number.
    // Otherwise, in the normal case, we offset the packetId by the first
    // packetId to generate the sequence number. This is to make sure the
    // generated sequence number starts from the initial sequence number.
    // This is important to support pausing and resuming a stream sent to the
    // browser at current stage.
    // FIXME: Handle the corner case better.
    uint32_t sequenceNumber = m_initialSeqNumber + (packetId > m_initialPacketId ? packetId - m_initialPacketId : 0);

    boost::shared_lock<boost::shared_mutex> lock(m_fecMutex);
    if (m_fecProtectedFrames && m_fecProtectedFrames[0].numberSentFECPackets > 0) {
        uint32_t i = 0;
        for (; i < NUMBER_OF_PROTECTED_FRAMES; ++i) {
            if (packetId > m_fecProtectedFrames[i].lastPacketId) {
                sequenceNumber += m_fecProtectedFrames[i].numberSentFECPackets;
                break;
            }
        }

        // If we don't find the frame this packet belongs to in the cached protected frames (10 frames),
        // we adopt the oldest frame.
        if (i == NUMBER_OF_PROTECTED_FRAMES)
            sequenceNumber += m_fecProtectedFrames[NUMBER_OF_PROTECTED_FRAMES - 1].numberSentFECPackets;
    }
    lock.unlock();

    if (sequenceNumber > m_highestSeqNumber)
        m_highestSeqNumber = sequenceNumber;

    ELOG_TRACE("Sequence number for packet %u of stream %u: %u; packets sent %u;", packetId, m_id, static_cast<uint16_t>(sequenceNumber), m_packetsSent.load());
    return sequenceNumber;
}

void ProtectedRTPSender::restart()
{
    // So that we know if a packet is the first one we're going to send or not
    // after restarting the sender.
    m_initialPacketId = -1;
    // Start the sequence number from the last highest sequence number.
    // This is to make the browser happy to play the stream which was paused
    // before but now resumed in the same peer connection.
    m_initialSeqNumber = m_highestSeqNumber + 1;
    // Reset some statistics. Currently we only reset the FEC protected frame
    // information because it's directly related to sequence number generation.
    // FIXME: See if other statistics like the bitrate stuffs need to be reset
    // or not, which depends on how we clearly define the functionality of this
    // method.
    if (m_fecProtectedFrames)
        memset(m_fecProtectedFrames, 0, sizeof(m_fecProtectedFrames));
}

void ProtectedRTPSender::processBitrate()
{
    m_mediaBitrate->Process();
    if (m_nackBitrate)
        m_nackBitrate->Process();
    if (m_fecBitrate)
        m_fecBitrate->Process();
}

uint32_t ProtectedRTPSender::mediaSentBitrate()
{
    return m_mediaBitrate->BitrateLast();
}

uint32_t ProtectedRTPSender::nackSentBitrate()
{
    return m_nackBitrate ? m_nackBitrate->BitrateLast() : 0;
}

uint32_t ProtectedRTPSender::fecSentBitrate()
{
    return m_fecBitrate ? m_fecBitrate->BitrateLast() : 0;
}

} /* namespace woogeen_base */
