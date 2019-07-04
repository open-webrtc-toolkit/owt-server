// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FACE_DETECTION_H
#define FACE_DETECTION_H

#include <inference_engine.hpp>

using namespace InferenceEngine;

class FaceDetection {
public:
    struct DetectedObject {
        cv::Rect rect;
        float confidence;
    };

public:
    FaceDetection(std::string device = "CPU");
    virtual ~FaceDetection();

    bool init();

    bool infer(cv::Mat& image, std::vector<DetectedObject> &result);

private:
    float m_threshold;

    std::string m_device;
    std::string m_model;

    InferencePlugin m_plugin;
    InferRequest m_infer_request;

    Blob::Ptr m_input_blob;
    Blob::Ptr m_output_blob;
};

#endif //FACE_DETECTION_H
