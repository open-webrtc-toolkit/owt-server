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
#include "webrtc/modules/video_coding/codecs/h264/h264_encoder_impl.h"

#include <limits>
#include <string>

#include "webrtc/base/checks.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"

#include "WGOpenH264Encoder.h"

namespace webrtc {

DEFINE_LOGGER(WGOpenH264Encoder, "woogeen.WGOpenH264Encoder");

WGOpenH264Encoder* WGOpenH264Encoder::Create()
{
  return new WGOpenH264Encoder();
}

WGOpenH264Encoder::WGOpenH264Encoder()
    : width_(0),
    height_(0),
    frame_rate_(0),
    bitrate_(0),
    idr_interval_(0),
    encoded_complete_callback_(NULL),
    inited_(false),
    first_frame_encoded_(false),
    timestamp_(0),
    encoder_(NULL)
{
}

WGOpenH264Encoder::~WGOpenH264Encoder()
{
    Release();
}

int32_t WGOpenH264Encoder::InitEncode(const VideoCodec* codec_settings,
        int32_t number_of_cores,
        size_t max_payload_size)
{
    if (codec_settings == NULL) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (codec_settings->maxFramerate < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    // allow zero to represent an unspecified maxBitRate
    if (codec_settings->maxBitrate > 0 && codec_settings->startBitrate > codec_settings->maxBitrate) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (codec_settings->width < 1 || codec_settings->height < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (number_of_cores < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    int ret_val= Release();
    if (ret_val < 0) {
        return ret_val;
    }

    if (encoder_ == NULL) {
        ret_val = WelsCreateSVCEncoder(&encoder_);
        if (ret_val != 0) {
            ELOG_ERROR_T("Fail to create encoder, ret(%d)",ret_val);
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
    }

#if 0
    int32_t iTraceLevel = WELS_LOG_DETAIL;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
#endif

    ELOG_INFO_T("%s, width(%d), height(%d), maxFrameRate(%d), targetBitrateKbps(%d), idrInterval(%d)",
            g_strCodecVer,
            codec_settings->width,
            codec_settings->height,
            codec_settings->maxFramerate,
            codec_settings->targetBitrate,
            codec_settings->H264().keyFrameInterval)

    SEncParamBase param;
    memset (&param, 0, sizeof(SEncParamBase));

    param.iUsageType = CAMERA_VIDEO_REAL_TIME;
    param.iRCMode = RC_BITRATE_MODE;
    param.fMaxFrameRate = codec_settings->maxFramerate;
    param.iPicWidth = codec_settings->width;
    param.iPicHeight = codec_settings->height;
    param.iTargetBitrate = codec_settings->targetBitrate * 1000;

    ret_val = encoder_->Initialize(&param);
    if (ret_val != 0) {
        ELOG_ERROR_T("Fail to initialize encoder, ret(%d)", ret_val);

        WelsDestroySVCEncoder(encoder_);
        encoder_ = NULL;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    int32_t iIdrInterval = codec_settings->H264().keyFrameInterval;
    ret_val = encoder_->SetOption(ENCODER_OPTION_IDR_INTERVAL, &iIdrInterval);
    if (ret_val != 0) {
        ELOG_ERROR_T("Fail to set IDR interval, ret(%d)", ret_val);
    }

    width_ = codec_settings->width;
    height_ = codec_settings->height;
    frame_rate_ = codec_settings->maxFramerate;
    bitrate_ = codec_settings->targetBitrate * 1000;
    idr_interval_ = codec_settings->H264().keyFrameInterval;

    if (encoded_image_._buffer != NULL) {
        delete [] encoded_image_._buffer;
    }
    encoded_image_._size = CalcBufferSize(kI420, width_, height_);
    encoded_image_._buffer = new uint8_t[encoded_image_._size];
    encoded_image_._completeFrame = true;

    timestamp_ = 0;

    inited_ = true;

    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WGOpenH264Encoder::Release()
{
    if (encoded_image_._buffer != NULL) {
        delete [] encoded_image_._buffer;
        encoded_image_._buffer = NULL;
    }
    if (encoder_ != NULL) {
        WelsDestroySVCEncoder(encoder_);
        encoder_ = NULL;
    }
    inited_ = false;
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WGOpenH264Encoder::RegisterEncodeCompleteCallback(
        EncodedImageCallback* callback)
{
    encoded_complete_callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WGOpenH264Encoder::SetRateAllocation(
        const BitrateAllocation& bitrate_allocation,
        uint32_t framerate)
{
    if (bitrate_allocation.get_sum_bps() <= 0 || framerate <= 0)
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;

    ELOG_INFO("SetRate, bitrate %d -> %d, frame rate %d -> %d"
            , bitrate_allocation.get_sum_bps()
            , bitrate_
            , framerate
            , frame_rate_
            );

    bitrate_ = bitrate_allocation.get_sum_bps();
    frame_rate_ = static_cast<float>(framerate);

    SBitrateInfo target_bitrate;
    memset(&target_bitrate, 0, sizeof(SBitrateInfo));
    target_bitrate.iLayer = SPATIAL_LAYER_ALL,
        target_bitrate.iBitrate = bitrate_;
    encoder_->SetOption(ENCODER_OPTION_BITRATE,
            &target_bitrate);
    encoder_->SetOption(ENCODER_OPTION_FRAME_RATE, &frame_rate_);
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WGOpenH264Encoder::Encode(const VideoFrame& input_frame,
        const CodecSpecificInfo* codec_specific_info,
        const std::vector<FrameType>* frame_types)
{
    if (!inited_) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (encoded_complete_callback_ == NULL) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    FrameType frame_type = kVideoFrameDelta;
    // We only support one stream at the moment.
    if (frame_types && frame_types->size() > 0) {
        frame_type = (*frame_types)[0];
    }

    bool send_keyframe = (frame_type == kVideoFrameKey);
    if (send_keyframe) {
        encoder_->ForceIntraFrame(true);
        ELOG_DEBUG_T("ForceIntraFrame")
    }

    rtc::scoped_refptr<const VideoFrameBuffer> frame_buffer =
        input_frame.video_frame_buffer();

    SFrameBSInfo info;
    memset(&info, 0, sizeof(SFrameBSInfo));

    SSourcePicture pic;
    memset(&pic,0,sizeof(SSourcePicture));
    pic.iPicWidth = frame_buffer->width();
    pic.iPicHeight = frame_buffer->height();
    pic.iColorFormat = videoFormatI420;

    pic.iStride[0] = frame_buffer->StrideY();
    pic.iStride[1] = frame_buffer->StrideU();
    pic.iStride[2] = frame_buffer->StrideV();

    pic.pData[0]   = const_cast<uint8_t*>(frame_buffer->DataY());
    pic.pData[1]   = const_cast<uint8_t*>(frame_buffer->DataU());
    pic.pData[2]   = const_cast<uint8_t*>(frame_buffer->DataV());

    int retVal = encoder_->EncodeFrame(&pic, &info);
    if (retVal == videoFrameTypeSkip) {
        return WEBRTC_VIDEO_CODEC_OK;
    }

    RTPFragmentationHeader frag_info;
    encoded_image_._length = 0;

    int layer = 0;
    while (layer < info.iLayerNum) {
        const SLayerBSInfo* layer_bs_info = &info.sLayerInfo[layer];
        if (layer_bs_info != NULL) {
            int nal_begin  = 0;
            uint8_t* nal_buffer = NULL;
            char nal_type = 0;
            uint16_t fragments = frag_info.fragmentationVectorSize;
            frag_info.VerifyAndAllocateFragmentationHeader(
                    fragments + layer_bs_info->iNalCount);

            for (int nal_index = 0; nal_index < layer_bs_info->iNalCount; nal_index++) {
                nal_buffer  = layer_bs_info->pBsBuf + nal_begin;
                nal_type    = (nal_buffer[4] & 0x1F);
                nal_begin += layer_bs_info->pNalLengthInByte[nal_index];

                if (nal_type == 7 || nal_type == 8 || nal_type == 5) {
                    encoded_image_._frameType = kVideoFrameKey;
                } else {
                    encoded_image_._frameType = kVideoFrameDelta;
                }

                size_t bytes_to_copy = layer_bs_info->pNalLengthInByte[nal_index];
                memcpy(encoded_image_._buffer + encoded_image_._length, nal_buffer, bytes_to_copy);

                frag_info.fragmentationOffset[fragments + nal_index] = encoded_image_._length + 4;
                frag_info.fragmentationLength[fragments + nal_index] = bytes_to_copy - 4;

                encoded_image_._length += bytes_to_copy;
            } // for
        }
        layer++;
    }

    if (encoded_image_._length == 0) {
        return WEBRTC_VIDEO_CODEC_OK;
    }

    encoded_image_._timeStamp       = input_frame.timestamp();
    encoded_image_.capture_time_ms_ = input_frame.render_time_ms();
    encoded_image_._encodedHeight = height_;
    encoded_image_._encodedWidth = width_;
    //encoded_image_.ntp_time_ms_ = input_frame.ntp_time_ms();
    //encoded_image_.rotation_ = input_frame.rotation();

    ELOG_TRACE_T("Encoded, length(%ld) %s", encoded_image_._length, encoded_image_._frameType == kVideoFrameKey ? "key frame" : "");

    CodecSpecificInfo codec_specific;
    memset(&codec_specific, 0, sizeof(codec_specific));
    codec_specific.codecType = kVideoCodecH264;
    // call back
    encoded_complete_callback_->OnEncodedImage(encoded_image_, &codec_specific, &frag_info);

    if (!first_frame_encoded_) {
        first_frame_encoded_ = true;
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WGOpenH264Encoder::SetResolution(uint32_t width, uint32_t height)
{
    SEncParamBase param;
    memset (&param, 0, sizeof(SEncParamBase));

    ELOG_INFO("SetResolution, %dx%d -> %dx%d"
            , width
            , height
            , width_
            , height_
            );

    param.iUsageType = CAMERA_VIDEO_REAL_TIME;
    param.iRCMode = RC_BITRATE_MODE;
    param.fMaxFrameRate = frame_rate_;
    param.iPicWidth = width;
    param.iPicHeight = height;
    param.iTargetBitrate = bitrate_;

    int ret = encoder_->SetOption(ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, &param);
    if (ret != 0) {
        ELOG_ERROR("Fail to set option base param, ret(%d)", ret);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    width_ = width;
    height_ = height;

    return WEBRTC_VIDEO_CODEC_OK;
}

}  // namespace webrtc
