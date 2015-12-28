/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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

#ifndef RTSPINWRAPPER_H
#define RTSPINWRAPPER_H

#include "MediaFramePipelineWrapper.h"
#include <RtspIn.h>
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>


/*
 * Wrapper class of woogeen_base::RtspIn
 */
class RtspIn : public FrameSource, woogeen_base::RtspInStatusListener {
 public:
  static void Init(v8::Local<v8::Object> exports);
  woogeen_base::RtspIn* me;
  std::string statusMsg;
  boost::mutex statusMutex;

 private:
  RtspIn();
  ~RtspIn();
  static v8::Persistent<v8::Function> constructor;

  uv_async_t async_;
  v8::Persistent<v8::Function> statusCallback_;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void init(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeDestination(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void statusCallback(uv_async_t *handle);
  virtual void notifyStatus(const std::string& message);
};

#endif
