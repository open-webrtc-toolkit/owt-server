// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AnaltyicsBuffer_H
#define AnaltyicsBuffer_H

#include <memory>

namespace owt {
namespace analytics {

//compact buffer of I420a, size must be at least width*height*3/2
struct AnalyticsBuffer {
 uint8_t * buffer;
 int width;
 int height;

 AnalyticsBuffer() : buffer(nullptr), width(0), height(0) {}

 ~AnalyticsBuffer() {
     if (buffer != nullptr) {
         delete [] buffer;
         buffer = nullptr; }
     }
};

}
}
#endif
