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

#ifndef SIP_GATEWAY_H
#define SIP_GATEWAY_H

#include "../../addons/common/NodeEventRegistry.h"
#include <node.h>
#include <SipGateway.h>
#include <node_object_wrap.h>
#include <string>

/*
 * Wrapper class of sip_gateway::SipGateway
 *
 * Represents connection between WebRTC clients and Sip clients.
 * Receives media from the WebRTC client and retransmits it to Sip client,
 * or receives media from Sip client and retransmits it to the WebRTC client.
 */
class SipGateway : public NodeEventedObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  sip_gateway::SipGateway* me;

 private:
  SipGateway();
  ~SipGateway();

  static v8::Persistent<v8::Function> constructor;

  /*
   * Constructor.
   * Constructs a SipGateway.
   */
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  /*
   * Closes the SipGateway.
   * The object cannot be used after this call.
   */
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  /*
   * Register to sip server.
   * Param: register info. 4 arguments.
   */
  static void sipReg(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void makeCall(const v8::FunctionCallbackInfo<v8::Value>&  args);
  static void hangup(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void accept(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void reject(const v8::FunctionCallbackInfo<v8::Value>& args);
};  // class SipGateway

#endif  // SIP_GATEWAY_H
