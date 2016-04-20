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

#ifndef OoVooInboundStreamProcessor_h
#define OoVooInboundStreamProcessor_h

#include "OoVooProtocolHeader.h"
#include "OoVooStreamProcessorHelper.h"

#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <ProtectedRTPSender.h>

namespace oovoo_gateway {

static const uint32_t NUMBER_OF_BUFFERED_PACKETS = 1;

class OoVooInboundStreamProcessor : public OoVooDataReceiver {
    DECLARE_LOGGER();
public:
    OoVooInboundStreamProcessor();
    virtual ~OoVooInboundStreamProcessor();

    void receiveOoVooData(const mediaPacket_t&);
    void receiveOoVooSyncMsg(uint64_t ntp, uint64_t rtp, bool isAudio, uint32_t streamId);

    void init();
    void setAudioCodecPayloadType(int payloadType) { m_audioCodecPT = payloadType; }
    void setVideoCodecPayloadType(int payloadType) { m_videoCodecPT = payloadType; }

    // Create a RTP sender to send the constructed RTP packets for the specified stream.
    // If there's an existing one for that stream, just reuse it.
    boost::shared_ptr<woogeen_base::ProtectedRTPSender>& createRTPSender(uint32_t streamId, erizo::RTPDataReceiver* rtpReceiver);
    // Destroy the RTP sender for the specified stream.
    void destroyRTPSender(uint32_t streamId);

    // Close the specified stream, which means it will reset the packet buffer for that stream.
    void closeStream(uint32_t streamId);

private:
    class SortableMediaPacket {
    public:
        static bool olderThan(const SortableMediaPacket&, const SortableMediaPacket&);

        mediaPacket_t packet;
        uint32_t extendedPacketId;
    };

    typedef std::list<SortableMediaPacket> SortableMediaPacketList;

    class BufferedPackets {
    public:
        BufferedPackets();
        ~BufferedPackets();

        SortableMediaPacketList list;
        char* payloadBuffers[NUMBER_OF_BUFFERED_PACKETS];
        char* availablePayloadBuffer;
        uint16_t packetIdWraps;
        uint32_t maxPacketId;
    };

    void close();

    void constructAndSendRtpPacket(woogeen_base::ProtectedRTPSender*, const mediaPacket_t&, uint32_t extendedPacketId);
    template<bool enableAbsSendTime, bool enableTimeOffset>
    uint32_t completeRtpHeaderExtension();

    std::map<uint32_t, boost::shared_ptr<woogeen_base::ProtectedRTPSender>> m_rtpSenders;
    std::map<uint32_t, boost::shared_ptr<BufferedPackets>> m_bufferedPackets;
    int m_audioCodecPT;
    int m_videoCodecPT;
    char* m_rtpBuffer;
};

} /* namespace oovoo_gateway */

#endif /* OoVooInboundStreamProcessor_h */
