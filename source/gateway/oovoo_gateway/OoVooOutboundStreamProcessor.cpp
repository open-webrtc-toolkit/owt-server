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

#include "OoVooOutboundStreamProcessor.h"

#include <assert.h>
#include <rtp/RtpVP8Parser.h>
#include <rtputils.h>

using namespace erizo;

namespace oovoo_gateway {

DEFINE_LOGGER(OoVooOutboundStreamProcessor, "ooVoo.OoVooOutboundStreamProcessor");

OoVooOutboundStreamProcessor::OoVooOutboundStreamProcessor()
    : m_ooVooReceiver(nullptr)
{
    m_rtpHeaderParser.reset(webrtc::RtpHeaderParser::Create());
}

OoVooOutboundStreamProcessor::~OoVooOutboundStreamProcessor()
{
}

void OoVooOutboundStreamProcessor::init(OoVooDataReceiver* receiver)
{
    m_ooVooReceiver = receiver;
}

void OoVooOutboundStreamProcessor::processRtcpData(char* buf, int len, bool isAudio)
{
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT) { // Sender Report
        SenderReport* report = reinterpret_cast<SenderReport*>(buf);
        uint32_t ssrc = report->getRTCPHeader().getSSRC();
        boost::shared_lock<boost::shared_mutex> lock(m_ssrcStreamIdMapMutex);
        std::map<uint32_t, uint32_t>::iterator it = m_ssrcStreamIdMap.find(ssrc);
        if (it != m_ssrcStreamIdMap.end()) {
            uint32_t ntp_timestamp_hi = report->getNTPTimestampHighBits();
            uint32_t ntp_timestamp_lo = report->getNTPTimestampLowBits();
            uint32_t rtp_timestamp = report->getRTPTimestamp();
            ELOG_DEBUG("Received RTCP Sender Report for %s stream %u - ntp_timestamp_hi: %u, ntp_timestamp_lo: %u, rtp_timestamp: %u", isAudio ? "AUDIO" : "VIDEO", (*it).second, ntp_timestamp_hi, ntp_timestamp_lo, rtp_timestamp);
            m_ooVooReceiver->receiveOoVooSyncMsg(NtpToMs(ntp_timestamp_hi, ntp_timestamp_lo), rtp_timestamp, isAudio, (*it).second);
        }
    }
}

void OoVooOutboundStreamProcessor::processAudioData(char* buf, int len)
{
    webrtc::RTPHeader header;
    if (!m_rtpHeaderParser->Parse(reinterpret_cast<uint8_t*>(buf), len, &header)) {
        ELOG_WARN("Not a valid RTP packet; ignoring.");
        return;
    }

    if (header.payloadType != PCMU_8000_PT) {
        ELOG_TRACE("PT AUDIO %d", header.payloadType);
    }

    boost::shared_lock<boost::shared_mutex> streamIdLock(m_ssrcStreamIdMapMutex);
    std::map<uint32_t, uint32_t>::iterator it = m_ssrcStreamIdMap.find(header.ssrc);
    if (it == m_ssrcStreamIdMap.end())
        return;
    uint32_t streamId = it->second;
    streamIdLock.unlock();

    boost::lock_guard<boost::mutex> lock(m_audioMutex);

    m_audioPacket.streamId = streamId;
    m_audioPacket.pData = buf + header.headerLength;
    m_audioPacket.length = len - header.headerLength;
    m_audioPacket.ts = header.timestamp;
    m_audioPacket.packetId = header.sequenceNumber;
    m_audioPacket.isLastFragment = !!header.markerBit;
    m_audioPacket.isAudio = true;
    m_audioPacket.frameId = 0;
    m_audioPacket.fragmentId = 0;
    m_audioPacket.resolution = VideoResolutionType::unknown;
    m_audioPacket.isKey = false;
    m_audioPacket.isMirrored = false;
    m_audioPacket.rotation = 0;

    ELOG_TRACE("Created mediaPacket: %s", dumpMediaPacket(m_audioPacket).c_str());

    m_ooVooReceiver->receiveOoVooData(m_audioPacket);
}

