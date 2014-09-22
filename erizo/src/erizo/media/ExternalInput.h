#ifndef EXTERNALINPUT_H_
#define EXTERNALINPUT_H_

#include <string> 
#include <map>
#include <queue>
#include "../MediaDefinitions.h"
#include "codecs/VideoCodec.h"
#include "MediaProcessor.h"
#include "boost/thread.hpp"
#include <logger.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

namespace erizo{
  class WebRtcConnection;

  class ExternalInput : public MediaSource, public RTPDataReceiver {
      DECLARE_LOGGER();
    public:
      DLL_PUBLIC ExternalInput (const std::string& inputUrl);
      virtual ~ExternalInput();
      DLL_PUBLIC int init();
      void receiveRtpData(char* rtpdata, int len, DataType type = VIDEO, uint32_t streamId = 0);
      int sendFirPacket();


    private:
      boost::scoped_ptr<OutputProcessor> op_;
      VideoDecoder inCodec_;
      boost::scoped_array<unsigned char> decodedBuffer_;
      char sendVideoBuffer_[2000];

      std::string url_;
      bool running_;
      bool needTranscoding_;
	    boost::mutex queueMutex_;
      boost::thread thread_, encodeThread_;
      std::queue<RawDataPacket> packetQueue_;
      AVFormatContext* context_;
      AVPacket avpacket_;
      int video_stream_index_,video_time_base_;
      int audio_stream_index_, audio_time_base_;
      int bufflen_;

      int64_t lastPts_,lastAudioPts_;
      int64_t startTime_;


      void receiveLoop();
      void encodeLoop();

  };
}
#endif /* EXTERNALINPUT_H_ */
