// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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
