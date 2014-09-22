/**
 * VideoCodec.h
 */

#ifndef AUDIOCODEC_H_
#define AUDIOCODEC_H_

#include "Codecs.h"
#include <logger.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

namespace erizo {

  class AudioEncoder {
    DECLARE_LOGGER();
    public:
      AudioEncoder();
      virtual ~AudioEncoder();
      int initEncoder (const AudioCodecInfo& info);
      int encodeAudio (unsigned char* inBuffer, int nSamples, AVPacket* pkt);
      int closeEncoder ();

    private:
      AVCodec* aCoder_;
      AVCodecContext* aCoderContext_;
      AVFrame* aFrame_;
  };

  class AudioDecoder {
    DECLARE_LOGGER();
    public:
      AudioDecoder();
      virtual ~AudioDecoder();
      int initDecoder (const AudioCodecInfo& info);
      int initDecoder (AVCodecContext* context);
      int decodeAudio(unsigned char* inBuff, int inBuffLen,
          unsigned char* outBuff, int outBuffLen, int* gotFrame);
      int closeDecoder();

    private:
      AVCodec* aDecoder_;
      AVCodecContext* aDecoderContext_;
      AVFrame* dFrame_;
  };

}
#endif /* AUDIOCODEC_H_ */
