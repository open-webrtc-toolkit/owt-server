/*
 * Rtcp.h
 */

#ifndef RTCP_H_
#define RTCP_H_

#include <netinet/in.h>

//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |V=2|P|    RC   |   PT=RR=201   |             length            | header
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                     SSRC of packet sender                     |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                 SSRC_1 (SSRC of first source)                 | report
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
// | fraction lost |       cumulative number of packets lost       |   1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |           extended highest sequence number received           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      interarrival jitter                      |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                         last SR (LSR)                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                   delay since last SR (DLSR)                  |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                 SSRC_2 (SSRC of second source)                | report
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
// :                               ...                             :   2
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                  profile-specific extensions                  |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class RTCPHeader {
public:
  // Constructor
  inline RTCPHeader()
    : rc_or_fmt_(0), padding_(0), version_(2), packetType_(0)
    , length_(0),  ssrc_(0) {
  }

public:
  // Member functions

  inline uint8_t getRCOrFMT() const {
    return rc_or_fmt_;
  }

  inline void setRCOrFMT(uint8_t rc_or_fmt) {
    rc_or_fmt_ = rc_or_fmt;
  }

  inline uint8_t getVersion() const {
    return version_;
  }

  inline void setVersion(uint8_t ver) {
    version_ = ver;
  }

  inline uint8_t getPacketType() const {
    return packetType_;
  }

  inline void setPacketType(uint8_t packetType) {
    packetType_ = packetType;
  }

  inline uint16_t getLength() const {
    return ntohs(length_);
  }

  inline void setLength(uint16_t length) {
    length_ = htons(length);
  }

  inline uint32_t getSSRC() const {
    return ntohl(ssrc_);
  }

  inline void setSSRC(uint32_t ssrc) {
    ssrc_ = htonl(ssrc);
  }

private:
  uint8_t rc_or_fmt_ :5;
  uint8_t padding_ :1;
  uint8_t version_ :2;
  uint8_t packetType_;
  uint16_t length_;
  uint32_t ssrc_;
};

class ReportBlock {
public:
  // Constructor
  inline ReportBlock()
    : sourceSsrc_(0), fractionLost_(0), highestSeqNumber_(0), jitter_(0)
    , lsr_(0), dlsr_(0) {
  }

public:
  // Member functions

  inline uint32_t getSourceSSRC() const {
    return ntohl(sourceSsrc_);
  }

  inline void setSourceSSRC(uint32_t sourceSsrc) {
    sourceSsrc_ = htonl(sourceSsrc);
  }

  inline uint8_t getFractionLost() const {
    return fractionLost_;
  }

  inline void setFractionLost(uint8_t fracLost) {
    fractionLost_ = fracLost;
  }

  inline uint32_t getCumulativeLost() const {
    return (cumulativeLost_[0] << 16) + (cumulativeLost_[1] << 8) + cumulativeLost_[2];
  }

  inline void setCumulativeLost(uint32_t cumulativeLost) {
    cumulativeLost_[0] = (cumulativeLost >> 16) & 0xff;
    cumulativeLost_[1] = (cumulativeLost >> 8) & 0xff;
    cumulativeLost_[2] = cumulativeLost & 0xff;
  }

  inline uint32_t getHighestSeqNumber() const {
    return ntohl(highestSeqNumber_);
  }

  inline void setHighestSeqNumber(uint32_t seqNumber) {
    highestSeqNumber_ = htonl(seqNumber);
  }

  inline uint32_t getJitter() const {
    return ntohl(jitter_);
  }

  inline void setJitter(uint32_t jitter) {
    jitter_ = htonl(jitter);
  }

  inline uint32_t getLSR() const {
    return ntohl(lsr_);
  }

  inline void setLSR(uint32_t lsr) {
    lsr_ = htonl(lsr);
  }

  inline uint32_t getDLSR() const {
    return ntohl(dlsr_);
  }

  inline void setDLSR(uint32_t dlsr) {
    dlsr_ = htonl(dlsr);
  }

private:
  uint32_t sourceSsrc_;
  uint8_t fractionLost_;
  uint8_t cumulativeLost_[3];
  uint32_t highestSeqNumber_;
  uint32_t jitter_;
  uint32_t lsr_;
  uint32_t dlsr_;
};

//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |V=2|P|   FMT   |       PT      |          length               |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                  SSRC of packet sender                        |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                  SSRC of media source                         |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   :            Feedback Control Information (FCI)                 :
//   :                                                               :

