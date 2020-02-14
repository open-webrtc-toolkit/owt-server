// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SIP_CALLCONNECTION_H
#define SIP_CALLCONNECTION_H

#include <node.h>
#include <SipCallConnection.h>
#include <node_object_wrap.h>
#include <string>

/*
 * Wrapper class of sip_gateway::SipCallConnection
 *
 * Represents connection between WebRTC clients and Sip clients.
 * Receives media from the WebRTC client and retransmits it to Sip client,
 * or receives media from Sip client and retransmits it to the WebRTC client.
 */
class SipCallConnection : public MediaFilter {
 public:
  static void Init(v8::Local<v8::Object> exports);
  sip_gateway::SipCallConnection* me;

 private:
  SipCallConnection();
  ~SipCallConnection();

  static v8::Persistent<v8::Function> constructor;

  /*
   * Constructor.
   * Constructs a SipCallConnection.
   */
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  /*
   * Closes the SipCallConnection.
   * The object cannot be used after this call.
   */
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);


  static void setAudioReceiver(const v8::FunctionCallbackInfo<v8::Value>& args);
  /*
   * Sets a MediaReceiver that is going to receive Video Data
   * Param: the MediaReceiver
   */
  static void setVideoReceiver(const v8::FunctionCallbackInfo<v8::Value>& args);

};  // class SipCallConnection

#endif  // SIP_CALLCONNECTION_H
