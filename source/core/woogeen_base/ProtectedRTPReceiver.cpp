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

#include "ProtectedRTPReceiver.h"

#include <rtputils.h>

using namespace erizo;

namespace woogeen_base {

DEFINE_LOGGER(ProtectedRTPReceiver, "woogeen.ProtectedRTPReceiver");

ProtectedRTPReceiver::ProtectedRTPReceiver(boost::shared_ptr<RTPDataReceiver> rtpReceiver)
    : m_nackEnabled(false)
    , m_packetsReceived(0)
    , m_packetsLost(0)
    , m_decapsulatedRTPReceiver(rtpReceiver)
{
    sinkfbSource_ = this;
    m_clock = webrtc::Clock::GetRealTimeClock();
    m_receiveStatistics.reset(webrtc::ReceiveStatistics::Create(m_clock));
    m_rtpHeaderParser.reset(webrtc::RtpHeaderParser::Create());
}

ProtectedRTPReceiver::~ProtectedRTPReceiver()
{
    // Make sure the object is not destroyed when other threads are still
    // accessing some members.
    boost::lock_guard<boost::shared_mutex> lastSRLock(m_lastSRMutex);
    boost::lock_guard<boost::shared_mutex> fecLock(m_fecMutex);
    boost::lock_guard<boost::shared_mutex> seqNumberLock(m_sequenceNumberMutex);
}

void ProtectedRTPReceiver::processRtcpData(char* buf, int len, bool isAudio)
{
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT) { // Sender Report
        SenderReport* report = reinterpret_cast<SenderReport*>(buf);
        uint32_t ssrc = report->getRTCPHeader().getSSRC();
        uint32_t ntp_timestamp_hi = report->getNTPTimestampHighBits();
        uint32_t ntp_timestamp_lo = report->getNTPTimestampLowBits();
        ELOG_DEBUG("Received RTCP Sender Report for %s stream (SSRC %u) - ntp_timestamp_hi: %u, ntp_timestamp_lo: %u", isAudio ? "AUDIO" : "VIDEO", ssrc, ntp_timestamp_hi, ntp_timestamp_lo);

        LastSR lastSR;
        lastSR.midNTP = (ntp_timestamp_hi << 16) + (ntp_timestamp_lo >> 16);
        m_clock->CurrentNtp(lastSR.arrivialSecs, lastSR.arrivialFrac);       
        boost::lock_guard<boost::shared_mutex> lock(m_lastSRMutex);
        m_lastSR[ssrc] = lastSR;
    }

    // TODO: Add a new interface specific for RTCP data in RTPDataReceiver.
    m_decapsulatedRTPReceiver->receiveRtpData(buf, len, isAudio ? AUDIO : VIDEO, chead->getSSRC());
}

static const uint32_t MAX_NACK_LIST_SIZE = 250;

void ProtectedRTPReceiver::setNACKStatus(bool enable)
{
    if (!enable && m_nackEnabled) {
        boost::unique_lock<boost::shared_mutex> lock(m_sequenceNumberMutex);
        m_sequenceNumberInfo.clear();
    }

    m_nackEnabled = enable;
}

