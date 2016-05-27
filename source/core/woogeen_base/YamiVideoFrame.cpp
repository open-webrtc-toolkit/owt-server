/*
 * Copyright 2016 Intel Corporation All Rights Reserved.
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

#include "YamiVideoFrame.h"

#include "VideoDisplay.h"
#include <webrtc/video_frame.h>

using namespace webrtc;

bool YamiVideoFrame::convertToI420VideoFrame(I420VideoFrame& i420Frame)
{
    if (!frame)
        return false;

    VAImage image;
    uint8_t* buffer = mapVASurfaceToVAImage(frame->surface, image);
    if (!buffer)
        return false;

    bool ret = false;
    i420Frame.set_timestamp(frame->timeStamp);
    switch (image.format.fourcc) {
    case VA_FOURCC_YV12:
        assert(image.num_planes == 3);
        i420Frame.CreateFrame(image.offsets[1] - image.offsets[0],
                              buffer + image.offsets[0],
                              image.data_size - image.offsets[2],
                              buffer + image.offsets[2],
                              image.offsets[2] - image.offsets[1],
                              buffer + image.offsets[1],
                              image.width,
                              image.height,
                              image.pitches[0],
                              image.pitches[2],
                              image.pitches[1],
                              kVideoRotation_0);
        ret = true;
        break;
    case VA_FOURCC_NV12:
        // TODO
        assert(false);
        break;
    default:
        assert(false);
        break;
    }

    unmapVAImage(image);
    return ret;
}
