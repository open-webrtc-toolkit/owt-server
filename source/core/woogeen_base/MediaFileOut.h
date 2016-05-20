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

#include <AVStreamOut.h>
#include <MediaUtilities.h>
#include <logger.h>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace woogeen_base {

class MediaFileOut : public AVStreamOut {
    DECLARE_LOGGER();

public:
    MediaFileOut(const std::string& url, const AVOptions* audio, const AVOptions* video, int snapshotInterval, EventRegistry* handle);
    ~MediaFileOut();

    // AVStreamOut interface
    void onFrame(const Frame&);
    void onTimeout();

private:
    void close();
    bool init(const AVOptions* audio, const AVOptions* video);
    bool addVideoStream(enum AVCodecID codec_id, unsigned int width, unsigned int height);
    bool addAudioStream(enum AVCodecID codec_id, int nbChannels = 1, int sampleRate = 8000);
    int writeAVFrame(AVStream*, const EncodedFrame&);

    AVStream* m_videoStream;
    AVStream* m_audioStream;
    AVFormatContext* m_context;
    std::string m_recordPath;
    int m_snapshotInterval; // FIXME: snapshot interval for the future usage
};

} /* namespace woogeen_base */

#endif /* MediaFileOut_h */
