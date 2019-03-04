// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FfEncoder_h
#define FfEncoder_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/common_types.h>

#include <logger.h>

#include "MediaFramePipeline.h"
#include "AudioEncoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avstring.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/audio_fifo.h>
}

namespace mcu {
using namespace woogeen_base;
using namespace webrtc;

class FfEncoder : public AudioEncoder {
    DECLARE_LOGGER();

public:
    FfEncoder(const FrameFormat format);
    ~FfEncoder();

    bool init() override;
    bool addAudioFrame(const AudioFrame *audioFrame) override;

protected:
    bool initEncoder(const FrameFormat format);
    bool addToFifo(const AudioFrame* audioFrame);
    void encode();
    void sendOut(AVPacket &pkt);
    char *ff_err2str(int errRet);

private:
    FrameFormat m_format;

    uint32_t m_timestampOffset;
    bool m_valid;

    int32_t m_channels;
    int32_t m_sampleRate;

    AVCodecContext* m_audioEnc;
    AVAudioFifo* m_audioFifo;
    AVFrame* m_audioFrame;

    char m_errbuff[500];
};

} /* namespace mcu */

#endif /* FfEncoder_h */
