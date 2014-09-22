/*
 * RtpHeaderExt.h
 */

#ifndef RTPHEADEREXT_H_
#define RTPHEADEREXT_H_

#include <netinet/in.h>

class RTPExtensionTransmissionTimeOffset {
public:
  // constants

  static const int SIZE = 4;

public:
  // Constructor
  inline RTPExtensionTransmissionTimeOffset() {
    init();
  }

public:
  // Member functions

  void init() {
    length_ = 2;
    id_ = 2;
    payload_[0] = 0;
    payload_[1] = 0;
    payload_[2] = 0;
  }

  uint8_t getId() const {
    return id_;
  }

  void setId(uint8_t id) {
    id_ = id;
  }

  uint8_t getLength() const {
    return length_;
  }

  uint32_t getPayload() const {
    return (payload_[0] << 16) + (payload_[1] << 8) + payload_[2];
  }

  void setPayload(uint32_t pl) {
    payload_[0] = (pl >> 16) & 0xff;
    payload_[1] = (pl >> 8) & 0xff;
    payload_[2] = pl & 0xff;
  }

private:
  uint32_t length_ :4;
  uint32_t id_ :4;
  uint8_t payload_[3];

};

#endif /* RTPHEADEREXT_H_ */
