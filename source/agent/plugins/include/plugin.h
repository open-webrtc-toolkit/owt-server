/*
 * Copyright 2017 Intel Corporation All Rights Reserved. 
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

#ifndef AnalyticsPlugin_h
#define AnalyticsPlugin_h

#include <unordered_map>
#include <string>

#include "rvadefs.h"
#include "analytics_buffer.h"
//#include "videoframe.h"

class rvaFrameCallback {
 public:
  virtual ~rvaFrameCallback() {}
  /// Frame callback to send back VideoFrame to MCU
  virtual void OnPluginFrame(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer) {};
};

class rvaEventCallback {
 public:
  virtual ~rvaEventCallback() {}
  /// Event callback to send event message to MCU
  virtual void OnPluginEvent(rvaU64 event_id, std::string& message) {};
};

/// Realtime Video Analytics Plugin interface definition shared by MCU and
/// plugin implementation.
class rvaPlugin {
 public:
  /// Constructor
  rvaPlugin() {}
  /**
   @brief Initializes a plugin with provided params after MCU creates the plugin.
   @param params unordered map that contains name-value pair of parameters
   @return RVA_ERR_OK if no issue initialize it. Other return code if any failure.
  */
  virtual rvaStatus PluginInit(std::unordered_map<std::string, std::string> params) = 0;
  /**
   @brief Release internal resources the plugin holds before MCU destroy the plugin.
   @return RVA_ERR_OK if no issue close the plugin. Other return code if any failure.
  */
  virtual rvaStatus PluginClose() = 0;
  /**
   @brief MCU will use this interface to fetch current applied params on the plugin.
   @param params name-value pair will be returned to the MCU provided unordered_map.
   @return RVA_ERR_OK if params are successfull filled in, or empty param is provided. 
           Other return code if any failure.
  */
  virtual rvaStatus GetPluginParams(std::unordered_map<std::string, std::string> &params) = 0; 
  /**
   @brief MCU will use this interface to update params on the plugin.
   @param params name-value pair to be set.
   @return RVA_ERR_OK if params are successfull updated. 
           Other return code if any failure.
  */
  virtual rvaStatus SetPluginParams(std::unordered_map<std::string, std::string> params) = 0;
  /**
   @brief MCU pushes a video frame to the plugin for processing. Note this processing
          must be asynchronous on other thread and should return immediately to caller.
   @param frame the video frame for processing
   @return RVA_ERR_OK if no issue. Other return code if any failure. 
  */
  virtual rvaStatus ProcessFrameAsync(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer) = 0;
  /**
   @brief Register a callback on the plugin for receiving frames from the plugin.
   @param pCallback the frame callback function registered by MCU.
   @return RVA_ERR_OK if no issue. Other return code if any failure. 
  */
  virtual rvaStatus RegisterFrameCallback(rvaFrameCallback* pCallback) = 0;
  /**
   @brief unregister the pre-registered frame callback on the plugin.
   @return RVA_ERR_OK if no issue. Other return code if any failure. 
  */
  virtual rvaStatus DeRegisterFrameCallback() = 0;
  /**
   @brief Register a callback on the plugin for receiving events from the plugin.
   @param pCallback the event callback function registered by MCU.
   @return RVA_ERR_OK if no issue. Other return code if any failure. 
  */
  virtual rvaStatus RegisterEventCallback(rvaEventCallback* pCallback) = 0;
  /**
   @brief unregister the pre-registered event callback on the plugin.
   @return RVA_ERR_OK if no issue. Other return code if any failure. 
  */
  virtual rvaStatus DeRegisterEventCallback() = 0;
};

typedef rvaPlugin* rva_create_t();
typedef void rva_destroy_t(rvaPlugin*);


/// Your plugin implemenation is required to invoke
/// below DECLARE_PLUGIN macro to delcare interfaces
/// to MCU.
#define DECLARE_PLUGIN(className)\
extern "C" {\
  rvaPlugin* CreatePlugin() {\
    return new className;\
  }\
  void DestroyPlugin(rvaPlugin* p) {\
    delete p;\
  }\
}

#endif
