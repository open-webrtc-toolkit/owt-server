// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_H
#define CONFIG_H

#ifdef _WIN32
#include <os/windows/w_dirent.h>
#else
#include <dirent.h>
#endif

/// @brief model full path used by face recognition plugin 
static const char face_detection_model_full_path_fp32[] = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP32/face-detection-retail-0004.xml";

static const char face_detection_model_full_path_fp16[] = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP16/face-detection-retail-0004.xml";

static const char face_recognition_model_full_path_fp32[] = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-reidentification-retail-0095/FP32/face-reidentification-retail-0095.xml";

static const char face_recognition_model_full_path_fp16[] = "/opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-reidentification-retail-0095/FP16/face-reidentification-retail-0095.xml";
 
#endif
