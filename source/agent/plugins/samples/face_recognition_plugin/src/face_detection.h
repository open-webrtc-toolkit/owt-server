// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FACE_DETECTION_H
#define FACE_DETECTION_H

#include <iostream>
#include <string.h>
#include <functional>
#include <vector>
#include <utility>
#include <map>
#include <inference_engine.hpp>

#include <opencv2/opencv.hpp>
#include "ie_common.h"

struct BaseDetection {
  InferenceEngine::ExecutableNetwork net;
  InferenceEngine::InferencePlugin * plugin;
  InferenceEngine::InferRequest::Ptr request;
  std::string  commandLineFlag;
  std::string topoName;
  const int maxBatch;

  BaseDetection(std::string commandLineFlag, std::string topoName, int maxBatch);

  virtual ~BaseDetection() {}

  InferenceEngine::ExecutableNetwork* operator ->() {
      return &net;
  }
  virtual InferenceEngine::CNNNetwork read()  = 0;

  virtual void submitRequest() {
      if (!enabled() || request == nullptr) return;
      request->StartAsync();
  }

  virtual void wait() {
      if (!enabled()|| !request) return;
      request->Wait(InferenceEngine::IInferRequest::WaitMode::RESULT_READY);
  }
  mutable bool enablingChecked = false;
  mutable bool _enabled = false;

  bool enabled() const ;
};

struct Result {
    int label;
    float confidence;
    cv::Rect location;
};

struct FaceDetectionClass : BaseDetection {
    void initialize(const std::string& model_xml_path, const std::string& device_name) ;
    void submitRequest() override;

    void enqueue(const cv::Mat &frame);

    explicit FaceDetectionClass() :
      BaseDetection("", "Face Detection", 1) {}

    InferenceEngine::CNNNetwork read() override;

    void fetchResults();

    std::string input;
    std::string output;
    int maxProposalCount;
    int objectSize;
    int enquedFrames = 0;
    float width = 0;
    float height = 0;
    bool resultsFetched = false;
    std::vector<std::string> labels;
    using BaseDetection::operator=;
    bool need_process=false;

    std::vector<Result> results;
};

struct Load {
    BaseDetection& detector;
    explicit Load(BaseDetection& detector) : detector(detector) { }
    void into(InferenceEngine::InferencePlugin & plg) const;
};

#endif