void OoVooOutboundStreamProcessor::processVideoData(char* buf, int len)
{
    webrtc::RTPHeader header;
    if (!m_rtpHeaderParser->Parse(reinterpret_cast<uint8_t*>(buf), len, &header)) {
        ELOG_WARN("Not a valid RTP packet; ignoring.");
        return;
    }

    assert(header.payloadType == VP8_90000_PT);

    boost::shared_lock<boost::shared_mutex> streamIdLock(m_ssrcStreamIdMapMutex);
    std::map<uint32_t, uint32_t>::iterator it = m_ssrcStreamIdMap.find(header.ssrc);
    if (it == m_ssrcStreamIdMap.end())
        return;
    uint32_t streamId = it->second;
    streamIdLock.unlock();

    int inBuffOffset = header.headerLength;

    boost::lock_guard<boost::mutex> lock(m_videoMutex);

    AuxiliaryPacketInfo& auxPacketInfo = getAuxiliaryPacketInfo(streamId);
    // The information which we cannot provide now or is determined w/o the need to parse the packet
    // m_videoPacket.resolution = VideoResolutionType::unknown;
    m_videoPacket.resolution = auxPacketInfo.resolution;
    m_videoPacket.isAudio = false;
    m_videoPacket.isMirrored = false;
    m_videoPacket.rotation = 0;

    // The information which can be gotten from the RTP header
    m_videoPacket.streamId = streamId;
    m_videoPacket.ts = header.timestamp;
    // Convention from ooVoo: isLastFragment is used to mark if current packet is
    // the last packet in its frame. The WebRTC RTP packet uses the marker for
    // frame boundaries.
    m_videoPacket.isLastFragment = !!header.markerBit;
    m_videoPacket.packetId = header.sequenceNumber;

    // The information which can only be gotten by parsing the VP8 payload descriptor and header
    if (len > inBuffOffset) {
        RTPPayloadVP8 parsed;
        RtpVP8Parser::parseVP8(reinterpret_cast<unsigned char*>(buf + inBuffOffset), len - inBuffOffset, &parsed);

        // This is UGLY!
        m_videoPacket.pData = const_cast<void*>(reinterpret_cast<const void*>(parsed.data));
        m_videoPacket.length = parsed.dataLength;
        m_videoPacket.isKey = (parsed.frameType == kIFrame);

        // The fragmentId marks the position of current packet in the frame.
        if (parsed.beginningOfPartition && parsed.partitionID == 0)
            auxPacketInfo.fragmentId = 0;
        m_videoPacket.fragmentId = auxPacketInfo.fragmentId++;

        if (parsed.hasPictureID) {
            m_videoPacket.frameId = parsed.pictureID;
            auxPacketInfo.frameId = parsed.pictureID;
        } else {
            // If the RTP packet doesn't provide the picture ID, we simply count the
            // frameId ourselves. NOTE it's NOT accurate as UDP doesn't guarantee the
            // packets arrive in order. The limitation applies to fragmentId settings.
            m_videoPacket.frameId = auxPacketInfo.frameId;
            if (header.markerBit)
                ++auxPacketInfo.frameId; // Monotonically increasing
        }
    } else {
        // The payload length of this RTP packet is zero. It can be passed down
        // after a FEC packet from the browser has been processed.
        m_videoPacket.pData = nullptr;
        m_videoPacket.length = 0;
        m_videoPacket.isKey = 0;
        m_videoPacket.fragmentId = 0;
        m_videoPacket.frameId = 0;
    }

    if (m_videoPacket.isKey) {
        ELOG_DEBUG("Created mediaPacket: %s", dumpMediaPacket(m_videoPacket).c_str());
    } else {
        ELOG_TRACE("Created mediaPacket: %s", dumpMediaPacket(m_videoPacket).c_str());
    }

    m_ooVooReceiver->receiveOoVooData(m_videoPacket);
}

void OoVooOutboundStreamProcessor::receiveRtpData(char* buf, int len, erizo::DataType dataType, uint32_t)
{
    if (webrtc::RtpHeaderParser::IsRtcp(reinterpret_cast<uint8_t*>(buf), len)) {
        processRtcpData(buf, len, dataType == AUDIO);
        return;
    }

    switch (dataType) {
    case VIDEO:
        processVideoData(buf, len);
        break;
    case AUDIO:
        processAudioData(buf, len);
        break;
    default:
        break;
    }
}

