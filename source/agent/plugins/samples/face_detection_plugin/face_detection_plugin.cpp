// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <thread>

#include <opencv2/opencv.hpp>

#include "common.h"
#include "face_detection_plugin.h"

using namespace std;

void init_worker(FaceDetectionPlugin *plugin) {
    plugin->init();
}

void FaceDetectionPlugin::init() {
    m_detection = new FaceDetection("CPU");
    m_detection->init();

    m_ready = true;
}

FaceDetectionPlugin::FaceDetectionPlugin()
    : m_frame_callback(NULL)
    , m_event_callback(NULL)
    , m_detection(NULL)
    , m_init_thread(NULL)
    , m_ready(false) {
    dout << endl;

    m_init_thread = new std::thread(init_worker, this);
}

rvaStatus FaceDetectionPlugin::PluginInit(std::unordered_map<std::string, std::string> params) {
    dout << endl;
    return RVA_ERR_OK;
}

rvaStatus FaceDetectionPlugin::PluginClose() {
    dout << endl;
    return RVA_ERR_OK;
}

rvaStatus FaceDetectionPlugin::ProcessFrameAsync(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer) {
    if (!buffer || !buffer->buffer) {
        dout << "Invalid buffer!" << endl;

        if (m_frame_callback) {
            m_frame_callback->OnPluginFrame(std::move(buffer));
        }

        return RVA_ERR_OK;
    }

    if (!m_ready) {
        dout << "Not ready!" << endl;

        if (m_frame_callback) {
            m_frame_callback->OnPluginFrame(std::move(buffer));
        }

        return RVA_ERR_OK;
    }

    dout << "+++" << endl;

    cv::Mat yuv_image(buffer->height + buffer->height / 2, buffer->width, CV_8UC1, buffer->buffer);
    cv::Mat bgr_image(buffer->height, buffer->width, CV_8UC3);
    cv::cvtColor(yuv_image, bgr_image, cv::COLOR_YUV2BGR_I420);

    std::vector<FaceDetection::DetectedObject> result;
    m_detection->infer(bgr_image, result);

    for (auto &obj : result) {
        cv::rectangle(bgr_image, obj.rect, cv::Scalar(255, 255, 0), 2);

        char msg[128];
        snprintf(msg, 128, "conf: %.2f", obj.confidence);

        int font_face = cv::FONT_HERSHEY_COMPLEX_SMALL;
        double font_scale = 1;
        int thickness = 1;
        int baseline;
        cv::Size text_size = cv::getTextSize(msg, font_face, font_scale, thickness, &baseline);

        cv::Point origin;
        origin.y = obj.rect.y - 10;
        if (obj.rect.width > text_size.width)
            origin.x = obj.rect.x + (obj.rect.width - text_size.width) / 2;
        else
            origin.x = obj.rect.x;

        cv::putText(bgr_image,
                msg,
                origin,
                font_face,
                font_scale,
                cv::Scalar(255, 255, 0),
                thickness
                );
    }

    cv::cvtColor(bgr_image, yuv_image, cv::COLOR_BGR2YUV_I420);
    buffer->buffer = yuv_image.data;

    dout << "---" << endl;

    if (m_frame_callback) {
        m_frame_callback->OnPluginFrame(std::move(buffer));
    }

    return RVA_ERR_OK;
}

// Declare the plugin
DECLARE_PLUGIN(FaceDetectionPlugin)

