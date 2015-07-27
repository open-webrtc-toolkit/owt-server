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

#ifndef RTSPMuxer_h
#define RTSPMuxer_h

#include "MediaMuxer.h"
#include "MediaUtilities.h"

#include <boost/scoped_ptr.hpp>
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

namespace mcu {

class RTSPMuxer : public woogeen_base::MediaMuxer {
    DECLARE_LOGGER();
public:
    RTSPMuxer(const std::string& url, woogeen_base::EventRegistry* callback = nullptr);
    ~RTSPMuxer();

    // MediaMuxer interface
    bool setMediaSource(woogeen_base::FrameDispatcher* videoDispatcher, woogeen_base::FrameDispatcher* audioDispatcher);
    void unsetMediaSource();
    void onFrame(const woogeen_base::Frame&);
    void onTimeout();

private:
    void close();
    void addVideoStream(enum AVCodecID codec_id, unsigned int width = 640, unsigned int height = 480);
    void addAudioStream(enum AVCodecID codec_id, int nbChannels = 2, int sampleRate = 48000);
    int writeVideoFrame(uint8_t*, size_t, int64_t);
    int writeAudioFrame(uint8_t*, size_t, int64_t);
    AVFrame* allocAudioFrame(AVCodecContext*);
    void encodeAudio();
    void processAudio(uint8_t* data, int nbSamples, int nbChannels = 2, int sampleRate = 48000);

    woogeen_base::FrameDispatcher*                  m_videoSource;
    woogeen_base::FrameDispatcher*                  m_audioSource;
    AVFormatContext*                                m_context;
    SwrContext*                                     m_resampleContext;
    AVAudioFifo*                                    m_audioFifo;
    AVStream*                                       m_videoStream;
    AVStream*                                       m_audioStream;
    int32_t                                         m_videoId, m_audioId;
    std::string                                     m_uri;
    AVFrame*                                        m_audioEncodingFrame;
    boost::mutex                                    m_contextMutex;
#ifdef DUMP_RAW
    std::unique_ptr<std::ofstream>                  m_dumpFile;
#endif
};

}

#endif // RTSPMuxer_h
