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

#ifndef WGOpenH264Encoder_h
#define WGOpenH264Encoder_h

#include "logger.h"

#include "codec/api/svc/codec_api.h"
#include "codec/api/svc/codec_app_def.h"
#include "codec/api/svc/codec_def.h"
#include "codec/api/svc/codec_ver.h"

namespace webrtc {

class WGOpenH264Encoder : public VideoEncoder {
    DECLARE_LOGGER();

public:
    static WGOpenH264Encoder* Create();

    explicit WGOpenH264Encoder();
    ~WGOpenH264Encoder() override;

    int32_t InitEncode(const VideoCodec* codec_settings,
            int32_t number_of_cores,
            size_t max_payload_size) override;
    int32_t Release() override;

    int32_t RegisterEncodeCompleteCallback(
            EncodedImageCallback* callback) override;
    int32_t SetRateAllocation(const BitrateAllocation& bitrate_allocation,
            uint32_t framerate) override;

    int32_t Encode(const VideoFrame& frame,
            const CodecSpecificInfo* codec_specific_info,
            const std::vector<FrameType>* frame_types) override;

    const char* ImplementationName() const override {return "WGOpenH264Encoder";}

    int32_t SetChannelParameters(uint32_t packet_loss, int64_t rtt) override {return WEBRTC_VIDEO_CODEC_OK;}
    int32_t SetResolution(uint32_t width, uint32_t height) override;

private:
    uint32_t width_;
    uint32_t height_;
    uint32_t frame_rate_;
    uint32_t bitrate_;
    uint32_t idr_interval_;

    EncodedImage encoded_image_;
    EncodedImageCallback* encoded_complete_callback_;
    bool inited_;
    bool first_frame_encoded_;
    int64_t timestamp_;
    ISVCEncoder* encoder_;
};

}  // namespace webrtc


#endif /* WGOpenH264Encoder_h */
