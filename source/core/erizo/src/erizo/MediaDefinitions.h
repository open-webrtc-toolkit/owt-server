/*
 * mediadefinitions.h
 */

#ifndef MEDIADEFINITIONS_H_
#define MEDIADEFINITIONS_H_

#include <string>

#include <Compiler.h>

namespace erizo{

class NiceConnection;

enum packetType{
  VIDEO_PACKET,
  AUDIO_PACKET,
  OTHER_PACKET    
};

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
  char data[MAX_DATA_PACKET_SIZE];
  int length;
  packetType type;
};

class FeedbackSink{
public:
  virtual ~FeedbackSink() = 0;
  virtual int deliverFeedback(char* buf, int len)=0;
  // Is it able to sink the NACK feedback
  virtual bool acceptNACK() { return false; }
};

inline FeedbackSink::~FeedbackSink() {}

class FeedbackSource{
protected:
  FeedbackSink* fbSink_;
public:
  void setFeedbackSink(FeedbackSink* sink) {
    fbSink_ = sink;
  };
};

class MediaSource;
/*
 * A MediaSink 
 */
class MediaSink{
protected:
  //SSRCs received by the SINK
  unsigned int audioSinkSSRC_;
  unsigned int videoSinkSSRC_;
  //Is it able to provide Feedback
  FeedbackSource* sinkfbSource_;
public:
  virtual int deliverAudioData(char* buf, int len)=0;
  virtual int deliverVideoData(char* buf, int len)=0;
  unsigned int getVideoSinkSSRC (){return videoSinkSSRC_;};
  void setVideoSinkSSRC (unsigned int ssrc){videoSinkSSRC_ = ssrc;};
  unsigned int getAudioSinkSSRC (){return audioSinkSSRC_;};
  void setAudioSinkSSRC (unsigned int ssrc){audioSinkSSRC_ = ssrc;};
  FeedbackSource* getFeedbackSource(){
    return sinkfbSource_;
  };
  virtual int preferredVideoPayloadType() { return -1; }
  // Is it able to sink the encapsulated RTP data (like RED)
  virtual bool acceptEncapsulatedRTPData() { return false; }
  // Is it able to sink the FEC packet
  virtual bool acceptFEC() { return false; }
  // Is it able to sink the re-sent packet (response to NACK)
  virtual bool acceptResentData() { return false; }
  MediaSink() : audioSinkSSRC_(0), videoSinkSSRC_(0), sinkfbSource_(nullptr) {}
  virtual ~MediaSink() = 0; 
};

inline MediaSink::~MediaSink() {}

/**
 * A MediaSource is any class that produces audio or video data.
 */
class MediaSource{
protected: 
  //SSRCs coming from the source
  unsigned int videoSourceSSRC_;
  unsigned int audioSourceSSRC_;
  MediaSink* videoSink_;
  MediaSink* audioSink_;
  //can it accept feedback
  FeedbackSink* sourcefbSink_;
public:
  DLL_PUBLIC void setAudioSink(MediaSink* audioSink){
    this->audioSink_ = audioSink;
  };
  DLL_PUBLIC void setVideoSink(MediaSink* videoSink){
    this->videoSink_ = videoSink;
  };

  FeedbackSink* getFeedbackSink(){
    return sourcefbSink_;
  };
  virtual int sendFirPacket()=0;
  virtual int setVideoCodec(const std::string& codecName, unsigned int clockRate){return -1;};
  virtual int setAudioCodec(const std::string& codecName, unsigned int clockRate){return -1;};
  unsigned int getVideoSourceSSRC (){return videoSourceSSRC_;};
  void setVideoSourceSSRC (unsigned int ssrc){videoSourceSSRC_ = ssrc;};
  unsigned int getAudioSourceSSRC (){return audioSourceSSRC_;};
  void setAudioSourceSSRC (unsigned int ssrc){audioSourceSSRC_ = ssrc;};
  MediaSource() : videoSourceSSRC_(0), audioSourceSSRC_(0), videoSink_(nullptr), audioSink_(nullptr), sourcefbSink_(nullptr) {}
  virtual ~MediaSource(){};
};

/**
 * A NiceReceiver is any class that can receive data from a nice connection.
 */
class NiceReceiver{
public:
  virtual int receiveNiceData(char* buf, int len, NiceConnection* nice)=0;
  virtual ~NiceReceiver(){};
};

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

#endif /* MEDIADEFINITIONS_H_ */