void OoVooOutboundStreamProcessor::setVideoStreamResolution(uint32_t streamId, VideoResolutionType resolution)
{
    ELOG_DEBUG("Setting video stream %u resolution type to %d", streamId, resolution);

    boost::lock_guard<boost::mutex> lock(m_videoMutex);
    AuxiliaryPacketInfo& auxPacketInfo = getAuxiliaryPacketInfo(streamId);
    auxPacketInfo.resolution = resolution;
}

bool OoVooOutboundStreamProcessor::mapSsrcToStreamId(uint32_t ssrc, uint32_t streamId)
{
    // In practice, there should be no contention though we place a lock here
    // because the map is established in one thread only if we strictly follow
    // ooVoo API conventions, and it's before media packets are sent out.
    boost::lock_guard<boost::shared_mutex> lock(m_ssrcStreamIdMapMutex);

    if (m_ssrcStreamIdMap.find(ssrc) != m_ssrcStreamIdMap.end()) {
        ELOG_WARN("The SSRC %u is already mapped; don't remap it to %u", ssrc, streamId);
        return false;
    }

    m_ssrcStreamIdMap[ssrc] = streamId;
    return true;
}

uint32_t OoVooOutboundStreamProcessor::unmapSsrc(uint32_t ssrc)
{
    uint32_t streamId = NOT_A_OOVOO_STREAM_ID;
    boost::lock_guard<boost::shared_mutex> lock(m_ssrcStreamIdMapMutex);

    std::map<uint32_t, uint32_t>::iterator it = m_ssrcStreamIdMap.find(ssrc);
    if (it != m_ssrcStreamIdMap.end()) {
        streamId = it->second;
        m_ssrcStreamIdMap.erase(it);
    } else {
        ELOG_WARN("The SSRC %u is not mapped yet", ssrc);
    }

    return streamId;
}

OoVooOutboundStreamProcessor::AuxiliaryPacketInfo& OoVooOutboundStreamProcessor::getAuxiliaryPacketInfo(uint32_t streamId)
{
    std::map<uint32_t, AuxiliaryPacketInfo>::iterator it = m_auxPacketInfoMap.find(streamId);
    if (it == m_auxPacketInfoMap.end()) {
        // The initial value of fragmentId (the second field) is important.
        // We will have more things for fragmentId 0 which means the packet is
        // the beginning of a partition of a frame. If we make the initial value 0,
        // but the first packet we receive happens to be NOT a beginning packet, and
        // we further forward this packet to AVS and another WebRTC or ooVoo client,
        // the client may get errors when trying to decode a frame starting from
        // this packet - even worse, the client may crash if the frame this packet
        // belongs to is a key frame, like the libvpx library Chrome 36 which has
        // a bug by not doing sanity checking in some place by accident, bug & fix -
        // https://github.com/WebRTC-Labs/libvpx/commit/caba78ef49e8cc2250ea79c2d21e5e3710354199
        //
        // So for safety reason, we make the initial fragmentId 1, and if the first received
        // packet is a beginning packet, we can get the information from the RTP VP8
        // payload descriptor and reset it to 0. So this won't bring a problem.
        // The only problem is that the fragmentId may not be accurate for a non-beginning
        // packet, because neither RTP or VP8 payload descriptor/header provides such
        // information, instead the decoder should contruct frames based on the beginning
        // packet, the frameId (pictureId) or the marker bit, and the packetId.
        // We need to ask ooVoo whether fragmentId for non-beginning packets is important
        // or not. WebRTC doesn't have such information directly but the decoder will
        // deduce it from the other information I mentioned above. ooVoo should also be
        // able to do that.
        //
        AuxiliaryPacketInfo packetInfo = {0, 1, unknown};
        m_auxPacketInfoMap[streamId] = packetInfo;
        return m_auxPacketInfoMap[streamId];
    }

    return (*it).second;
}

} /* namespace oovoo_gateway */
