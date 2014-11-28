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

namespace mcu {

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
    uhd_4k,
    total = uhd_4k,
};

struct VideoSize {
    int width;
    int height;
};

struct Region {
    std::string id;
    float left; // percentage
    float top;    // percentage
    float relativesize;    //fraction
    float priority;
} ;

struct VideoLayout {
    VideoResolutionType rootsize;
    std::vector<Region> regions;
    unsigned int divFactor;    //valid for fluidLayout
    unsigned int subWidth;
    unsigned int subHeight;
};

extern VideoSize VideoSizes[];
extern const char* VideoResString[];

}
#endif