void ProtectedRTPReceiver::updateLostSequenceNumbers(uint16_t currentReceived, uint32_t ssrc)
{
    if (!m_nackEnabled)
        return;

    ReceivedSequenceNumberInfo* seqNumberInfo = nullptr;

    boost::unique_lock<boost::shared_mutex> lock(m_sequenceNumberMutex);
    std::map<uint32_t, boost::shared_ptr<ReceivedSequenceNumberInfo>>::iterator it = m_sequenceNumberInfo.find(ssrc);
    if (it == m_sequenceNumberInfo.end()) {
        seqNumberInfo = new ReceivedSequenceNumberInfo();
        m_sequenceNumberInfo[ssrc].reset(seqNumberInfo);
    } else
        seqNumberInfo = (*it).second.get();

    assert(seqNumberInfo);

    if (seqNumberInfo->isFirstSequenceNumber) {
        seqNumberInfo->latestSequenceNumber = currentReceived;
        seqNumberInfo->isFirstSequenceNumber = false;
        return;
    }

    SequenceNumberSet& numbers = seqNumberInfo->lostSequenceNumbers;
    bool hasNewLost = false;
    if (webrtc::IsNewerSequenceNumber(currentReceived, seqNumberInfo->latestSequenceNumber)) {
        for (uint16_t i = seqNumberInfo->latestSequenceNumber + 1; webrtc::IsNewerSequenceNumber(currentReceived, i); ++i) {
            if (numbers.size() == MAX_NACK_LIST_SIZE) {
                ELOG_DEBUG("Remove oldest lost sequence numbers for stream %u due to exceeding maximal allowed size", ssrc);
                numbers.erase(numbers.begin());
            }
            ELOG_DEBUG("Insert %u into the lost sequence number set for stream %u", i, ssrc);
            numbers.insert(numbers.end(), i);
            hasNewLost = true;
        }
        seqNumberInfo->latestSequenceNumber = currentReceived;
    } else {
        ELOG_DEBUG("Remove %u from the lost sequence number set for stream %u", currentReceived, ssrc);
        numbers.erase(currentReceived);
    }

    if (hasNewLost) {
        char rtcpNACKBuffer[MAX_DATA_PACKET_SIZE];
        uint32_t length = generateNACK(numbers, rtcpNACKBuffer, MAX_DATA_PACKET_SIZE, ssrc);
        if (length > 0 && fbSink_)
            fbSink_->deliverFeedback(rtcpNACKBuffer, length);
    }
}

int ProtectedRTPReceiver::deliverAudioData(char* buf, int len)
{
    if (m_rtpHeaderParser->IsRtcp(reinterpret_cast<uint8_t*>(buf), len)) {
        processRtcpData(buf, len, true);
        return 0;
    }

    webrtc::RTPHeader header;
    if (m_rtpHeaderParser->Parse(reinterpret_cast<uint8_t*>(buf), len, &header)) {
        ++m_packetsReceived;
        updateLostSequenceNumbers(header.sequenceNumber, header.ssrc);
        // FIXME: Pass the correct "retransmitted" flag as the last parameter.
        m_receiveStatistics->IncomingPacket(header, len, false);
        m_decapsulatedRTPReceiver->receiveRtpData(buf, len, AUDIO, header.ssrc);
        return len;
    }

    return 0;
}

int ProtectedRTPReceiver::deliverVideoData(char* buf, int len)
{
    if (m_rtpHeaderParser->IsRtcp(reinterpret_cast<uint8_t*>(buf), len)) {
        processRtcpData(buf, len, false);
        return 0;
    }

    webrtc::RTPHeader header;
    if (m_rtpHeaderParser->Parse(reinterpret_cast<uint8_t*>(buf), len, &header)) {
        ++m_packetsReceived;
        updateLostSequenceNumbers(header.sequenceNumber, header.ssrc);
        // FIXME: Pass the correct "retransmitted" flag as the last parameter.
        m_receiveStatistics->IncomingPacket(header, len, false);

        if (header.payloadType == RED_90000_PT) {
            webrtc::FecReceiver* fecReceiver = getFecReceiver(header.ssrc);
            redheader* redhead = reinterpret_cast<redheader*>(buf + header.headerLength);
            if (fecReceiver->AddReceivedRedPacket(header, reinterpret_cast<uint8_t*>(buf), len, ULP_90000_PT)) {
                ELOG_WARN("Stream %u: Incoming RED packet error", header.ssrc);
                return 0;
            }
            fecReceiver->ProcessReceivedFec();

            if (redhead->payloadtype == ULP_90000_PT) {
                // Notify the receiver statistics that a FEC packet is received,
                // though this information is not used for now.
                m_receiveStatistics->FecPacketReceived(header, len);
                ELOG_INFO("Received FEC packet for stream %u, length %u", header.ssrc, len);

                // Send an empty VP8 packet to the RTP receiver, because we want
                // to fill the sequence number hole caused by the FEC packet.
                RTPHeader* rtpHeader = reinterpret_cast<RTPHeader*>(buf);
                // Re-write the payload type.
                rtpHeader->setPayloadType(VP8_90000_PT);
                m_decapsulatedRTPReceiver->receiveRtpData(buf, header.headerLength, VIDEO, header.ssrc);
            }
        } else
            m_decapsulatedRTPReceiver->receiveRtpData(buf, len, VIDEO, header.ssrc);

        return len;
    }

    return 0;
}

