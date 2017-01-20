/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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
#include <libavcodec/avcodec.h>
#include <libavutil/avstring.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/audio_fifo.h>
}

namespace woogeen_base {

class RtspOut : public AVStreamOut {
    DECLARE_LOGGER();
public:
    RtspOut(const std::string& url, const AVOptions* audio, const AVOptions* video, EventRegistry* handle);
    ~RtspOut();

    void onFrame(const woogeen_base::Frame&);
    void onTimeout() {;}

protected:
    bool hasAudio() {return !m_audioOptions.codec.empty();}
    bool hasVideo() {return !m_videoOptions.codec.empty();}
    bool isHls(std::string uri) {return (uri.compare(0, 7, "http://") == 0);}

    bool checkCodec(AVOptions &audioOptions, AVOptions &videoOptions);

    void sendLoop();

    bool connect();
    bool reconnect();
    void close();

    bool openAudioEncoder(AVOptions &options);
    bool addAudioStream(AVOptions &options);
    bool addVideoStream(AVOptions &options);

    bool writeHeader();
    void addAudioFrame(uint8_t* data, int nbSamples);
    int writeAVFrame(AVStream* stream, const EncodedFrame& frame);

    char *ff_err2str(int errRet);

private:
    std::string m_uri;
    AVOptions m_audioOptions;
    AVOptions m_videoOptions;

    boost::thread m_thread;

    bool m_audioReceived;
    bool m_videoReceived;

    AVFormatContext* m_context;
    AVStream* m_audioStream;
    AVStream* m_videoStream;

    AVCodecContext* m_audioEnc;
    AVAudioFifo* m_audioFifo;
    AVFrame* m_audioEncodingFrame;

    boost::shared_ptr<woogeen_base::EncodedFrame> m_videoKeyFrame;
    boost::scoped_ptr<MediaFrameQueue> m_frameQueue;

    std::ostringstream m_AsyncEvent;

    char m_errbuff[500];
};

}

#endif // RtspOut_h
