// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AnalyticsPlugin_h
#define AnalyticsPlugin_h

#include <unordered_map>
#include <string>

#include "rvadefs.h"
#include "analytics_buffer.h"

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
