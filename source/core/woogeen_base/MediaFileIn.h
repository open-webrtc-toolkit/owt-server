// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MediaFileIn_h
#define MediaFileIn_h

#include "MediaFramePipeline.h"

namespace woogeen_base {

class MediaFileIn : public FrameSource {
public:
    void onFeedback(const FeedbackMsg& msg) {};
};

}
#endif /* MediaFileIn_h */