//   The Feedback Control Information (FCI) for the Full Intra Request
//   consists of one or more FCI entries, the content of which is depicted
//   in Figure 4.  The length of the FIR feedback message MUST be set to
//   2+2*N, where N is the number of FCI entries.
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                              SSRC                             |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   | Seq nr.       |    Reserved                                   |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


class RTCPFeedbackHeader {
public:
  // Constructor
  inline RTCPFeedbackHeader()
    : sourceSsrc_(0) {
  }

public:
  // Member functions

  inline RTCPHeader& getRTCPHeader() {
    return rtcp_;
  }

  inline uint32_t getSourceSSRC() const {
    return ntohl(sourceSsrc_);
  }

  inline void setSourceSSRC(uint32_t sourceSsrc) {
    sourceSsrc_ = htonl(sourceSsrc);
  }

private:
  RTCPHeader rtcp_;
  uint32_t sourceSsrc_;
};

//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |            PID                |             BLP               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// RFC 4585 6.2.1 Generic NACK

class GenericNACK {
public:
  // Constructor
  inline GenericNACK()
    : packetId_(0)
    , bitMask_(0) {
  }

public:
  // Member functions

  inline uint16_t getPacketId() const {
    return ntohs(packetId_);
  }

  inline void setPacketId(uint16_t packetId) {
    packetId_ = htons(packetId);
  }

  inline uint16_t getBitMask() const {
    return ntohs(bitMask_);
  }

  inline void setBitMask(uint16_t bitMask) {
    bitMask_ = htons(bitMask);
  }

private:
  uint16_t packetId_;
  uint16_t bitMask_;
};

//         0                   1                   2                   3
//         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    RC   |   PT=SR=200   |             length            |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         SSRC of sender                        |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// sender |              NTP timestamp, most significant word             |
// info   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |             NTP timestamp, least significant word             |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         RTP timestamp                         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                     sender's packet count                     |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                      sender's octet count                     |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_1 (SSRC of first source)                 |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   1    | fraction lost |       cumulative number of packets lost       |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |           extended highest sequence number received           |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                      interarrival jitter                      |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         last SR (LSR)                         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                   delay since last SR (DLSR)                  |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_2 (SSRC of second source)                |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   2    :                               ...                             :
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//        |                  profile-specific extensions                  |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// RFC 3550 6.4.1 SR: Sender Report RTCP Packet

class SenderReport {
public:
  // Constructor
  inline SenderReport()
    : ntp_timestamp_hi_(0), ntp_timestamp_lo_(0), rtp_timestamp_(0)
    , packetCount_(0), octetCount_(0) {
  }

public:
  // Member functions

  inline RTCPHeader& getRTCPHeader() {
    return rtcp_;
  }

  inline uint32_t getNTPTimestampHighBits() const {
    return ntohl(ntp_timestamp_hi_);
  }

  inline void setNTPTimestampHighBits(uint32_t ntp_timestamp_hi) {
    ntp_timestamp_hi_ = htonl(ntp_timestamp_hi);
  }

  inline uint32_t getNTPTimestampLowBits() const {
    return ntohl(ntp_timestamp_lo_);
  }

  inline void setNTPTimestampLowBits(uint32_t ntp_timestamp_lo) {
    ntp_timestamp_lo_ = htonl(ntp_timestamp_lo);
  }

  inline uint32_t getRTPTimestamp() const {
    return ntohl(rtp_timestamp_);
  }

  inline void setRTPTimestamp(uint32_t rtp_timestamp) {
    rtp_timestamp_ = htonl(rtp_timestamp);
  }

  inline uint32_t getPacketCount() const {
    return ntohl(packetCount_);
  }

  inline void setPacketCount(uint32_t packetCount) {
    packetCount_ = htonl(packetCount);
  }

  inline uint32_t getOctetCount() const {
    return ntohl(octetCount_);
  }

  inline void setOctetCount(uint32_t octetCount) {
    octetCount_ = htonl(octetCount);
  }

private:
  RTCPHeader rtcp_;
  uint32_t ntp_timestamp_hi_;
  uint32_t ntp_timestamp_lo_;
  uint32_t rtp_timestamp_;
  uint32_t packetCount_;
  uint32_t octetCount_;
};

#endif /* RTCP_H_ */
