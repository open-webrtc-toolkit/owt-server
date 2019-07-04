// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include <opencv2/opencv.hpp>

#include "common.h"
#include "face_detection.h"

using namespace std;
using namespace InferenceEngine;

static cv::Rect TruncateToValidRect(const cv::Rect& rect,
                             const cv::Size& size) {
    auto tl = rect.tl(), br = rect.br();
    tl.x = std::max(0, std::min(size.width - 1, tl.x));
    tl.y = std::max(0, std::min(size.height - 1, tl.y));
    br.x = std::max(0, std::min(size.width, br.x));
    br.y = std::max(0, std::min(size.height, br.y));
    int w = std::max(0, br.x - tl.x);
    int h = std::max(0, br.y - tl.y);
    return cv::Rect(tl.x, tl.y, w, h);
}

static cv::Rect IncreaseRect(const cv::Rect& r, float coeff_x,
                      float coeff_y)  {
    cv::Point2f tl = r.tl();
    cv::Point2f br = r.br();
    cv::Point2f c = (tl * 0.5f) + (br * 0.5f);
    cv::Point2f diff = c - tl;
    cv::Point2f new_diff{diff.x * coeff_x, diff.y * coeff_y};
    cv::Point2f new_tl = c - new_diff;
    cv::Point2f new_br = c + new_diff;

    cv::Point new_tl_int {static_cast<int>(std::floor(new_tl.x)), static_cast<int>(std::floor(new_tl.y))};
    cv::Point new_br_int {static_cast<int>(std::ceil(new_br.x)), static_cast<int>(std::ceil(new_br.y))};

    return cv::Rect(new_tl_int, new_br_int);
}

FaceDetection::FaceDetection(std::string device)
    : m_threshold(0.4) {
    dout << "Cons device: "<< device << endl;

    if (!device.compare("CPU")) {
        m_device = "CPU";
        m_model = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP32/face-detection-retail-0004.xml";
    } else if (!device.compare("GPU")) {
        m_device = "GPU";
        m_model = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP32/face-detection-retail-0004.xml";
    } else if (!device.compare("HDDL")) {
        m_device = "HDDL";
        m_model = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP16/face-detection-retail-0004.xml";
    } else {
        m_device = "CPU";
        m_model = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP32/face-detection-retail-0004.xml";
    }
}

FaceDetection::~FaceDetection() {
    dout << endl;
}

bool FaceDetection::init() {
    dout << "+++" << endl;

    loadModel(m_device, m_model, &m_plugin, &m_infer_request, &m_input_blob, &m_output_blob);

    dout << "---" << endl;
    return true;
}

bool FaceDetection::infer(cv::Mat& image, std::vector<DetectedObject> &result)
{
    int image_width = image.cols;
    int image_height = image.rows;

    matU8ToBlob<uint8_t>(image, m_input_blob);

    dout << "+++do_infer" << endl;
    m_infer_request.Infer();
    dout << "---do_infer" << endl;

    const float *detections = m_output_blob->buffer().as<float *>();

    SizeVector output_dims = m_output_blob->getTensorDesc().getDims();
    int max_detections_count = output_dims[2];
    int object_size = output_dims[3];

    for (int det_id = 0; det_id < max_detections_count; ++det_id) {
        const int start_pos = det_id * object_size;

        const float batchID = detections[start_pos];

#define SSD_EMPTY_DETECTIONS_INDICATOR -1.0
        if (batchID == SSD_EMPTY_DETECTIONS_INDICATOR) {
            break;
        }

        const float score = std::min(std::max(0.0f, detections[start_pos + 2]), 1.0f);
        const float x0 =
            std::min(std::max(0.0f, detections[start_pos + 3]), 1.0f) * image_width;
        const float y0 =
            std::min(std::max(0.0f, detections[start_pos + 4]), 1.0f) * image_height;
        const float x1 =
            std::min(std::max(0.0f, detections[start_pos + 5]), 1.0f) * image_width;
        const float y1 =
            std::min(std::max(0.0f, detections[start_pos + 6]), 1.0f) * image_height;

        DetectedObject object;
        object.confidence = score;
        object.rect = cv::Rect(cv::Point(round(x0), round(y0)),
                cv::Point(round(x1), round(y1)));

        object.rect = TruncateToValidRect(IncreaseRect(object.rect,
                    1,
                    1),
                cv::Size(image_width, image_height));

        if (object.confidence >= m_threshold && object.rect.area() > 0) {
            dout << "Detect confidence: " << object.confidence << ", rect:  " << object.rect << endl;
            result.emplace_back(object);
        }
    }

    return true;
}

