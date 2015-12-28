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

#ifndef VCMFrameDecoder_h
#define VCMFrameDecoder_h

#include "MediaFramePipeline.h"

#include <boost/scoped_ptr.hpp>
#include <logger.h>
#include <webrtc/modules/video_coding/codecs/interface/video_codec_interface.h>
#include <webrtc/video_decoder.h>

namespace woogeen_base {

class VCMFrameDecoder : public VideoFrameDecoder, public webrtc::DecodedImageCallback {
    DECLARE_LOGGER();

public:
    VCMFrameDecoder(FrameFormat format);
    ~VCMFrameDecoder();

    bool init(FrameFormat format);

    void onFrame(const Frame&);
    int32_t Decoded(webrtc::I420VideoFrame& decodedImage);

private:
    bool m_needDecode;
    int64_t m_ntpDelta;
    webrtc::CodecSpecificInfo m_codecInfo;
    boost::scoped_ptr<webrtc::VideoDecoder> m_decoder;
};

} /* namespace woogeen_base */

#endif /* VCMFrameDecoder_h */
