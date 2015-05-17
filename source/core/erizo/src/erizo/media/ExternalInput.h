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
  class ExternalInput : public MediaSource {
      DECLARE_LOGGER();
    public:
      DLL_PUBLIC ExternalInput (const std::string& inputUrl);
      virtual ~ExternalInput();
      DLL_PUBLIC int init();
      int sendFirPacket();

    private:
      std::string url_;
      bool running_;
      boost::thread thread_;
      AVFormatContext* context_;
      AVPacket avpacket_;
      int video_stream_index_,video_time_base_;
      int audio_stream_index_, audio_time_base_;

      void receiveLoop();
  };
}
#endif /* EXTERNALINPUT_H_ */
