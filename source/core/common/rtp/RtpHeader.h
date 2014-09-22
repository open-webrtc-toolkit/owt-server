/*
 * RtpHeader.h
 *
 *  Created on: Sep 20, 2012
 *      Author: pedro
 */

#ifndef RTPHEADER_H_
#define RTPHEADER_H_

#include <netinet/in.h>

//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                           timestamp                           |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |           synchronization source (SSRC) identifier            |
//   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//   |            contributing source (CSRC) identifiers             |
//   |                             ....                              |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

//   The first twelve octets are present in every RTP packet, while the
//   list of CSRC identifiers is present only when inserted by a mixer.
//   The fields have the following meaning:

//   version (V): 2 bits
//        This field identifies the version of RTP. The version defined by
//        this specification is two (2). (The value 1 is used by the first
//        draft version of RTP and the value 0 is used by the protocol
//        initially implemented in the "vat" audio tool.)

//   padding (P): 1 bit
//        If the padding bit is set, the packet contains one or more
//        additional padding octets at the end which are not part of the
//      payload. The last octet of the padding contains a count of how
//      many padding octets should be ignored. Padding may be needed by
//      some encryption algorithms with fixed block sizes or for
//      carrying several RTP packets in a lower-layer protocol data
//      unit.

// extension (X): 1 bit
//      If the extension bit is set, the fixed header is followed by
//      exactly one header extension, with a format defined in Section
//      5.3.1.

// CSRC count (CC): 4 bits
//      The CSRC count contains the number of CSRC identifiers that
//      follow the fixed header.

// marker (M): 1 bit
//      The interpretation of the marker is defined by a profile. It is
//      intended to allow significant events such as frame boundaries to
//      be marked in the packet stream. A profile may define additional
//      marker bits or specify that there is no marker bit by changing
//      the number of bits in the payload type field (see Section 5.3).

// payload type (PT): 7 bits
//      This field identifies the format of the RTP payload and
//      determines its interpretation by the application. A profile
//      specifies a default static mapping of payload type codes to
//      payload formats. Additional payload type codes may be defined
//      dynamically through non-RTP means (see Section 3). An initial
//      set of default mappings for audio and video is specified in the
//      companion profile Internet-Draft draft-ietf-avt-profile, and
//      may be extended in future editions of the Assigned Numbers RFC
//      [6].  An RTP sender emits a single RTP payload type at any given
//      time; this field is not intended for multiplexing separate media
//      streams (see Section 5.2).

// sequence number: 16 bits
//      The sequence number increments by one for each RTP data packet
//      sent, and may be used by the receiver to detect packet loss and
//      to restore packet sequence. The initial value of the sequence
//      number is random (unpredictable) to make known-plaintext attacks
//      on encryption more difficult, even if the source itself does not
//      encrypt, because the packets may flow through a translator that
//      does. Techniques for choosing unpredictable numbers are
//      discussed in [7].

// timestamp: 32 bits
//      The timestamp reflects the sampling instant of the first octet
//      in the RTP data packet. The sampling instant must be derived
//      from a clock that increments monotonically and linearly in time
//      to allow synchronization and jitter calculations (see Section
//      6.3.1).  The resolution of the clock must be sufficient for the
//      desired synchronization accuracy and for measuring packet
//      arrival jitter (one tick per video frame is typically not
//      sufficient).  The clock frequency is dependent on the format of
//      data carried as payload and is specified statically in the
//      profile or payload format specification that defines the format,
//      or may be specified dynamically for payload formats defined
//      through non-RTP means. If RTP packets are generated
//      periodically, the nominal sampling instant as determined from
//      the sampling clock is to be used, not a reading of the system
//      clock. As an example, for fixed-rate audio the timestamp clock
//      would likely increment by one for each sampling period.  If an
//      audio application reads blocks covering 160 sampling periods
//      from the input device, the timestamp would be increased by 160
//      for each such block, regardless of whether the block is
//      transmitted in a packet or dropped as silent.

// The initial value of the timestamp is random, as for the sequence
// number. Several consecutive RTP packets may have equal timestamps if
// they are (logically) generated at once, e.g., belong to the same
// video frame. Consecutive RTP packets may contain timestamps that are
// not monotonic if the data is not transmitted in the order it was
// sampled, as in the case of MPEG interpolated video frames. (The
// sequence numbers of the packets as transmitted will still be
// monotonic.)

// SSRC: 32 bits
//      The SSRC field identifies the synchronization source. This
//      identifier is chosen randomly, with the intent that no two
//      synchronization sources within the same RTP session will have
//      the same SSRC identifier. An example algorithm for generating a
//      random identifier is presented in Appendix A.6. Although the
//      probability of multiple sources choosing the same identifier is
//      low, all RTP implementations must be prepared to detect and
//      resolve collisions.  Section 8 describes the probability of
//      collision along with a mechanism for resolving collisions and
//      detecting RTP-level forwarding loops based on the uniqueness of
//      the SSRC identifier. If a source changes its source transport
//      address, it must also choose a new SSRC identifier to avoid
//      being interpreted as a looped source.

