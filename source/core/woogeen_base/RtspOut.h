/*
 * Copyright 2015 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#ifndef RtspOut_h
#define RtspOut_h

#include "AVStreamOut.h"
#include "MediaUtilities.h"

#include <logger.h>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

#ifdef DUMP_RAW
#include <fstream>
#include <memory>
#endif

namespace woogeen_base {

class RtspOut : public AVStreamOut {
    DECLARE_LOGGER();
public:
    RtspOut(const std::string& url, MediaSpecInfo& audio, MediaSpecInfo& video, EventRegistry* handle);
    ~RtspOut();

    // AVStreamOut interface
    void onFrame(const woogeen_base::Frame&);
    void onTimeout();

private:
    void close();
    bool init(MediaSpecInfo& audio, MediaSpecInfo& video);
    bool addVideoStream(enum AVCodecID codec_id, unsigned int width, unsigned int height);
    bool addAudioStream(enum AVCodecID codec_id, int nbChannels = 2, int sampleRate = 48000);
    int writeVideoFrame(uint8_t*, size_t, int64_t);
    int writeAudioFrame(uint8_t*, size_t, int64_t);
    AVFrame* allocAudioFrame(AVCodecContext*);
    void encodeAudio();
    void processAudio(uint8_t* data, int nbSamples);

    AVFormatContext*                                m_context;
    SwrContext*                                     m_resampleContext;
    AVAudioFifo*                                    m_audioFifo;
    AVStream*                                       m_videoStream;
    AVStream*                                       m_audioStream;
    int32_t                                         m_videoId, m_audioId;
    std::string                                     m_uri;
    AVFrame*                                        m_audioEncodingFrame;
#ifdef DUMP_RAW
    std::unique_ptr<std::ofstream>                  m_dumpFile;
#endif
};

}

#endif // RtspOut_h
