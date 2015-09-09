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

#include "ExternalInput.h"
#include "../erizoAPI/MediaDefinitions.h"
#include "../erizoAPI/WebRtcConnection.h"

#include <Gateway.h>
#include <node.h>
#include <string>

/*
 * Wrapper class of woogeen_base::Gateway
 *
 * Represents connection between WebRTC clients, or between a WebRTC client
 * and other third-party clients.
 * Receives media from the WebRTC client and retransmits it to others,
 * or receives media from other clients and retransmits it to the WebRTC client.
 */
class Gateway : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);
  woogeen_base::Gateway* me;

 private:
  Gateway();
  ~Gateway();

  /*
   * Constructor.
   * Constructs a Gateway
   */
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  /*
   * Closes the Gateway.
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
   * Adds an External Publisher
   * Param: the ExternalInput of the Publisher
   */
  static v8::Handle<v8::Value> addExternalPublisher(const v8::Arguments& args);

  /*
   * add Event Listener
   */
  static v8::Handle<v8::Value> addEventListener(const v8::Arguments& args);
  /*
   * pass custom message
   */
  static v8::Handle<v8::Value> customMessage(const v8::Arguments& args);
  /*
   * Gets the gateway statistics
   */
  static v8::Handle<v8::Value> retrieveStatistics(const v8::Arguments& args);

  /*
   * Start/stop subscribing video or audio
   */
  static v8::Handle<v8::Value> subscribeStream(const v8::Arguments& args);
  static v8::Handle<v8::Value> unsubscribeStream(const v8::Arguments& args);
  /*
   * Start/stop publishing video or audio
   */
  static v8::Handle<v8::Value> publishStream(const v8::Arguments& args);
  static v8::Handle<v8::Value> unpublishStream(const v8::Arguments& args);
};

#endif
