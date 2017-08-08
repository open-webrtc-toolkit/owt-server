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
}

namespace woogeen_base {

class RtspOut : public AVStreamOut {
    DECLARE_LOGGER();
public:
    RtspOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int streamingTimeout);
    ~RtspOut();

    void onFrame(const woogeen_base::Frame&);
    void onTimeout() {;}

protected:
    bool isHls(std::string uri) {return (uri.compare(0, 7, "http://") == 0);}

    void sendLoop();

    bool connect();
    bool reconnect();
    void close();

    bool addAudioStream(FrameFormat format, uint32_t sampleRate, uint32_t channels);
    bool addVideoStream(FrameFormat format, uint32_t width, uint32_t height);

    bool writeHeader();
    int writeAVFrame(AVStream* stream, const EncodedFrame& frame);

    char *ff_err2str(int errRet);

private:
    std::string m_uri;
    bool m_hasAudio;
    bool m_hasVideo;

    FrameFormat m_audioFormat;
    uint32_t m_sampleRate;
    uint32_t m_channels;

    FrameFormat m_videoFormat;
    uint32_t m_width;
    uint32_t m_height;

    boost::thread m_thread;

    bool m_audioReceived;
    bool m_videoReceived;

    AVFormatContext* m_context;
    AVStream* m_audioStream;
    AVStream* m_videoStream;

    boost::shared_ptr<woogeen_base::EncodedFrame> m_videoKeyFrame;
    boost::scoped_ptr<MediaFrameQueue> m_frameQueue;

    std::ostringstream m_AsyncEvent;

    char m_errbuff[500];
};

}

#endif // RtspOut_h
