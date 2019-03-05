// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef GATEWAY_H
#define GATEWAY_H

#include "../../access/webrtc/MediaDefinitions.h"
#include "../../access/webrtc/WebRtcConnection.h"
#include "NodeEventRegistry.h"
#include <Gateway.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of owt_base::Gateway
 *
 * Represents connection between WebRTC clients, or between a WebRTC client
 * and other third-party clients.
 * Receives media from the WebRTC client and retransmits it to others,
 * or receives media from other clients and retransmits it to the WebRTC client.
 */
class Gateway : public NodeEventedObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  owt_base::Gateway* me;

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
