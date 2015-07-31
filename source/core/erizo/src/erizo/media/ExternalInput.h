#ifndef EXTERNALINPUT_H_
#define EXTERNALINPUT_H_

#include "../MediaDefinitions.h"

#include <boost/thread.hpp>
#include <logger.h>
#include <map>
#include <queue>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

namespace erizo{

  static inline int64_t currentTimeMillis() {
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
  }

  class TimeoutHandler {
    public:
      TimeoutHandler(int32_t timeout) : timeout_ms_(timeout), lastTime_(currentTimeMillis()) { }

      void reset(int32_t timeout) {
        timeout_ms_ = timeout;
        lastTime_ = currentTimeMillis();
      }

      static int check_interrupt(void* handler) {
        return handler && static_cast<TimeoutHandler *>(handler)->is_timeout();
      }

    private:
      bool is_timeout() {
        int32_t delay = currentTimeMillis() - lastTime_;
        return delay > timeout_ms_;
      }

      int32_t timeout_ms_;
      int64_t lastTime_;
  };

  class ExternalInputStatusListener {
    public:
      virtual ~ExternalInputStatusListener() {
      }

      virtual void notifyStatus(const std::string& message) = 0;
  };

  class ExternalInput : public MediaSource {
      DECLARE_LOGGER();
    public:
      DLL_PUBLIC ExternalInput (const std::string& options);
      virtual ~ExternalInput();
      DLL_PUBLIC void init();
      int sendFirPacket();
      void setStatusListener(ExternalInputStatusListener* listener);

    private:
      std::string url_;
      AVDictionary* transport_opts_;
      bool running_;
      boost::thread thread_;
      AVFormatContext* context_;
      TimeoutHandler* timeoutHandler_;
      AVPacket avpacket_;
      int video_stream_index_;
      int audio_stream_index_;
      uint32_t audio_sequence_number_;
      ExternalInputStatusListener* statusListener_;

      bool connect();
      void receiveLoop();
  };
}
#endif /* EXTERNALINPUT_H_ */
