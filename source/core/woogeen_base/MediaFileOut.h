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

#ifndef MediaFileOut_h
#define MediaFileOut_h

#include <AVStreamOut.h>
#include <MediaUtilities.h>
#include <logger.h>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avstring.h>
}

namespace woogeen_base {

class MediaFileOut : public AVStreamOut {
    DECLARE_LOGGER();

public:
    MediaFileOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle);
    ~MediaFileOut();

    // AVStreamOut interface
    void onFrame(const Frame&);
    void onTimeout();
    void onVideoSourceChanged();

protected:
    void close();
    bool addAudioStream(FrameFormat format, uint32_t sampleRate, uint32_t channels);
    bool addVideoStream(FrameFormat format, uint32_t width, uint32_t height);
    bool getReady();
    int writeAVFrame(AVStream*, const EncodedFrame&, bool isVideo);
    char *ff_err2str(int errRet);

private:
    boost::scoped_ptr<MediaFrameQueue> m_videoQueue;
    boost::scoped_ptr<MediaFrameQueue> m_audioQueue;

    std::string m_uri;
    bool m_hasAudio;
    bool m_hasVideo;

    FrameFormat m_audioFormat;
    uint32_t m_sampleRate;
    uint32_t m_channels;

    FrameFormat m_videoFormat;
    uint32_t m_width;
    uint32_t m_height;

    AVFormatContext* m_context;
    AVStream* m_audioStream;
    AVStream* m_videoStream;

    bool m_videoSourceChanged;

    Frame m_videoKeyFrame;

    char m_errbuff[500];
};

} /* namespace woogeen_base */

#endif /* MediaFileOut_h */
