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

#include "WebRTCFeedbackProcessor.h"

#include "ProtectedRTPSender.h"
#include "QoSUtility.h"

using namespace erizo;

namespace woogeen_base {

DEFINE_LOGGER(WebRTCFeedbackProcessor, "woogeen.WebRTCFeedbackProcessor");

WebRTCFeedbackProcessor::WebRTCFeedbackProcessor(uint32_t id)
    : m_id(id)
    , m_videoSSRC(0)
    , m_audioSSRC(0)
    , m_lastRTT(0)
{
    m_clock = webrtc::Clock::GetRealTimeClock();
    resetPacketLoss();
}

WebRTCFeedbackProcessor::~WebRTCFeedbackProcessor()
{
    reset();
}

void WebRTCFeedbackProcessor::reset()
{
    resetPacketLoss();
    resetVideoFeedbackReactor();
    resetAudioFeedbackReactor();
    boost::lock_guard<boost::shared_mutex> lock(m_lossMutex);
    m_lastHighSeqNumbers.clear();
}

void WebRTCFeedbackProcessor::initVideoFeedbackReactor(uint32_t streamId, uint32_t ssrc, boost::shared_ptr<ProtectedRTPSender> rtpSender, boost::shared_ptr<IntraFrameCallback> iFrameCallback)
{
    boost::lock_guard<boost::shared_mutex> lock(m_feedbackMutex);
    m_videoSSRC = ssrc;
    m_videoRTPSender = rtpSender;
    m_iFrameCallback = iFrameCallback;
    m_bitrateObserver.reset(new VideoFeedbackReactor(streamId, rtpSender));
    m_bitrateController.reset(webrtc::BitrateController::CreateBitrateController(m_clock, true));
    m_bandwidthObserver.reset(m_bitrateController->CreateRtcpBandwidthObserver());
    // TODO: Adjust the bitrate settings according to the Gateway settings.
    m_bitrateController->SetBitrateObserver(m_bitrateObserver.get(), 300 * 1000, 0, 2048 * 1000);
}

void WebRTCFeedbackProcessor::initAudioFeedbackReactor(uint32_t streamId, uint32_t ssrc, boost::shared_ptr<ProtectedRTPSender> rtpSender)
{
    boost::lock_guard<boost::shared_mutex> lock(m_feedbackMutex);
    m_audioSSRC = ssrc;
    m_audioRTPSender = rtpSender;
}

void WebRTCFeedbackProcessor::resetVideoFeedbackReactor()
{
    boost::unique_lock<boost::shared_mutex> lossLock(m_lossMutex);
    std::map<uint32_t, uint32_t>::iterator it = m_lastHighSeqNumbers.find(m_videoSSRC);
    if (it != m_lastHighSeqNumbers.end())
        m_lastHighSeqNumbers.erase(it);
    lossLock.unlock();

    boost::lock_guard<boost::shared_mutex> lock(m_feedbackMutex);
    m_videoRTPSender.reset();
    m_bitrateObserver.reset();
    m_bitrateController.reset();
    m_bandwidthObserver.reset();
    m_videoSSRC = 0;
}

void WebRTCFeedbackProcessor::resetAudioFeedbackReactor()
{
    boost::unique_lock<boost::shared_mutex> lossLock(m_lossMutex);
    std::map<uint32_t, uint32_t>::iterator it = m_lastHighSeqNumbers.find(m_audioSSRC);
    if (it != m_lastHighSeqNumbers.end())
        m_lastHighSeqNumbers.erase(it);
    lossLock.unlock();

    boost::lock_guard<boost::shared_mutex> lock(m_feedbackMutex);
    m_audioRTPSender.reset();
    m_audioSSRC = 0;
}

void WebRTCFeedbackProcessor::resetPacketLoss()
{
    boost::lock_guard<boost::shared_mutex> lock(m_lossMutex);
    m_packetLoss.totalNumberOfPackets = 0;
    m_packetLoss.aggregatedFracLost = 0;
}

PacketLossInfo WebRTCFeedbackProcessor::packetLoss()
{
    boost::shared_lock<boost::shared_mutex> lock(m_lossMutex);
    return m_packetLoss;
}

// This should be invoked by the WebRTC subscribers for RTCP feedback.
int WebRTCFeedbackProcessor::deliverFeedback(char* buf, int len)
{
    if (len <= 0)
        return 0;

    char* movingBuf = buf;
    uint32_t rtcpLength = 0;
    int totalLength = 0;
    // There can be compound RTCP packets so we need this loop.
    do {
        movingBuf += rtcpLength;
        RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(movingBuf);
        rtcpLength = (chead->getLength() + 1) * 4;
        totalLength += rtcpLength;
        // TODO: The report block may also be in the Sender report, if the remote
        // WebRTC connection who receives the streams also sends out its own.
        // Currently this doesn't happen because we ask the browser to establish
        // separated connections for receiving and sending, but make sure we don't
        // forget this when we change the MCU/Gateway behavior in the future.
        switch (chead->getPacketType()) {
        case RTCP_Receiver_PT: {
            handleReceiverReport(chead, rtcpLength);
            break;
        }
        case RTCP_RTP_Feedback_PT: {
            uint8_t fmt = chead->getRCOrFMT();
            if (fmt == 1) {
                // Generic NACK.
                handleNACK(chead, rtcpLength);
            }
            break;
        }
        case RTCP_PS_Feedback_PT: {
            switch (chead->getRCOrFMT()) {
            case 1: // PLI
                handlePLI(chead, rtcpLength);
                break; 
            case 4: // FIR
                handleFIR(chead, rtcpLength);
                break; 
            default:
                break;
            }
        }
        default:
            break;
        }
    } while (totalLength < len);

    return len;
}

void WebRTCFeedbackProcessor::handleReceiverReport(RTCPHeader* chead, uint32_t rtcpLength)
{
    assert(chead->getPacketType() == RTCP_Receiver_PT);

    char* buf = reinterpret_cast<char*>(chead);
    uint8_t blockCount = chead->getRCOrFMT();
    ELOG_DEBUG("RTCP receiver report to client %u, length %u, block count %u", m_id, rtcpLength, blockCount);
    uint32_t blockOffset = sizeof(RTCPHeader);
    // Use the RC field of the packet, since the length may include paddings.
    if (blockCount * sizeof(ReportBlock) > rtcpLength - blockOffset) {
        ELOG_WARN("Invalid RTCP receiver report: block count %u, length %u bytes", blockCount, rtcpLength);
        return;
    }

    webrtc::ReportBlockList reportBlocks;
    uint32_t rtt = 0;
    boost::shared_lock<boost::shared_mutex> lock(m_feedbackMutex);

    while (blockCount--) {
        ReportBlock* report = reinterpret_cast<ReportBlock*>(buf + blockOffset);
        ELOG_DEBUG("RTCP receiver report block to client %u, source SSRC %u, frac lost %u, cumulative lost %u, highest seq number %u, jitter %u, lsr %u, dlsr %u", m_id, report->getSourceSSRC(), report->getFractionLost(), report->getCumulativeLost(), report->getHighestSeqNumber(), report->getJitter(), report->getLSR(), report->getDLSR());
        // Only consider the report to the video stream.
        // FIXME: In theory there could be multiple source SSRCs for
        // a single video stream of a single client?
        // For now, one WebRTCFeedbackProcessor is corresponding to one inbound client
        // which is supposed to have maximal one video stream and one audio stream.
        if (m_bandwidthObserver && report->getSourceSSRC() == m_videoSSRC) {
            rtt = calculateRTT(report->getLSR(), report->getDLSR());
            webrtc::RTCPReportBlock reportBlock(
                chead->getSSRC(),
                report->getSourceSSRC(),
                report->getFractionLost(),
                report->getCumulativeLost(),
                report->getHighestSeqNumber(),
                report->getJitter(),
                report->getLSR(),
                report->getDLSR());
            reportBlocks.push_back(reportBlock);
        }
        // TODO: We may remove updatePacketLoss later if we find another approach to
        // getting the average aggregated packet loss information needed by ooVoo.
        updatePacketLoss(report);
        blockOffset += sizeof(ReportBlock);
    }
    if (m_bandwidthObserver)
        m_bandwidthObserver->OnReceivedRtcpReceiverReport(reportBlocks, rtt, m_clock->CurrentNtpInMilliseconds());
}

void WebRTCFeedbackProcessor::handleNACK(RTCPHeader* chead, uint32_t rtcpLength)
{
    assert(chead->getPacketType() == RTCP_RTP_Feedback_PT && chead->getRCOrFMT() == 1);

    char* buf = reinterpret_cast<char*>(chead);
    uint32_t sourceSSRC = reinterpret_cast<RTCPFeedbackHeader*>(chead)->getSourceSSRC();
    ELOG_DEBUG("RTCP Generic NACK to client %u, length %u, source SSRC %u", m_id, rtcpLength, sourceSSRC);

    boost::shared_lock<boost::shared_mutex> lock(m_feedbackMutex);
    erizo::DataType type = (sourceSSRC == m_videoSSRC ? VIDEO : AUDIO);
    ProtectedRTPSender* rtpSender = (sourceSSRC == m_videoSSRC ? m_videoRTPSender.get() : m_audioRTPSender.get());
    if (rtpSender) {
        uint32_t itemOffset = sizeof(RTCPFeedbackHeader);
        while (itemOffset < rtcpLength) {
            GenericNACK* nackItem = reinterpret_cast<GenericNACK*>(buf + itemOffset);
            uint16_t sequenceNumber = nackItem->getPacketId();
            uint16_t bitMask = nackItem->getBitMask();
            rtpSender->resendPacket(sequenceNumber, m_lastRTT + 5, type);
            if (bitMask) {
                for (int i = 1; i <= 16; ++i) {
                    if (bitMask & 0x01)
                        rtpSender->resendPacket(sequenceNumber + i, m_lastRTT + 5, type);
                    bitMask = bitMask >> 1;
                }
            }
            itemOffset += sizeof(GenericNACK);
        }
    }
}

void WebRTCFeedbackProcessor::handleFIR(RTCPHeader*, uint32_t)
{
    if (m_iFrameCallback)
        m_iFrameCallback->handleIntraFrameRequest();
}

void WebRTCFeedbackProcessor::handlePLI(RTCPHeader*, uint32_t)
{
    if (m_iFrameCallback)
        m_iFrameCallback->handleIntraFrameRequest();
}

uint32_t WebRTCFeedbackProcessor::calculateRTT(uint32_t lsr, uint32_t dlsr)
{
    ELOG_DEBUG("Calculating RTT for video stream %u (SSRC %u)", m_id, m_videoSSRC);
    if (!m_videoRTPSender)
        return 0;

    // Get the send time of the corresponding sender report.
    uint64_t sendTimeOfSRInMs = m_videoRTPSender->sendTimeOfSenderReport(lsr);
    uint32_t rtt = 0;

    if (sendTimeOfSRInMs > 0) {
        // dlsr is in units of 1/65536 seconds.
        uint32_t delayInMs = (dlsr & 0x0000FFFF) * 1000;
        delayInMs /= 65536;
        delayInMs += ((dlsr & 0xFFFF0000) >> 16) * 1000;
        // Get current time (the receive time of current receiver report).
        uint64_t receiveTimeOfRRInMs = m_clock->CurrentNtpInMilliseconds();
        rtt = receiveTimeOfRRInMs - sendTimeOfSRInMs - delayInMs;
        if (rtt <= 0)
            rtt = 1;
        ELOG_DEBUG("Calculated RTT for video stream %u (SSRC %u): %u (send time of SR %lu, receive time of RR %lu, delay %u)", m_id, m_videoSSRC, rtt, sendTimeOfSRInMs, receiveTimeOfRRInMs, delayInMs);
        m_lastRTT = rtt;
    }

    return rtt;
}

void WebRTCFeedbackProcessor::updatePacketLoss(ReportBlock* report)
{
    ELOG_DEBUG("Updating packet loss information for %u", m_id);
    uint32_t sourceSsrc = report->getSourceSSRC();
    boost::lock_guard<boost::shared_mutex> lock(m_lossMutex);
    std::map<uint32_t, uint32_t>::iterator it = m_lastHighSeqNumbers.find(sourceSsrc);
    if (it != m_lastHighSeqNumbers.end()) {
        uint32_t numberOfPackets = report->getHighestSeqNumber() - it->second;
        // The fraction of RTP data packets from source SSRC_n lost since the
        // previous SR or RR packet was sent, expressed as a fixed point
        // number with the binary point at the left edge of the field.    (That
        // is equivalent to taking the integer part after multiplying the
        // loss fraction by 256.)
        // FIXME: In theory, the aggregate can overflow?
        m_packetLoss.aggregatedFracLost += numberOfPackets * report->getFractionLost();
        m_packetLoss.totalNumberOfPackets += numberOfPackets;
        it->second = report->getHighestSeqNumber();
    } else
        m_lastHighSeqNumbers[sourceSsrc] = report->getHighestSeqNumber();

    if (m_packetLoss.totalNumberOfPackets > 0) {
        double lossRate = ((m_packetLoss.aggregatedFracLost + m_packetLoss.totalNumberOfPackets / 2) / m_packetLoss.totalNumberOfPackets) / 256.0;
        ELOG_DEBUG("Updated packet loss using the report to stream %u of client %u, total #packets %u, aggregated loss rate %f (this report: %u)", sourceSsrc, m_id, m_packetLoss.totalNumberOfPackets, lossRate, report->getFractionLost());
    }
}

}/* namespace woogeen_base */
