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

#ifndef GATEWAY_H
#define GATEWAY_H

#include "../../access/webrtc/MediaDefinitions.h"
#include "../../access/webrtc/WebRtcConnection.h"
#include "NodeEventRegistry.h"
#include <Gateway.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of woogeen_base::Gateway
 *
 * Represents connection between WebRTC clients, or between a WebRTC client
 * and other third-party clients.
 * Receives media from the WebRTC client and retransmits it to others,
 * or receives media from other clients and retransmits it to the WebRTC client.
 */
class Gateway : public NodeEventedObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  woogeen_base::Gateway* me;

private:
  Gateway();
  ~Gateway();
  static v8::Persistent<v8::Function> constructor;

  /*
   * Constructor.
   * Constructs a Gateway
   */
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Closes the Gateway.
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
   * pass custom message
   */
  static void customMessage(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Gets the gateway statistics
   */
  static void retrieveStatistics(const v8::FunctionCallbackInfo<v8::Value>& args);

  /*
   * Start/stop subscribing video or audio
   */
  static void subscribeStream(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void unsubscribeStream(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Start/stop publishing video or audio
   */
  static void publishStream(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void unpublishStream(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
