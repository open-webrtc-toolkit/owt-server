// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef PERSON_DETECTION_H
#define PERSON_DETECTION_H

#include <string>
#include <chrono>
#include <cmath>
#include <inference_engine.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class PersonDetectionClass
{
  public:
    int initialize(std::string model_path);
    void load_frame(cv::Mat input_frame);
    std::vector<float> do_infer();
    virtual ~PersonDetectionClass();

  private:
    cv::Mat input_frame;
    float * input_buffer;
    float * output_buffer;
    cv::Mat* output_frames;
    cv::Mat frameInfer;
    cv::Mat frameInfer_prewhitten;
    InferenceEngine::CNNNetReader networkReader;
    InferenceEngine::ExecutableNetwork executable_network;
    InferenceEngine::InferencePlugin plugin;
    InferenceEngine::InferRequest::Ptr async_infer_request;
    InferenceEngine::InputsDataMap input_info;
    InferenceEngine::OutputsDataMap output_info;
};

#endif
