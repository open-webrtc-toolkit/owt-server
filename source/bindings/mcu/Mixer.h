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

#include <MixerInterface.h>
#include <node.h>
#include <string>

/*
 * Wrapper class of mcu::MixerInterface
 */
class Mixer : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);
  mcu::MixerInterface* me;

 private:
  Mixer();
  ~Mixer();

  /*
   * Constructor.
   * Constructs a Mixer
   */
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  /*
   * Closes the Mixer.
   * The object cannot be used after this call
   */
  static v8::Handle<v8::Value> close(const v8::Arguments& args);
  /*
   * Adds a Publisher
   * Param: the WebRtcConnection of the Publisher
   */
  static v8::Handle<v8::Value> addPublisher(const v8::Arguments& args);
  /*
   * Removes a Publisher
   */
  static v8::Handle<v8::Value> removePublisher(const v8::Arguments& args);
  /*
   * Adds a subscriber
   * Param1: the WebRtcConnection of the subscriber
   * Param2: an unique Id for the subscriber
   */
  static v8::Handle<v8::Value> addSubscriber(const v8::Arguments& args);
  /*
   * Removes a subscriber given its peer id
   * Param: the peerId
   */
  static v8::Handle<v8::Value> removeSubscriber(const v8::Arguments& args);
  /*
   * Adds an ExternalOutput
   * Param: The ExternalOutput uri and options
   */
  static v8::Handle<v8::Value> addExternalOutput(const v8::Arguments& args);
  /*
   * Removes an ExternalOutput given its peer id
   * Param: the peerId
   */
  static v8::Handle<v8::Value> removeExternalOutput(const v8::Arguments& args);
  /*
   * Sets an External Publisher
   * Param: the ExternalInput of the Publisher
   */
  static v8::Handle<v8::Value> addExternalPublisher(const v8::Arguments& args);
};

#endif
