/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#ifndef EXTERNALOUTPUT_H
#define EXTERNALOUTPUT_H

#include <MediaMuxer.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of woogeen_base::MediaMuxer
 * Note: MediaMuxer instances are managed by the native layer
 */
class ExternalOutput : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> exports);
  woogeen_base::MediaMuxer* me;

 private:
  ExternalOutput();
  ~ExternalOutput();
  static v8::Persistent<v8::Function> constructor;

  /*
   * Constructor.
   * Constructs an ExternalOutput
   */
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Closes the ExternalOutput.
   * The object cannot be used after this call
   */
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  /*
   * Sets MediaSources that provide both video and audio frames
   * Param: the Gateway/Mixer to provide frames.
   */
  static void setMediaSource(const v8::FunctionCallbackInfo<v8::Value>& args);

  /*
   * Unset MediaSource that provide both video and audio frames
   * Param: the MediaMuxer to unset, and the flag of recycle
   */
  static void unsetMediaSource(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
