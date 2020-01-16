// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef Im360StitchParams_h
#define Im360StitchParams_h

#include <interface/stitcher.h>
#include "VideoHelper.h"

namespace mcu {
namespace Im360StitchParams {

enum CamModel {
    CamA2C1080P = 0,
    CamB4C1080P,
    CamC3C8K, // Insta
    CamD3C8K  // Kandao
};

inline owt_base::VideoSize outputImageSize(CamModel model)
{
    owt_base::VideoSize size;
    switch (model) {
    case CamC3C8K: {
        size.width = 7680;
        size.height = 3840;
        break;
    }
    case CamD3C8K: {
        size.width = 7680;
        size.height = 3840;
        break;
    }
    default:
        printf("unknown camera model (%d)", model);
        break;
    }
    return size;
}


inline void viewpointsRange(CamModel model, float *range)
{
    switch (model) {
    case CamA2C1080P: {
        range[0] = 202.8f;
        range[1] = 202.8f;
        break;
    }
    case CamB4C1080P: {
        range[0] = 64.0f;
        range[1] = 160.0f;
        range[2] = 64.0f;
        range[3] = 160.0f;
        break;
    }
    case CamC3C8K: {
        range[0] = 144.0f;
        range[1] = 144.0f;
        range[2] = 144.0f;
        break;
    }
    case CamD3C8K: {
        range[0] = 154.0f;
        range[1] = 154.0f;
        range[2] = 154.0f;
        break;
    }
    default:
        printf("unknown camera model (%d)", model);
        break;
    }
}

inline XCam::FMRegionRatio fmRegionRatio(CamModel model)
{
    XCam::FMRegionRatio ratio;

    switch (model) {
    case CamA2C1080P: {
        ratio.pos_x = 0.0f;
        ratio.width = 1.0f;
        ratio.pos_y = 1.0f / 3.0f;
        ratio.height = 1.0f / 3.0f;
        break;
    }
    case CamC3C8K: {
        ratio.pos_x = 0.0f;
        ratio.width = 1.0f;
        ratio.pos_y = 1.0f / 3.0f;
        ratio.height = 1.0f / 3.0f;
        break;
    }
    case CamD3C8K: {
        ratio.pos_x = 0.0f;
        ratio.width = 1.0f;
        ratio.pos_y = 1.0f / 3.0f;
        ratio.height = 1.0f / 3.0f;
        break;
    }
    default:
        printf("unsupported camera model (%d)", model);
        break;
    }

    return ratio;
}

inline XCam::FMConfig softFmConfig(CamModel model)
{
    XCam::FMConfig cfg;

    switch (model) {
    case CamA2C1080P: {
        cfg.stitch_min_width = 136;
        cfg.min_corners = 4;
        cfg.offset_factor = 0.9f;
        cfg.delta_mean_offset = 120.0f;
        cfg.recur_offset_error = 8.0f;
        cfg.max_adjusted_offset = 24.0f;
        cfg.max_valid_offset_y = 8.0f;
        cfg.max_track_error = 28.0f;
        break;
    }
    case CamB4C1080P: {
        cfg.stitch_min_width = 136;
        cfg.min_corners = 4;
        cfg.offset_factor = 0.8f;
        cfg.delta_mean_offset = 120.0f;
        cfg.recur_offset_error = 8.0f;
        cfg.max_adjusted_offset = 24.0f;
        cfg.max_valid_offset_y = 20.0f;
        cfg.max_track_error = 28.0f;
        break;
    }
    case CamC3C8K: {
        cfg.stitch_min_width = 136;
        cfg.min_corners = 4;
        cfg.offset_factor = 0.95f;
        cfg.delta_mean_offset = 256.0f;
        cfg.recur_offset_error = 4.0f;
        cfg.max_adjusted_offset = 24.0f;
        cfg.max_valid_offset_y = 20.0f;
        cfg.max_track_error = 6.0f;
        break;
    }
    case CamD3C8K: {
        cfg.stitch_min_width = 256;
        cfg.min_corners = 4;
        cfg.offset_factor = 0.6f;
        cfg.delta_mean_offset = 256.0f;
        cfg.recur_offset_error = 2.0f;
        cfg.max_adjusted_offset = 24.0f;
        cfg.max_valid_offset_y = 32.0f;
        cfg.max_track_error = 10.0f;
        break;
    }
    default:
        printf("unknown camera model (%d)", model);
        break;
    }

    return cfg;
}

inline XCam::StitchInfo softStitchInfo(CamModel model, XCam::StitchScopicMode scopic_mode)
{
    XCam::StitchInfo info;

    switch (model) {
    case CamA2C1080P: {
        info.fisheye_info[0].center_x = 480.0f;
        info.fisheye_info[0].center_y = 480.0f;
        info.fisheye_info[0].wide_angle = 202.8f;
        info.fisheye_info[0].radius = 480.0f;
        info.fisheye_info[0].rotate_angle = -90.0f;
        info.fisheye_info[1].center_x = 1436.0f;
        info.fisheye_info[1].center_y = 480.0f;
        info.fisheye_info[1].wide_angle = 202.8f;
        info.fisheye_info[1].radius = 480.0f;
        info.fisheye_info[1].rotate_angle = 89.7f;
        break;
    }
    case CamC3C8K: {
        switch (scopic_mode) {
        case XCam::ScopicStereoLeft: {
            info.merge_width[0] = 256;
            info.merge_width[1] = 256;
            info.merge_width[2] = 256;

            info.fisheye_info[0].center_x = 1907.0f;
            info.fisheye_info[0].center_y = 1440.0f;
            info.fisheye_info[0].wide_angle = 200.0f;
            info.fisheye_info[0].radius = 1984.0f;
            info.fisheye_info[0].rotate_angle = 90.3f;
            info.fisheye_info[1].center_x = 1920.0f;
            info.fisheye_info[1].center_y = 1440.0f;
            info.fisheye_info[1].wide_angle = 200.0f;
            info.fisheye_info[1].radius = 1984.0f;
            info.fisheye_info[1].rotate_angle = 90.2f;
            info.fisheye_info[2].center_x = 1920.0f;
            info.fisheye_info[2].center_y = 1440.0f;
            info.fisheye_info[2].wide_angle = 200.0f;
            info.fisheye_info[2].radius = 1984.0f;
            info.fisheye_info[2].rotate_angle = 91.2f;
            break;
        }
        case XCam::ScopicStereoRight: {
            info.merge_width[0] = 256;
            info.merge_width[1] = 256;
            info.merge_width[2] = 256;

            info.fisheye_info[0].center_x = 1920.0f;
            info.fisheye_info[0].center_y = 1440.0f;
            info.fisheye_info[0].wide_angle = 200.0f;
            info.fisheye_info[0].radius = 1984.0f;
            info.fisheye_info[0].rotate_angle = 90.0f;
            info.fisheye_info[1].center_x = 1920.0f;
            info.fisheye_info[1].center_y = 1440.0f;
            info.fisheye_info[1].wide_angle = 200.0f;
            info.fisheye_info[1].radius = 1984.0f;
            info.fisheye_info[1].rotate_angle = 90.0f;
            info.fisheye_info[2].center_x = 1914.0f;
            info.fisheye_info[2].center_y = 1440.0f;
            info.fisheye_info[2].wide_angle = 200.0f;
            info.fisheye_info[2].radius = 1984.0f;
            info.fisheye_info[2].rotate_angle = 90.1f;
            break;
        }
        default:
            printf("unsupported scopic mode (%d)", scopic_mode);
            break;
        }
        break;
    }
    case CamD3C8K: {
        switch (scopic_mode) {
        case XCam::ScopicStereoLeft: {
            info.merge_width[0] = 192;
            info.merge_width[1] = 192;
            info.merge_width[2] = 192;
            info.fisheye_info[0].center_x = 1804.0f;
            info.fisheye_info[0].center_y = 1532.0f;
            info.fisheye_info[0].wide_angle = 190.0f;
            info.fisheye_info[0].radius = 1900.0f;
            info.fisheye_info[0].rotate_angle = 91.5f;
            info.fisheye_info[1].center_x = 1836.0f;
            info.fisheye_info[1].center_y = 1532.0f;
            info.fisheye_info[1].wide_angle = 190.0f;
            info.fisheye_info[1].radius = 1900.0f;
            info.fisheye_info[1].rotate_angle = 92.0f;
            info.fisheye_info[2].center_x = 1820.0f;
            info.fisheye_info[2].center_y = 1532.0f;
            info.fisheye_info[2].wide_angle = 190.0f;
            info.fisheye_info[2].radius = 1900.0f;
            info.fisheye_info[2].rotate_angle = 91.0f;
            break;
        }
        case XCam::ScopicStereoRight: {
            info.merge_width[0] = 192;
            info.merge_width[1] = 192;
            info.merge_width[2] = 192;
            info.fisheye_info[0].center_x = 1836.0f;
            info.fisheye_info[0].center_y = 1532.0f;
            info.fisheye_info[0].wide_angle = 190.0f;
            info.fisheye_info[0].radius = 1900.0f;
            info.fisheye_info[0].rotate_angle = 88.0f;
            info.fisheye_info[1].center_x = 1852.0f;
            info.fisheye_info[1].center_y = 1576.0f;
            info.fisheye_info[1].wide_angle = 190.0f;
            info.fisheye_info[1].radius = 1900.0f;
            info.fisheye_info[1].rotate_angle = 90.0f;
            info.fisheye_info[2].center_x = 1836.0f;
            info.fisheye_info[2].center_y = 1532.0f;
            info.fisheye_info[2].wide_angle = 190.0f;
            info.fisheye_info[2].radius = 1900.0f;
            info.fisheye_info[2].rotate_angle = 91.0f;
            break;
        }
        default:
            printf("unsupported scopic mode (%d)", scopic_mode);
            break;
        }
        break;
    }
    default:
        printf("unsupported camera model (%d)", model);
        break;
    }

    return info;
}

}
}

#endif /* Im360StitchParams_h */
