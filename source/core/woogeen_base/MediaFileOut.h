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

#ifndef MediaFileOut_h
#define MediaFileOut_h

#include <logger.h>
#include <AVStreamOut.h>
#include <MediaUtilities.h>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace woogeen_base {

class MediaFileOut : public AVStreamOut {
    DECLARE_LOGGER();

public:
    MediaFileOut(const std::string& url, MediaSpecInfo& audio, MediaSpecInfo& video, int snapshotInterval, EventRegistry* handle);
    ~MediaFileOut();

    // AVStreamOut interface
    void onFrame(const Frame&);

    void onTimeout();

private:
    void close();
    bool addVideoStream(enum AVCodecID codec_id, unsigned int width = 1280, unsigned int height = 720);
    bool addAudioStream(enum AVCodecID codec_id, int nbChannels = 1, int sampleRate = 8000);
    void writeVideoFrame(EncodedFrame& encoded_frame);
    void writeAudioFrame(EncodedFrame& encoded_frame);

    AVStream* m_videoStream;
    AVStream* m_audioStream;
    AVFormatContext* m_context;
    boost::mutex m_contextMutex;
    bool m_avTrailerNeeded;

    int32_t m_videoId;
    int32_t m_audioId;

    woogeen_base::FrameFormat m_videoFrameFormat;
    woogeen_base::FrameFormat m_audioFrameFormat;
    std::string m_recordPath;
    // FIXME: snapshot interval for the future usage
    int m_snapshotInterval;
};

} /* namespace woogeen_base */

#endif /* MediaFileOut_h */
