// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef UTILS_H
#define UTILS_H

#include <cstddef>

namespace owt_base {

class Utils {
public:
    // Fill memory with zeros.
    static void ZeroMemory(void* ptr, size_t len);
};

}
#endif