// 0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |      defined by profile       |           length              |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                        header extension                       |
// |                             ....                              |

class RTPHeader {
public:
  // constants

  /**
   * KSize
   * Longitud de la cabecera en bytes.
   */
  static const int MIN_SIZE = 12;
  static const int RTP_ONE_BYTE_HEADER_EXTENSION = 0xBEDE;

public:
  // Constructor
  inline RTPHeader()
    : cc_(0), extension_(0), padding_(0), version_(2), payloadType_(0), marker_(0)
    , seqNumber_(0), timestamp_(0), ssrc_(0), extId_(0), extLength_(0) {
    // No implementation required
  }

public:
  // Member functions

  /**
   * Get the marker bit from the RTP header.
   * @return 1 if marker bit is set 0 if is not set.
   */
  inline uint8_t getMarker() const {
    return marker_;
  }

  /**
   * Set the marker bit from the RTP header.
   * @param marker 1 to set marker bit, 0 to unset it.
   */
  inline void setMarker(uint8_t marker) {
    marker_ = marker;
  }

  /**
   * Get the extension bit from the RTP header.
   * @return 1 if extension bit is set 0 if is not set.
   */
  inline uint8_t getExtension() const {
    return extension_;
  }

  /**
   * Set the extension bit from the RTP header
   * @param ext 1 to set extension bit, 0 to unset i
   */
  inline void setExtension(uint8_t ext) {
    extension_ = ext;
  }

  inline uint8_t hasPadding() const {
    return padding_;
  }

  /**
   * Get the version bits from the RTP header.
   * @return the version number (2).
   */
  inline uint8_t getVersion() const {
    return version_;
  }

  /**
   * Set the version bits in the RTP header
   * @param ver the version number (2).
   */
  inline void setVersion(uint8_t ver) {
    version_ = ver;
  }

  /**
   * Get the payload type from the RTP header.
   * @return A TInt8 holding the value.
   */
  inline uint8_t getPayloadType() const {
    return payloadType_;
  }

  /**
   * Set the payload type from the RTP header.
   * @param aType the payload type. Valid range between 0x00 to 0x7F
   */
  inline void setPayloadType(uint8_t payloadType) {
    payloadType_ = payloadType;
  }

  /**
   * Get the sequence number field from the RTP header.
   * @return A TInt16 holding the value.
   */
  inline uint16_t getSeqNumber() const {
    return ntohs(seqNumber_);
  }

  /**
   * Set the seq number from the RTP header.
   * @param seqNumber The seq number. Valid range between 0x0000 to 0xFFFF
   */
  inline void setSeqNumber(uint16_t seqNumber) {
    seqNumber_ = htons(seqNumber);
  }

  /**
   * Get the Timestamp field from the RTP header.
   * @return A TInt32 holding the value.
   */
  inline uint32_t getTimestamp() const {
    return ntohl(timestamp_);
  }

  /**
   * Set the Timestamp from the RTP header.
   * @param timestamp The Tmestamp. Valid range between 0x00000000 to 0xFFFFFFFF
   */
  inline void setTimestamp(uint32_t timestamp) {
    timestamp_ = htonl(timestamp);
  }

  /**
   * Get the SSRC field from the RTP header.
   * @return A TInt32 holding the value.
   */
  inline uint32_t getSSRC() const {
    return ntohl(ssrc_);
  }

  /**
   * Set the SSRC from the RTP header.
   * @param ssrc The SSRC. Valid range between 0x00000000 to 0xFFFFFFFF
   */
  inline void setSSRC(uint32_t ssrc) {
    ssrc_ = htonl(ssrc);
  }
  /**
   * Get the extenstion id field from the RTP header.
   * @return A TInt16 holding the value.
   */
  inline uint16_t getExtId() const {
    return ntohs(extId_);
  }

  /**
   * Set the extension id in the RTP header.
   * @param extensionId The extension id.
   */
  inline void setExtId(uint16_t extensionId) {
    extId_ = htons(extensionId);
  }

  /**
   * Get the extension length field from the RTP header.
   * @return A TInt16 holding the value.
   */
  inline uint16_t getExtLength() const {
    return ntohs(extLength_);
  }

  /**
   * Set the extension length in the RTP header.
   * @param extensionLength The extension length.
   */
  inline void setExtLength(uint16_t extensionLength) {
    extLength_ = htons(extensionLength);
  }

  /**
   * Get the RTP header length
   * @return the length in 8 bit units
   */
  inline int getHeaderLength() {
    return MIN_SIZE + cc_ * 4 + extension_ * (4 + ntohs(extLength_) * 4);
  }

private:
  uint8_t cc_ :4;
  uint8_t extension_ :1;
  uint8_t padding_ :1;
  uint8_t version_ :2;
  uint8_t payloadType_ :7;
  uint8_t marker_ :1;
  uint16_t seqNumber_;
  uint32_t timestamp_;
  uint32_t ssrc_;
  //uint32_t csrc[3];
  uint16_t extId_;
  uint16_t extLength_;

};

#endif /* RTPHEADER_H_ */
