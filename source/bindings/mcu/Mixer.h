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

#ifndef MIXER_H
#define MIXER_H

#include "../woogeen_base/Gateway.h"
#include "../woogeen_base/NodeEventRegistry.h"

#include <MixerInterface.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of mcu::MixerInterface
 */
class Mixer : public NodeEventedObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  mcu::MixerInterface* me;

 private:
  Mixer();
  ~Mixer();
  static v8::Persistent<v8::Function> constructor;

  /*
   * Constructor.
   * Constructs a Mixer
   */
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Closes the Mixer.
   * The object cannot be used after this call
   */
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Adds a Publisher
   * Param: the WebRtcConnection of the Publisher
   */
  static void addPublisher(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Removes a Publisher
   */
  static void removePublisher(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Adds a subscriber
   * Param1: the WebRtcConnection of the subscriber
   * Param2: an unique Id for the subscriber
   */
  static void addSubscriber(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Removes a subscriber given its peer id
   * Param: the peerId
   */
  static void removeSubscriber(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Enable a subscriber given its peer id
   * Param: peerId, isAudio
   */
  static void subscribeStream(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Disable a subscriber given its peer id
   * Param: peerId, isAudio
   */
  static void unsubscribeStream(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Adds an External Publisher
   * Param: the ExternalInput of the Publisher
   */
  static void addExternalPublisher(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Gets the region of a publisher in the mixer
   * Param: the publisher id
   */
  static void getRegion(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Sets the region of a publisher in the mixer
   * Param1: the publisher id
   * Param2: the region id
   */
  static void setRegion(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Changes the bitrate of a publisher in the mixer
   * Param1: the publisher id
   * Param2: the bitrate
   */
  static void setVideoBitrate(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
