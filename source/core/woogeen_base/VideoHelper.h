/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
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

#ifndef VideoHelper_h
#define VideoHelper_h

#include <string>
#include <map>

namespace woogeen_base {

struct VideoSize {
    unsigned int width;
    unsigned int height;
};

struct YUVColor {
    uint8_t y;
    uint8_t cb; // Also called U
    uint8_t cr; // Also called V
};

const std::map<std::string, VideoSize> VideoResolutions =
    {{"cif", {352, 288}},
     {"vga", {640, 480}},
     {"svga", {800, 600}},
     {"xga", {1024, 768}},
     {"hd720p", {1280, 720}},
     {"sif", {320, 240}},
     {"hvga", {480, 320}},
     {"r480x360", {480, 360}},
     {"r640x360", {640, 360}},
     {"qcif", {176, 144}},
     {"r192x144", {192, 144}},
     {"hd1080p", {1920, 1080}},
     {"uhd_4k", {3840, 2160}},
     {"r360x360", {360, 360}},
     {"r480x480", {480, 480}},
     {"r720x720", {720, 720}}
    };

class VideoResolutionHelper {
public:
    static bool getVideoSize(const std::string& resolution, VideoSize& videoSize) {
        std::map<std::string, VideoSize>::const_iterator it = VideoResolutions.find(resolution);
        if (it != VideoResolutions.end()) {
            videoSize = it->second;
            return true;
        }
        return false;
    }
};

// Video background colors definition
const std::map<std::string, YUVColor> VideoColors =
    {{"black", {0x00, 0x80, 0x80}},
     {"white", {0xFF, 0x80, 0x80}}};

class VideoColorHelper {
public:
    static bool getVideoColor(const std::string& name, YUVColor& color) {
        std::map<std::string, YUVColor>::const_iterator it = VideoColors.find(name);
        if (it != VideoColors.end()) {
            color = it->second;
            return true;
        }
        return false;
    }

    static bool getVideoColor(int r, int g, int b, YUVColor& color) {
        // Make sure the RGB values make sense
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
            return false;

        color.y = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
        color.cb = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;
        color.cr = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;
        return true;
    }
};

}
#endif
