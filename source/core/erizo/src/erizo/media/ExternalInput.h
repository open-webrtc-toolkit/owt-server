#ifndef EXTERNALINPUT_H_
#define EXTERNALINPUT_H_

#include <string> 
#include <map>
#include <queue>
#include "../MediaDefinitions.h"
#include "boost/thread.hpp"
#include <logger.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

namespace erizo{

  class ExternalInputStatusListener {
    public:
      virtual ~ExternalInputStatusListener() {
      }

      virtual void notifyStatus(const std::string& message) = 0;
  };

  class ExternalInput : public MediaSource {
      DECLARE_LOGGER();
    public:
      DLL_PUBLIC ExternalInput (const std::string& inputUrl);
      virtual ~ExternalInput();
      DLL_PUBLIC int init();
      int sendFirPacket();
      void setStatusListener(ExternalInputStatusListener* listener);

    private:
      std::string url_;
      bool running_;
      boost::thread thread_;
      AVFormatContext* context_;
      AVPacket avpacket_;
      int video_stream_index_,video_time_base_;
      int audio_stream_index_, audio_time_base_;
      ExternalInputStatusListener* statusListener_;

      bool connect();
      void receiveLoop();
  };
}
#endif /* EXTERNALINPUT_H_ */
