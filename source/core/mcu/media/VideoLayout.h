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

#ifndef VideoLayout_h
#define VideoLayout_h

#include <string>
#include <vector>
#include <map>

namespace mcu {

static const uint32_t MAX_VIDEO_SLOT_NUMBER = 16;

/**
 * the configuration is a subset of rfc5707, VideoLayout element definition
 *    An example of a video layout with six regions is:

      +-------+---+
      |       | 2 |
      |   1   +---+
      |       | 3 |
      +---+---+---+
      | 6 | 5 | 4 |
      +---+---+---+

      <videolayout type="text/msml-basic-layout">
         <root size="CIF"/>
         <region id="1" left="0" top="0" relativesize="2/3"/>
         <region id="2" left="67%" top="0" relativesize="1/3"/>
         <region id="3" left="67%" top="33%" relativesize="1/3">
         <region id="4" left="67%" top="67%" relativesize="1/3"/>
         <region id="5" left="33%" top="67%" relativesize="1/3"/>
         <region id="6" left="0" top="67%" relativesize="1/3"/>
      </videolayout>
 */
enum VideoResolutionType
{
    cif = 0,//352x288
    vga,
    hd_720p,
    sif, //320x240
    hvga, //480x320
    r480x360,
    qcif, //176x144
    r192x144,
    hd_1080p,
    uhd_4k
};

enum VideoBackgroundColor
{
    black = 0, //YUV: 0x00,0x80,0x80
    white, //YUV: 0xFF,0x80,0x80
};

struct VideoSize {
    int width;
    int height;
};

struct YUVColor {
    uint8_t y;
    uint8_t cb; // Also called U
    uint8_t cr; // Also called V
};

struct Region {
    std::string id;
    float left; // percentage
    float top;    // percentage
    float relativeSize;    //fraction
    float priority;
};

struct VideoLayout {
    VideoResolutionType rootSize;
    VideoBackgroundColor rootColor;

    // Valid for customized video layout
    unsigned int maxInput;
    std::vector<Region> regions;

    // Valid for fluid video layout
    unsigned int divFactor;
};

// Default video layout configuration
const VideoSize DEFAULT_VIDEO_SIZE = {640, 480};
const YUVColor DEFAULT_VIDEO_BG_COLOR = {0x00, 0x80, 0x80};

// Video resolutions definition
const std::map<std::string, VideoResolutionType> VideoResolutions = {{"cif", cif}, {"vga", vga}, {"hd720p", hd_720p}, {"sif", sif},
  {"hvga",hvga}, {"r480x360", r480x360}, {"qcif", qcif}, {"r192x144", r192x144}, {"hd1080p", hd_1080p}, {"uhd_4k", uhd_4k}};

const std::map<VideoResolutionType, VideoSize> VideoSizes = {{cif, {352, 288}}, {vga, {640, 480}}, {hd_720p, {1280, 720}}, {sif, {320, 240}},
  {hvga, {480, 320}}, {r480x360, {480, 360}}, {qcif, {176, 144}}, {r192x144, {192, 144}}, {hd_1080p, {1920, 1080}}, {uhd_4k, {3840, 2160}}};

// Video background colors definition
const std::map<std::string, VideoBackgroundColor> VideoColors = {{"black", black}, {"white", white}};

const std::map<VideoBackgroundColor, YUVColor> VideoYuvColors = {{black, {0x00, 0x80, 0x80}}, {white, {0xFF, 0x80, 0x80}}};

class VideoLayoutHelper {
public:
    static VideoResolutionType getVideoResolution(const std::string& resolutionDescription)
    {
        // Fetch video resolution
        std::map<std::string, VideoResolutionType>::const_iterator it = VideoResolutions.find(resolutionDescription);
        if (it != VideoResolutions.end())
            return it->second;

        return VideoResolutionType::vga;
    }

    static VideoSize getVideoSize(VideoResolutionType videoResolution)
    {
        // Fetch video size
        std::map<VideoResolutionType, VideoSize>::const_iterator it = VideoSizes.find(videoResolution);
        if (it != VideoSizes.end())
            return it->second;

        return DEFAULT_VIDEO_SIZE;
    }

    static VideoBackgroundColor getVideoBackgroundColor(const std::string& colorDescription)
    {
        // Fetch video background color
        std::map<std::string, VideoBackgroundColor>::const_iterator it = VideoColors.find(colorDescription);
        if (it != VideoColors.end())
            return it->second;

        return VideoBackgroundColor::black;
    }

    static YUVColor getVideoYUVColor(VideoBackgroundColor videoBgColor)
    {
        // Fetch video YUV color
        std::map<VideoBackgroundColor, YUVColor>::const_iterator it = VideoYuvColors.find(videoBgColor);
        if (it != VideoYuvColors.end())
            return it->second;

        return DEFAULT_VIDEO_BG_COLOR;
    }
};

}
#endif
