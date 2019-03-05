/*
 * mediadefinitions.h
 */

#ifndef MEDIADEFINITIONSOLD_H_
#define MEDIADEFINITIONSOLD_H_

namespace erizoExtra {

//class NiceConnection;

//////////////
// OLD INTEFACE ////
/*
 * The maximum size of a data packet is 1472:
 * (1500 (MTU) - 20 (IP header) - 8 (UDP header))
 * We should not cross this boundary either forwarding a packet (which is
 * guaranteed by the protocol) or sending a packet produced by ourselves (which
 * we should take care of).
 */
static const int MAX_DATA_PACKET_SIZE = 1472;

struct dataPacket{
  int comp;
  int length;
  erizo::packetType type;
};

/**
 * A NiceReceiver is any class that can receive data from a nice connection.
 *
class NiceReceiver{
public:
  virtual int receiveNiceData(char* buf, int len, NiceConnection* nice)=0;
  virtual ~NiceReceiver(){};
};
*/
enum DataType {
  VIDEO, AUDIO
};

struct RawDataPacket {
  unsigned char* data;
  int length;
  DataType type;
};

class RawDataReceiver {
public:
  virtual void receiveRawData(RawDataPacket& packet) = 0;
  virtual ~RawDataReceiver() {
  }
  ;
};

class RTPDataReceiver {
public:
  virtual void receiveRtpData(char* rtpdata, int len, DataType type = VIDEO, uint32_t streamId = 0) = 0;
  virtual ~RTPDataReceiver() {
  }
  ;
};

} /* namespace erizo */

#endif /* MEDIADEFINITIONSOLD_H_ */
