// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <sys/time.h>

#include "AudioTime.h"

namespace mcu {

uint32_t AudioTime::sTimestampOffset = 0;

void AudioTime::setTimestampOffset(uint32_t offset)
{
    sTimestampOffset = offset;
}

int64_t AudioTime::currentTime(void)
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000)) - sTimestampOffset;
}

} /* namespace mcu */
