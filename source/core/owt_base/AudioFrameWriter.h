// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioFrameWriter_h
#define AudioFrameWriter_h

#include <string>

#include <logger.h>

#include <webrtc/modules/include/module_common_types.h>

#include "MediaFramePipeline.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace owt_base {

class AudioFrameWriter {
    DECLARE_LOGGER();

public:
    AudioFrameWriter(const std::string& name);
    ~AudioFrameWriter();

    void write(const Frame& frame);
    void write(const webrtc::AudioFrame *audioFrame);

protected:
    FILE *getAudioFp(const webrtc::AudioFrame *audioFrame);

    void writeCompressedFrame(const Frame& frame);
    bool addAudioStream(FrameFormat format, uint32_t sampleRate, uint32_t channels);

private:
    std::string m_name;

    FILE *m_fp;
    uint32_t m_index;

    int32_t m_sampleRate;
    int32_t m_channels;

    AVFormatContext *m_context;
    AVStream *m_audioStream;
};

}

#endif // AudioFrameWriter_h
