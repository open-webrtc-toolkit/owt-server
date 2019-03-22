// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FACE_RECOGNITION_H
#define FACE_RECOGNITION_H

#include <string>
#include <chrono>
#include <cmath>
#include <inference_engine.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class FaceRecognitionClass
{
  public:
      int initialize(const std::string& model_xml_path, const std::string& device_name);
      void load_frame(cv::Mat input_frame);
      std::vector<float> do_infer();
      virtual ~FaceRecognitionClass();
      int get_input_width() { return infer_width; }
      int get_output_size() { return output_size; }
  private:
      unsigned char* input_buffer;
      float* output_buffer;
      int infer_width;
      int infer_height;
      size_t num_channels;
      int channel_size;
      int output_size;

      InferenceEngine::CNNNetReader networkReader;
      InferenceEngine::ExecutableNetwork executable_network;
      InferenceEngine::InferencePlugin plugin;
      InferenceEngine::InferRequest::Ptr   async_infer_request;
      InferenceEngine::InputsDataMap input_info;
      InferenceEngine::OutputsDataMap output_info;
};

#endif
