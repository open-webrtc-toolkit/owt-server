// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <string.h>
#include "myplugin.h"

MyPlugin::MyPlugin()
: frame_callback(nullptr)
, event_callback(nullptr) {}

rvaStatus MyPlugin::PluginInit(std::unordered_map<std::string, std::string> params) {
    std::cout << "In my plugin init." << std::endl;
    return RVA_ERR_OK;
}

rvaStatus MyPlugin::PluginClose() {
    return RVA_ERR_OK;
}

rvaStatus MyPlugin::ProcessFrameAsync(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer) {
    if (!buffer->buffer) {
        return RVA_ERR_OK;
    }
    if (buffer->width >=320 && buffer->height >=240) {
        memset(buffer->buffer + buffer->width * buffer->height, 28, buffer->width * buffer->height/16);
    }
    if (frame_callback) {
        frame_callback->OnPluginFrame(std::move(buffer));
    }
    return RVA_ERR_OK;
} 

// Declare the plugin 
DECLARE_PLUGIN(MyPlugin)

