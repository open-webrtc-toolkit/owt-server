// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MYPLUGIN_H
#define MYPLUGIN_H

#include <string.h>
#include <functional>
#include <vector>
#include <utility>
#include <unordered_map>
#include <map>

#include <inference_engine.hpp>
#include <ext_list.hpp>
#include <opencv2/opencv.hpp>

#include "detectionconfig.h"
#include "plugin.h"

// Copied from oVINO sample
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

static std::string fileNameNoExt(const std::string &filepath) {
    auto pos = filepath.rfind('.');
    if (pos == std::string::npos) return filepath;
    return filepath.substr(0, pos);
}

inline std::string fileExt(const std::string& filename) {
    auto pos = filename.rfind('.');
    if (pos == std::string::npos) return "";
    return filename.substr(pos + 1);
}

class threading_class
{
  public:
      threading_class();
      void make_thread();
  private:
      void threading_func();
};

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

    void submitRequest() override;

    void enqueue(const cv::Mat &frame);

    explicit FaceDetectionClass() : 
      BaseDetection(model_full_path_fp32,"Face Detection" , 1) {}

    InferenceEngine::CNNNetwork read() override;

    void fetchResults();
};

struct Load {
    BaseDetection& detector;
    explicit Load(BaseDetection& detector) : detector(detector) { }
    void into(InferenceEngine::InferencePlugin & plg) const;
};

struct threading_struct{
  bool started=false;
  bool need_process=false;
};

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
  //main initialization function for this plugin
  void load_init();
  //main image process function
  cv::Mat process(cv::Mat mYUV,int width, int height);
  void* inference_async(void*);
};

#endif  //MYPLUGIN_H
