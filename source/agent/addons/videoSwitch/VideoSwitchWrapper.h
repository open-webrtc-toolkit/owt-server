// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VIDEOSWITCHWRAPPER_H
#define VIDEOSWITCHWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <VideoQualitySwitch.h>
#include <nan.h>

#include <memory>

/*
 * Wrapper class of owt_base::VideoQualitySwitch
 */
class VideoSwitch : public FrameSource {
public:
    static NAN_MODULE_INIT(Init);
    std::shared_ptr<owt_base::VideoQualitySwitch> me;

private:
    VideoSwitch();
    ~VideoSwitch();

    static Nan::Persistent<v8::Function> constructor;

    static NAN_METHOD(New);
    static NAN_METHOD(close);
    static NAN_METHOD(setTargetBitrate);

    static NAN_METHOD(addDestination);
    static NAN_METHOD(removeDestination);
};

#endif
