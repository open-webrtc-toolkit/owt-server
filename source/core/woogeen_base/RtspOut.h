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
#include <libavutil/audio_fifo.h>
}

#include <fstream>
#include <memory>

namespace woogeen_base {

class RtspOut : public AVStreamOut {
    DECLARE_LOGGER();

    static const uint32_t VIDEO_BUFFER_SIZE = 4 * 1024 * 1024;

public:
    RtspOut(const std::string& url, const AVOptions* audio, const AVOptions* video, EventRegistry* handle);
    ~RtspOut();

    // AVStreamOut interface
    void onFrame(const woogeen_base::Frame&);
    void onTimeout() {;}

protected:
    static int readFunction(void* opaque, uint8_t* buf, int buf_size);

    bool hasAudio() {return !m_audioOptions.codec.empty();}
    bool hasVideo() {return !m_videoOptions.codec.empty();}

    void audioRun();
    void videoRun();

    bool detectInputVideoStream();

    bool addVideoStream(enum AVCodecID codec_id, unsigned int width, unsigned int height);
    bool addAudioStream(enum AVCodecID codec_id, int nbChannels = 2, int sampleRate = 48000);

    bool init();
    void close();

    int writeAudioFrame();
    int writeVideoFrame();

    void addToAudioFifo(uint8_t* data, int nbSamples);
    bool encodeAudioFrame(AVPacket *pkt);

private:
    std::string m_uri;

    AVOptions m_audioOptions;
    AVOptions m_videoOptions;

    boost::thread m_audioWorker;
    boost::thread m_videoWorker;

    bool m_audioReceived;
    bool m_videoReceived;

    boost::mutex m_readCbmutex;
    boost::condition_variable m_readCbcond;

    AVFormatContext* m_ifmtCtx;
    AVStream* m_inputVideoStream;

    AVFormatContext* m_context;
    boost::shared_mutex m_contextMutex;

    AVStream* m_audioStream;
    AVStream* m_videoStream;

    AVAudioFifo* m_audioFifo;
    boost::mutex m_audioFifoMutex;
    boost::condition_variable m_audioFifoCond;

    AVFrame* m_audioEncodingFrame;

    int64_t m_timeOffset;

    int64_t m_lastAudioTimestamp;
    int64_t m_lastVideoTimestamp;

#ifdef DUMP_AUDIO_RAW
    std::unique_ptr<std::ofstream> m_audioRawDumpFile;
#endif
};
}

#endif // RtspOut_h
