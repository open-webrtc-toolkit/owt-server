// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VCMFrameDecoder_h
#define VCMFrameDecoder_h

#include "MediaFramePipeline.h"

#include <boost/scoped_ptr.hpp>
#include <logger.h>

#include <webrtc/modules/video_coding/include/video_codec_interface.h>
#include <webrtc/video_decoder.h>

namespace woogeen_base {

class VCMFrameDecoder : public VideoFrameDecoder, public webrtc::DecodedImageCallback {
    DECLARE_LOGGER();

public:
    VCMFrameDecoder(FrameFormat format);
    ~VCMFrameDecoder();

    static bool supportFormat(FrameFormat format) {
        return (format == FRAME_FORMAT_VP8
                || format == FRAME_FORMAT_VP9
                || format == FRAME_FORMAT_H264);
    }

    bool init(FrameFormat format);

    void onFrame(const Frame&);
    int32_t Decoded(webrtc::VideoFrame& decodedImage);

private:
    bool m_needDecode;
    bool m_needKeyFrame;
    webrtc::CodecSpecificInfo m_codecInfo;
    boost::scoped_ptr<webrtc::VideoDecoder> m_decoder;
};

} /* namespace woogeen_base */

#endif /* VCMFrameDecoder_h */
