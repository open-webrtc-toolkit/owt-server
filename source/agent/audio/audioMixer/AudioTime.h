// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioTime_h
#define AudioTime_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace mcu {

class AudioTime {

public:
    static int64_t currentTime(void); //Millisecond
    static void setTimestampOffset(uint32_t offset);

private:
    static uint32_t sTimestampOffset;

};

} /* namespace mcu */

#endif /* AudioTime_h */