uint32_t ProtectedRTPReceiver::generateRTCPFeedback(char* buffer, uint32_t bufferSize, bool nack)
{
    uint32_t nackLength = nack ? generateNACKs(buffer, bufferSize) : 0;
    uint32_t rrLength = generateReceiverReport(&buffer[nackLength], bufferSize - nackLength);
    return nackLength + rrLength;
}

uint32_t ProtectedRTPReceiver::generateNACK(SequenceNumberSet& numbers, char* buf, uint32_t maxLength, uint32_t sourceSSRC)
{
    uint32_t length = 0;
    // The remaining buffer should be able to hold at least one NACK field and the feedback header,
    // otherwise we just skip generating NACK for this list.
    if (!numbers.empty() && length + sizeof(RTCPFeedbackHeader) + sizeof(GenericNACK) <= maxLength) {
        // Write the RTCP header for GenericNACK.
        RTCPFeedbackHeader* chead = reinterpret_cast<RTCPFeedbackHeader*>(buf + length);
        memset(chead, 0, sizeof(RTCPFeedbackHeader));
        chead->getRTCPHeader().setVersion(2);
        chead->getRTCPHeader().setPacketType(RTCP_RTP_Feedback_PT);
        chead->getRTCPHeader().setRCOrFMT(1);
        chead->setSourceSSRC(sourceSSRC);
        length += sizeof(RTCPFeedbackHeader);

        SequenceNumberSet::iterator it = numbers.begin();
        uint32_t numberOfNACKFields = 0;
        while (it != numbers.end()) {
            // FIXME: In case we cannot place all the feedback into one RTCP packet,
            // we should generate multiple RTCP packets, though it's very rare case.
            // Now we just discard the NACK field if so.
            if (length + sizeof(GenericNACK) > maxLength)
                break;

            uint16_t number = *it++;
            uint16_t bitMask = 0;
            while (it != numbers.end()) {
                int shift = static_cast<uint16_t>(*it - number) - 1;
                if (shift >= 0 && shift <= 15) {
                    bitMask |= (1 << shift);
                    ++it;
                } else
                    break;
            }

            GenericNACK* nack = reinterpret_cast<GenericNACK*>(buf + length);
            nack->setPacketId(number);
            nack->setBitMask(bitMask);
            length += sizeof(GenericNACK);
            ++numberOfNACKFields;
        }

        chead->getRTCPHeader().setLength(numberOfNACKFields + 2);
        if (it != numbers.end()) {
            ELOG_DEBUG("RTCP buffer not big enough, discarding some NACK sequence numbers for stream %u", sourceSSRC);
        }
    }

    return length;
}

uint32_t ProtectedRTPReceiver::generateNACKs(char* buf, uint32_t maxLength)
{
    uint32_t length = 0;
    std::map<uint32_t, boost::shared_ptr<ReceivedSequenceNumberInfo>>::iterator it;
    boost::shared_lock<boost::shared_mutex> lock(m_sequenceNumberMutex);
    for (it = m_sequenceNumberInfo.begin(); it != m_sequenceNumberInfo.end(); ++it) {
        ReceivedSequenceNumberInfo* seqNumberInfo = (*it).second.get();
        assert(seqNumberInfo);
        if (maxLength > length)
            length += generateNACK(seqNumberInfo->lostSequenceNumbers, &buf[length], maxLength - length, it->first);
    }

    return length;
}

