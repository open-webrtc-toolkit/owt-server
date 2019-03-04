// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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
  uint32_t numerator;
  uint32_t denominator;
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
    std::string id;
    std::string shape; // shape of region
    Shape area; // shape area
};

struct InputRegion {
    int input;
    Region region;
};

typedef std::list<InputRegion> LayoutSolution;

// Default video layout configuration
const woogeen_base::VideoSize DEFAULT_VIDEO_SIZE = {640, 480};
const woogeen_base::YUVColor DEFAULT_VIDEO_BG_COLOR = {0x00, 0x80, 0x80};

}
#endif
