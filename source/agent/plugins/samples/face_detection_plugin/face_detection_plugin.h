// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FACE_DETECTION_PLUGIN_H
#define FACE_DETECTION_PLUGIN_H

#include "plugin.h"

#include "face_detection.h"

class FaceDetectionPlugin : public rvaPlugin {
public:
    FaceDetectionPlugin();

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
        m_frame_callback = pCallback;
        return RVA_ERR_OK;
    }

    virtual rvaStatus DeRegisterFrameCallback() {
        m_frame_callback = nullptr;
        return RVA_ERR_OK;
    }

    virtual rvaStatus RegisterEventCallback(rvaEventCallback* pCallback) {
        m_event_callback = pCallback;
        return RVA_ERR_OK;
    }

    virtual rvaStatus DeRegisterEventCallback() {
        m_event_callback = nullptr;
        return RVA_ERR_OK;
    }

    void init();

private:
    rvaFrameCallback *m_frame_callback;
    rvaEventCallback *m_event_callback;

    FaceDetection *m_detection;

    std::thread *m_init_thread;
    bool m_ready;
};

#endif  //FACE_DETECTION_PLUGIN_H
