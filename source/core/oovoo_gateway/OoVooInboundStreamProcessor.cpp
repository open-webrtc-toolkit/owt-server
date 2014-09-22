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

#include "OoVooInboundStreamProcessor.h"

#include <assert.h>
#include <rtputils.h>

using namespace erizo;
using namespace woogeen_base;

namespace oovoo_gateway {

DEFINE_LOGGER(OoVooInboundStreamProcessor, "ooVoo.OoVooInboundStreamProcessor");

static bool isNewerPacketId(uint32_t current, uint32_t last, bool isAudio)
{
    // According to ooVoo: the packet ID for video is actually in uint16_t;
    // the packet ID for audio wraps at 2^32/20 = 214748364 (0xCCCCCCC)
    if (isAudio) {
        return (last < current && current - last <= 0x6666666)
            || (last > current && last - current > 0x6666666);
    }

    return last != current
        && static_cast<uint16_t>(static_cast<uint16_t>(current) - static_cast<uint16_t>(last)) < 0x8000;
}

bool OoVooInboundStreamProcessor::SortableMediaPacket::olderThan(const SortableMediaPacket& first, const SortableMediaPacket& second)
{
    assert(first.packet.isAudio == second.packet.isAudio);

    return first.extendedPacketId < second.extendedPacketId;
    // return isNewerPacketId(second.packet.packetId, first.packet.packetId, first.packet.isAudio);
}

OoVooInboundStreamProcessor::BufferedPackets::BufferedPackets()
    : availablePayloadBuffer(nullptr)
    , packetIdWraps(0)
    , maxPacketId(-1)
{
    for (uint32_t i = 0; i < NUMBER_OF_BUFFERED_PACKETS; ++i)
        payloadBuffers[i] = new char[MAX_DATA_PACKET_SIZE];

    availablePayloadBuffer = payloadBuffers[0];
}

OoVooInboundStreamProcessor::BufferedPackets::~BufferedPackets()
{
    for (uint32_t i = 0; i < NUMBER_OF_BUFFERED_PACKETS; ++i) {
        delete[] payloadBuffers[i];
        payloadBuffers[i] = nullptr;
    }
    availablePayloadBuffer = nullptr;
}

OoVooInboundStreamProcessor::OoVooInboundStreamProcessor()
    : m_audioCodecPT(PCMU_8000_PT)
    , m_videoCodecPT(VP8_90000_PT)
    , m_rtpBuffer(nullptr)
{
}

OoVooInboundStreamProcessor::~OoVooInboundStreamProcessor()
{
    close();
}

void OoVooInboundStreamProcessor::init()
{
    assert(!m_rtpBuffer);
    m_rtpBuffer = new char[MAX_DATA_PACKET_SIZE];
}

void OoVooInboundStreamProcessor::close()
{
    delete[] m_rtpBuffer;
    m_rtpBuffer = nullptr;
}

static uint32_t writeVP8PayloadDescriptor(char* buffer, uint32_t pictureId, bool beginningOfPartition)
{
    uint32_t payloadDescriptorLength = 0;
    // 0x10 - S bit;
    // 0x80 - X bit.
    buffer[payloadDescriptorLength] = (beginningOfPartition ? (0x10 | 0x80) : 0x80);
    ++payloadDescriptorLength;
    // Set the I bit of the X field, indicating that we have a picture ID.
    buffer[payloadDescriptorLength] = 0x80;
    ++payloadDescriptorLength;
    // Now write the picture ID, which can consume one or two bytes.
    int pictureIdLength = pictureId <= 0x7F ? 1 : 2;

    if (pictureIdLength == 2) {
        buffer[payloadDescriptorLength] = 0x80 | ((pictureId >> 8) & 0x7F);
        buffer[payloadDescriptorLength + 1] = pictureId & 0xFF;
    } else if (pictureIdLength == 1)
        buffer[payloadDescriptorLength] = pictureId & 0x7F;

    payloadDescriptorLength += pictureIdLength;

    return payloadDescriptorLength;
}

// TODO: Operations on audio and video data processing are NOT thread safe.
// But the operations should only be executed in the ooVoo stream read thread.

void OoVooInboundStreamProcessor::constructAndSendRtpPacket(ProtectedRTPSender* sender, const mediaPacket_t& packet, uint32_t extendedPacketId)
{
    // Common information for audio and video
    RTPHeader* head = reinterpret_cast<RTPHeader*>(m_rtpBuffer);
    memset(head, 0, sizeof(RTPHeader));
    head->setVersion(2);
    head->setTimestamp(packet.ts);
    head->setSSRC(packet.streamId);
    head->setMarker(packet.isLastFragment);
    head->setSeqNumber(sender->produceSequenceNumberForPacket(extendedPacketId));
    // head->setSeqNumber(sender->incrementSequenceNumber());

    if (packet.isAudio) {
        head->setPayloadType(m_audioCodecPT);
        uint32_t headerLength = head->getHeaderLength();
        // Now complete the payload.
        memcpy(&m_rtpBuffer[headerLength], packet.pData, packet.length);
        sender->sendPacket(m_rtpBuffer, packet.length, headerLength, AUDIO);
    } else {
        uint32_t headerLength =
            completeRtpHeaderExtension<ENABLE_RTP_ABS_SEND_TIME_EXTENSION,
                ENABLE_RTP_TRANSMISSION_TIME_OFFSET_EXTENSION>();
        uint32_t redHeaderLength = 0;
        if (sender->encapsulatedRTPDataEnabled()) {
            // One more byte for the RED header.
            head->setPayloadType(RED_90000_PT);
            redheader* red = reinterpret_cast<redheader*>(&m_rtpBuffer[headerLength]);
            red->payloadtype = m_videoCodecPT;
            red->follow = 0;
            redHeaderLength = 1;
        } else
            head->setPayloadType(m_videoCodecPT);

        // A RTP packet should be able to hold the payload from a single ooVoo packet.
        uint32_t payloadDescriptorLength = 0;
        if (m_videoCodecPT == VP8_90000_PT)
            payloadDescriptorLength += writeVP8PayloadDescriptor(&m_rtpBuffer[headerLength + redHeaderLength], packet.frameId, packet.fragmentId == 0);

        // Now complete the payload.
        memcpy(&m_rtpBuffer[headerLength + redHeaderLength + payloadDescriptorLength], packet.pData, packet.length);
        sender->sendPacket(m_rtpBuffer, packet.length + payloadDescriptorLength + redHeaderLength, headerLength, VIDEO);

        // Protect the RTP packets by sending additional FEC packets if necessary.
        if (sender->encapsulatedRTPDataEnabled() && sender->fecEnabled()) {
            // A new frame
            if (packet.fragmentId == 0)
                sender->outgoingVideoFrame(packet.isKey);

            // Remove the RED encapsulating header, because the content we want to protect
            // is the RTP packet without the encapsulation.
            head->setPayloadType(m_videoCodecPT);
            uint32_t payloadDescriptorLength = 0;
            if (m_videoCodecPT == VP8_90000_PT)
                payloadDescriptorLength += writeVP8PayloadDescriptor(&m_rtpBuffer[headerLength], packet.frameId, packet.fragmentId == 0);
            memcpy(&m_rtpBuffer[headerLength + payloadDescriptorLength], packet.pData, packet.length);
            sender->protectVideoPacket(m_rtpBuffer, packet.length + payloadDescriptorLength, headerLength, extendedPacketId);
        }
    }
}

template<bool enableAbsSendTime, bool enableTimeOffset>
uint32_t OoVooInboundStreamProcessor::completeRtpHeaderExtension()
{
    if (enableAbsSendTime) {
        ELOG_ERROR("The ooVoo gateway doesn't support the rtp abs-send-time extension");
    }

    RTPHeader* head = reinterpret_cast<RTPHeader*>(m_rtpBuffer);
    if (enableTimeOffset) {
        // Transmission time offset Extension
        head->setExtension(1);
        head->setExtId(RTPHeader::RTP_ONE_BYTE_HEADER_EXTENSION);
        head->setExtLength(RTPExtensionTransmissionTimeOffset::SIZE / 4);

        RTPExtensionTransmissionTimeOffset* ext = reinterpret_cast<RTPExtensionTransmissionTimeOffset*>(&m_rtpBuffer[sizeof(RTPHeader)]);
        ext->init();
        assert(head->getHeaderLength() == sizeof(RTPHeader) + RTPExtensionTransmissionTimeOffset::SIZE);
    }

    return head->getHeaderLength();
}

void OoVooInboundStreamProcessor::receiveOoVooData(const mediaPacket_t& packet)
{
    if (packet.isKey) {
        ELOG_DEBUG("Received mediaPacket: %s", dumpMediaPacket(packet).c_str());
    } else {
        ELOG_TRACE("Received mediaPacket: %s", dumpMediaPacket(packet).c_str());
    }

    if (packet.length == 0) {
        // ooVoo is forwarding the packets from another WebRTC client to us,
        // which could contain empty packets caused by the FEC packets from that WebRTC client.
        ELOG_TRACE("Discard empty mediaPacket");
        return;
    }

    std::map<uint32_t, boost::shared_ptr<ProtectedRTPSender>>::iterator senderIter = m_rtpSenders.find(packet.streamId);
    if (senderIter == m_rtpSenders.end())
        return;

    ProtectedRTPSender* rtpSender = senderIter->second.get();

    std::map<uint32_t, boost::shared_ptr<BufferedPackets>>::iterator packetsIter = m_bufferedPackets.find(packet.streamId);
    if (packetsIter == m_bufferedPackets.end()) {
        m_bufferedPackets[packet.streamId].reset(new BufferedPackets);
        packetsIter = m_bufferedPackets.find(packet.streamId);
    }
    BufferedPackets* packets = packetsIter->second.get();

    SortableMediaPacketList& packetList = packets->list;
    assert(packetList.size() < NUMBER_OF_BUFFERED_PACKETS);
    SortableMediaPacket newMediaPacket;
    newMediaPacket.packet = packet;

    // Extend the packet Id to 32-bit.
    if (packets->maxPacketId == static_cast<uint32_t>(-1)) {
        // We're now receiving the first packet of this stream.
        packets->maxPacketId = packet.packetId;
        newMediaPacket.extendedPacketId = packet.packetId;
    } else {
        uint16_t packetIdWraps = packets->packetIdWraps;
        uint32_t packetId = packet.packetId;
        if (isNewerPacketId(packetId, packets->maxPacketId, packet.isAudio)) {
            if (packets->maxPacketId > packetId) {
                // This means that the packet id is wrapped.
                ++packets->packetIdWraps;
                packetIdWraps = packets->packetIdWraps;
            }
            packets->maxPacketId = packetId;
        } else if (packets->maxPacketId < packetId) {
            // This means we're receiving an older packet before the latest wrap.
            assert(packetIdWraps > 0);
            --packetIdWraps;
        }

        // The packet Id wrap thresholds are different for audio and video.
        if (packet.isAudio)
            newMediaPacket.extendedPacketId = (packetIdWraps * 0xCCCCCCC) + packetId;
        else
            newMediaPacket.extendedPacketId = (packetIdWraps << 16) + packetId;
    }

    if (NUMBER_OF_BUFFERED_PACKETS == 1) {
        // We don't need buffers at all in this case.
        // Just send it out immediately.
        constructAndSendRtpPacket(rtpSender, newMediaPacket.packet, newMediaPacket.extendedPacketId);
        return;
    }

    char*& payloadBuffer = packets->availablePayloadBuffer;
    assert(payloadBuffer);
    memcpy(payloadBuffer, packet.pData, packet.length);
    newMediaPacket.packet.pData = payloadBuffer;

    if (packetList.size() == 0)
        packetList.push_front(newMediaPacket);
    else {
        // Sort the packet list when inserting a new packet.
        // The list is sorted with the newest packet at the beginning and
        // the oldest one at the end. We choose this order because in normal
        // cases the packets arrive in-order so the newest packet can be
        // inserted before the existing ones with just one comparison.
        // Note that the "insert" method of std::list will insert the new
        // element BEFORE the element at the specified position.
        SortableMediaPacketList::iterator it;
        for (it = packetList.begin(); it != packetList.end(); ++it) {
             if (SortableMediaPacket::olderThan(*it, newMediaPacket)) {
                 packetList.insert(it, newMediaPacket);
                 break;
             }
        }
        if (it == packetList.end()) {
            // This packet is older than any existing packet in the list.
            packetList.push_back(newMediaPacket);
        }
    }

    if (packetList.size() == NUMBER_OF_BUFFERED_PACKETS) {
        // Fetch the oldest packet, construct RTP packet and send it out.
        SortableMediaPacket& sendingPacket = packetList.back();
        constructAndSendRtpPacket(rtpSender, sendingPacket.packet, sendingPacket.extendedPacketId);
        payloadBuffer = static_cast<char*>(sendingPacket.packet.pData);
        packetList.pop_back();
    } else 
        payloadBuffer = packets->payloadBuffers[packetList.size()];

}

boost::shared_ptr<ProtectedRTPSender>& OoVooInboundStreamProcessor::createRTPSender(uint32_t streamId, RTPDataReceiver* rtpReceiver)
{
    std::map<uint32_t, boost::shared_ptr<ProtectedRTPSender>>::iterator it = m_rtpSenders.find(streamId);
    if (it == m_rtpSenders.end()) {
        ProtectedRTPSender* sender = new ProtectedRTPSender(streamId, rtpReceiver);
        m_rtpSenders[streamId].reset(sender);
        return m_rtpSenders[streamId];
    }

    // There's an existing RTPSender for this stream, meaning that the stream
    // should be paused and resumed, and we need to restart the sender.
    it->second->restart();
    return it->second;
}

void OoVooInboundStreamProcessor::destroyRTPSender(uint32_t streamId)
{
    std::map<uint32_t, boost::shared_ptr<ProtectedRTPSender>>::iterator it = m_rtpSenders.find(streamId);
    if (it != m_rtpSenders.end())
        m_rtpSenders.erase(it);
}

void OoVooInboundStreamProcessor::closeStream(uint32_t streamId)
{
    std::map<uint32_t, boost::shared_ptr<BufferedPackets>>::iterator it = m_bufferedPackets.find(streamId);
    if (it != m_bufferedPackets.end())
        m_bufferedPackets.erase(it);
}

void OoVooInboundStreamProcessor::receiveOoVooSyncMsg(uint64_t ntp, uint64_t rtp, bool isAudio, uint32_t streamId)
{
    uint32_t ntp_secs;
    uint32_t ntp_frac;
    MsToNtp(ntp, ntp_secs, ntp_frac);

    SenderReport* report = reinterpret_cast<SenderReport*>(m_rtpBuffer);
    uint16_t size = sizeof(SenderReport);
    assert(size == 28);
    memset(report, 0, size);

    report->getRTCPHeader().setVersion(2);
    report->getRTCPHeader().setPacketType(RTCP_Sender_PT);
    report->getRTCPHeader().setSSRC(streamId);
    report->getRTCPHeader().setLength(size / 4 - 1);

    report->setNTPTimestampHighBits(ntp_secs);
    report->setNTPTimestampLowBits(ntp_frac);
    report->setRTPTimestamp(rtp);

    std::map<uint32_t, boost::shared_ptr<ProtectedRTPSender>>::iterator it = m_rtpSenders.find(streamId);
    if (it != m_rtpSenders.end()) {
        report->setPacketCount(it->second->packetsSent());
        report->setOctetCount(it->second->payloadBytesSent());
        ELOG_DEBUG("Created RTCP Sender Report for %s stream %u - ntp_timestamp_hi: %u, ntp_timestamp_lo: %u, rtp_timestamp: %u, packet count %u, octet count %u", isAudio ? "AUDIO" : "VIDEO", streamId, ntp_secs, ntp_frac, static_cast<uint32_t>(rtp), report->getPacketCount(), report->getOctetCount());
        it->second->sendSenderReport(m_rtpBuffer, size, isAudio ? AUDIO : VIDEO);
    }
}

} /* namespace oovoo_gateway */
