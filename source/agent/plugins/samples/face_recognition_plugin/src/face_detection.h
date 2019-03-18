/*
 * Copyright 2018 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#include <iostream>
#include <string.h>
#include <functional>
#include <vector>
#include <utility>
#include <map>
#include <inference_engine.hpp>
//#include "interactive_face_detection.hpp"
#include <opencv2/opencv.hpp>
#include "common_ie.hpp"

#if 1
template <typename T>
void matU8ToBlob(const cv::Mat& orig_image, InferenceEngine::Blob::Ptr& blob, float scaleFactor = 1.0, int batchIndex = 0) {
    InferenceEngine::SizeVector blobSize = blob.get()->dims();
    const size_t width = blobSize[0];
    const size_t height = blobSize[1];
    const size_t channels = blobSize[2];
    T* blob_data = blob->buffer().as<T*>();

    cv::Mat resized_image(orig_image);
    if (width != orig_image.size().width || height!= orig_image.size().height) {
        cv::resize(orig_image, resized_image, cv::Size(width, height));
    }

    int batchOffset = batchIndex * width * height * channels;

    for (size_t c = 0; c < channels; c++) {
        for (size_t  h = 0; h < height; h++) {
            for (size_t w = 0; w < width; w++) {
                blob_data[batchOffset + c * width * height + h * width + w] =
                    resized_image.at<cv::Vec3b>(h, w)[c] * scaleFactor;
            }
        }
    }
}
#endif



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

  void printPerformanceCounts();

};

struct Result {
    int label;
    float confidence;
    cv::Rect location;
};

struct FaceDetectionClass : BaseDetection {
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

    void initialize() ;
    void submitRequest() override;

    void enqueue(const cv::Mat &frame);


    explicit FaceDetectionClass() : 
      BaseDetection("/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP32/face-detection-retail-0004.xml"\
      ,"Face Detection"
      , 1) {}


    InferenceEngine::CNNNetwork read() override;

    void fetchResults();
};

struct Load {
    BaseDetection& detector;
    explicit Load(BaseDetection& detector) : detector(detector) { }
    void into(InferenceEngine::InferencePlugin & plg) const;
};

