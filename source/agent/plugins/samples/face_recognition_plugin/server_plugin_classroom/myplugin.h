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
#ifndef MYPLUGIN_H
#define MYPLUGIN_H
#include "plugin.h"
#include <iostream>
#include <string.h>
#include <functional>
#include <vector>
#include <utility>
#include <map>
#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>


class threading_class
{
    public:
        threading_class();
        void make_thread();
    private:
        void threading_func(); /* { std::cout << "Hello\n"; }*/
};

struct recog_result{
    cv::Rect rectangle;
    float value;
    std::string name;
};

struct action_result{
  cv::Rect rectangle;
  float value;
  std::string label;
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
};

#endif  //MYPLUGIN_H
