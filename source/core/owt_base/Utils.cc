// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*
 *  Copyright 2017 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <cstring>

#include <Utils.h>

namespace owt_base {

void Utils::ZeroMemory(void* ptr, size_t len)
{
// Implementation of this method is copied from https://source.chromium.org/chromium/chromium/src/+/master:third_party/webrtc/rtc_base/zero_memory.cc;drc=5b32f238f3a20d00122b2335d9cf7faa9d29c2dd;l=23.
#ifdef WIN32
    SecureZeroMemory(ptr, len);
#else
    memset(ptr, 0, len);
    __asm__ __volatile__(""
                         :
                         : "r"(ptr)
                         : "memory");
#endif
}
}