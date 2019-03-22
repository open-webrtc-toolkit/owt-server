// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MYPLUGIN_H
#define MYPLUGIN_H

#include "plugin.h"

// Class definition for the plugin invoked by the sample service.
class MyPlugin : public rvaPlugin {
public:
    MyPlugin();

    virtual rvaStatus PluginInit(std::unordered_map<std::string, std::string> params);

    virtual rvaStatus PluginClose();

    virtual rvaStatus GetPluginParams(std::unordered_map<std::string, std::string>& params) {
        return RVA_ERR_OK;
    }

    virtual rvaStatus SetPluginParams(std::unordered_map<std::string, std::string> params) {
        return RVA_ERR_OK;
    }

    virtual rvaStatus ProcessFrameAsync(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer);

    virtual rvaStatus RegisterFrameCallback(rvaFrameCallback* pCallback) {
        frame_callback = pCallback;
        return RVA_ERR_OK;
    }

    virtual rvaStatus DeRegisterFrameCallback() {
        frame_callback = nullptr;
        return RVA_ERR_OK;
    }

    virtual rvaStatus RegisterEventCallback(rvaEventCallback* pCallback) {
        event_callback = pCallback;
        return RVA_ERR_OK;
    }

    virtual rvaStatus DeRegisterEventCallback() {
        event_callback = nullptr;
        return RVA_ERR_OK;
    }

private:
    rvaFrameCallback* frame_callback;
    rvaEventCallback* event_callback;
};

#endif  //MYPLUGIN_H
