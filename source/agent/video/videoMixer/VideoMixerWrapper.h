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

#ifndef VIDEOMIXERWRAPPER_H
#define VIDEOMIXERWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "VideoMixer.h"
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>

/*
 * Wrapper class of mcu::VideoMixer
 */
class VideoMixer : public node::ObjectWrap {
 public:
  DECLARE_LOGGER();
  static void Init(v8::Handle<v8::Object>, v8::Handle<v8::Object>);
  mcu::VideoMixer* me;

 private:
  VideoMixer();
  ~VideoMixer();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void addInput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeInput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void setInputActive(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addOutput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void setRegion(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void getRegion(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void setPrimary(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void getCurrentRegions(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
