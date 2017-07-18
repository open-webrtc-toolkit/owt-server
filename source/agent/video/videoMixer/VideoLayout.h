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

#ifndef VideoLayout_h
#define VideoLayout_h

#include "VideoHelper.h"

#include <list>

namespace mcu {

/**
 * the configuration of VideoLayout element definition
 *    An example of a video layout with 5 regions is:

      +-------+---+
      |       | 2 |
      |   1   +---+
      |       | 3 |
      +---+---+---+
      |   5   | 4 |
      +---+---+---+

      <videolayout type="text/msml-basic-layout">
         <root size="CIF"/>
         <region id="1" shape="rectangle" area.top="0" area.left="0", area.width="2/3", area.height="2/3"/>
         <region id="2" shape="rectangle" area.top="0" area.left="2/3", area.width="1/3", area.height="1/3"/>
         <region id="3" shape="rectangle" area.top="1/3" area.left="2/3", area.width="1/3", area.height="1/3"/>
         <region id="4" shape="rectangle" area.top="2/3" area.left="2/3", area.width="1/3", area.height="1/3"/>
         <region id="5" shape="rectangle" area.top="2/3" area.left="0", area.width="2/3", area.height="1/3"/>
      </videolayout>
 */

struct Rational {
  int numerator;
  int denominator;
};

struct Rectangle {
  Rational left; // percentage
  Rational top; // percentage
  Rational width; // percentage
  Rational height; // percentage
};

struct Circle {
  Rational centerW; // percentage
  Rational centerH; // percentage
  Rational radius; // percentage
};

union Shape {
  Rectangle rect;
  Circle circle;
};

struct Region {
    Region()
        : left(0.0)
        , top(0.0)
        , relativeSize(0.0)
    {
    }

    std::string id;
    float left; // percentage, deprecated
    float top;    // percentage, deprecated
    float relativeSize;    //fraction, deprecated
    //float priority;

    std::string shape; // shape of region
    Shape area; // shape area
};

struct InputRegion {
    int input;
    Region region;
};

typedef std::list<InputRegion> LayoutSolution;

class LayoutConsumer {
public:
    virtual void updateLayoutSolution(LayoutSolution& solution) = 0;
};

// Default video layout configuration
const woogeen_base::VideoSize DEFAULT_VIDEO_SIZE = {640, 480};
const woogeen_base::YUVColor DEFAULT_VIDEO_BG_COLOR = {0x00, 0x80, 0x80};

}
#endif