uint32_t ProtectedRTPReceiver::generateReceiverReport(char* buf, uint32_t maxLength)
{
    uint32_t totalLength = 0;
    webrtc::StatisticianMap stats = m_receiveStatistics->GetActiveStatisticians();
    // The remaining buffer should be able to hold at least one report block and the RTCP header,
    // otherwise we just skip generating the report.
    if (!stats.empty() && totalLength + sizeof(RTCPHeader) + sizeof(ReportBlock) <= maxLength) {
        RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
        memset(chead, 0, sizeof(RTCPHeader));
        chead->setVersion(2);
        chead->setPacketType(RTCP_Receiver_PT);
        uint8_t numberOfBlocks = 0;
        totalLength = sizeof(RTCPHeader);
        ReportBlock* report = reinterpret_cast<ReportBlock*>(buf + totalLength);
        webrtc::StatisticianMap::const_iterator it;
        uint32_t packetsLost = 0;
        for (it = stats.begin(); it != stats.end(); ++it) {
            // FIXME: In case we cannot place all the feedback into one RTCP packet,
            // we should generate multiple RTCP packets, though it's very rare case.
            // Now we just discard the reports if so.
            if (totalLength + sizeof(ReportBlock) > maxLength)
                break;

            webrtc::RtcpStatistics streamStats;
            if (!it->second->GetStatistics(&streamStats, true))
                continue;

            packetsLost += streamStats.cumulative_lost;

            memset(report, 0, sizeof(ReportBlock));
            report->setSourceSSRC(it->first);
            report->setFractionLost(streamStats.fraction_lost);
            report->setCumulativeLost(streamStats.cumulative_lost);
            report->setHighestSeqNumber(streamStats.extended_max_sequence_number);
            report->setJitter(streamStats.jitter);

            // last SR timestamp and delay since last SR are the information
            // needed for the remote endpoint (e.g., the browser) to calculate
            // the RTT (round trip time), as illustrated in RFC 3550 and the
            // WebRTC implementation in browser.
            boost::shared_lock<boost::shared_mutex> lock(m_lastSRMutex);
            std::map<uint32_t, LastSR>::iterator lastSRIt = m_lastSR.find(it->first);
            if (lastSRIt != m_lastSR.end()) {
                LastSR& lastSR = lastSRIt->second;
                uint32_t currentNtpSecs = 0;
                uint32_t currentNtpFrac = 0;
                m_clock->CurrentNtp(currentNtpSecs, currentNtpFrac);
                // Calculate delay since last SR (DLSR).
                // From RFC 3550 the delay should be expressed in units of 1/65536 seconds,
                // between receiving the last SR packet from source SSRC_n and sending this
                // reception report block.
                uint32_t now = (currentNtpSecs & 0x0000FFFF) << 16;
                now += (currentNtpFrac & 0xFFFF0000) >> 16;
                uint32_t lastSRArrivial = (lastSR.arrivialSecs & 0x0000FFFF) << 16;
                lastSRArrivial += (lastSR.arrivialFrac & 0xFFFF0000) >> 16;

                report->setDLSR(now - lastSRArrivial);
                report->setLSR(lastSR.midNTP);
            }
            lock.unlock();

            ELOG_DEBUG("Generated report block: source SSRC %u, fraction lost %u, cumulative lost %u, max sequence number %u, jitter %u, last SR %u, delay since last SR %u", it->first, streamStats.fraction_lost, streamStats.cumulative_lost, streamStats.extended_max_sequence_number, streamStats.jitter, report->getLSR(), report->getDLSR());

            totalLength += sizeof(ReportBlock);
            ++report;
            ++numberOfBlocks;
        }
        chead->setRCOrFMT(numberOfBlocks);
        chead->setLength(totalLength / 4 - 1);
        m_packetsLost = packetsLost;
    }

    return totalLength;
}

webrtc::FecReceiver* ProtectedRTPReceiver::getFecReceiver(uint32_t ssrc)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_fecMutex);
    std::map<uint32_t, boost::shared_ptr<webrtc::FecReceiver>>::iterator it = m_fecReceivers.find(ssrc);
    if (it == m_fecReceivers.end()) {
        WebRTCRtpData* rtpData = new WebRTCRtpData(ssrc, m_decapsulatedRTPReceiver.get());
        webrtc::FecReceiver* fecReceiver = webrtc::FecReceiver::Create(rtpData);

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_webRTCRtpData.push_back(boost::shared_ptr<WebRTCRtpData>(rtpData));
        m_fecReceivers[ssrc].reset(fecReceiver);
        return fecReceiver;
    }

    return (*it).second.get();
}

}/* namespace woogeen_base */
