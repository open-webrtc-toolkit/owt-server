/*
 * rtputils.h
 */

#ifndef RTPUTILS_H_
#define RTPUTILS_H_

#include <netinet/in.h>

#include "rtp/Rtcp.h"
#include "rtp/RtpHeader.h"
#include "rtp/RtpHeaderExt.h"

//     0                   1                    2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3  4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |F|   block PT  |  timestamp offset         |   block length    |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//
// RFC 2198          RTP Payload for Redundant Audio Data    September 1997
//
//    The bits in the header are specified as follows:
//
//    F: 1 bit First bit in header indicates whether another header block
//        follows.  If 1 further header blocks follow, if 0 this is the
//        last header block.
//        If 0 there is only 1 byte RED header
//
//    block PT: 7 bits RTP payload type for this block.
//
//    timestamp offset:  14 bits Unsigned offset of timestamp of this block
//        relative to timestamp given in RTP header.  The use of an unsigned
//        offset implies that redundant data must be sent after the primary
//        data, and is hence a time to be subtracted from the current
//        timestamp to determine the timestamp of the data for which this
//        block is the redundancy.
//
//    block length:  10 bits Length in bytes of the corresponding data
//        block excluding header.
struct redheader {
    uint32_t payloadtype :7;
    uint32_t follow :1;
    uint32_t tsLength :24;
    uint32_t getTS() {
        return (ntohl(tsLength) & 0xfffc00) >> 10;
    }
    uint32_t getLength() {
        return (ntohl(tsLength) & 0x3ff);
    }
};

// Payload types
#define RTCP_Sender_PT      200 // RTCP Sender Report
#define RTCP_Receiver_PT    201 // RTCP Receiver Report
#define RTCP_SDES_PT        202
#define RTCP_BYE            203
#define RTCP_APP            204
#define RTCP_RTP_Feedback_PT 205 // RTCP Transport Layer Feedback Packet
#define RTCP_PS_Feedback_PT    206 // RTCP Payload Specific Feedback Packet

#define RTCP_PLI_FMT        1
#define RTCP_SLI_FMT        2
#define RTCP_FIR_FMT        4
#define RTCP_AFB            15

#define VP8_90000_PT        100 // VP8 Video Codec
#define H264_90000_PT		127 // H264 Video Codec
#define RED_90000_PT        116 // REDundancy (RFC 2198)
#define ULP_90000_PT        117 // ULP/FEC
#define ISAC_16000_PT       103 // ISAC Audio Codec
#define ISAC_32000_PT       104 // ISAC Audio Codec
#define PCMU_8000_PT        0   // PCMU Audio Codec
#define OPUS_48000_PT       120 // Opus Audio Codec
#define PCMA_8000_PT        8   // PCMA Audio Codec
#define CN_8000_PT          13  // CN Audio Codec
#define CN_16000_PT         105 // CN Audio Codec
#define CN_32000_PT         106 // CN Audio Codec
#define CN_48000_PT         107 // CN Audio Codec
#define TEL_8000_PT         126 // Tel Audio Events

#define INVALID_PT          -1  // Not a valid PT

static const bool ENABLE_RTP_TRANSMISSION_TIME_OFFSET_EXTENSION = 0;
static const bool ENABLE_RTP_ABS_SEND_TIME_EXTENSION = 0;

inline bool isFeedback(char* buf) {
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*> (buf);
    uint8_t packetType = chead->getPacketType();
    return (packetType == RTCP_Receiver_PT ||
            packetType == RTCP_PS_Feedback_PT ||
            packetType == RTCP_RTP_Feedback_PT);
}

inline bool isRTCP(char* buf) {
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*> (buf);
    uint8_t packetType = chead->getPacketType();
    return (packetType == RTCP_Sender_PT ||
            packetType == RTCP_APP ||
            isFeedback(buf));
  }

#endif /* RTPUTILS_H */
