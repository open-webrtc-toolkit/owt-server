// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef INTERNAL_QUIC_H_
#define INTERNAL_QUIC_H_

#include "QuicTransport.h"
#include "../common/NodeEventRegistry.h"
#include <node.h>
#include <node_object_wrap.h>

#include <uv.h>
#include <nan.h>
/*
 * Wrapper class of QuicIn
 *
 * Receives media from one
 */
class QuicTransportIn: public node::ObjectWrap, public NodeEventRegistry {
 public:
  static void Init(v8::Local<v8::Object> exports);
  QuicIn* me;

 private:
  QuicTransportIn();
  ~QuicTransportIn();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void getListeningPort(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void send(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addEventListener(const v8::FunctionCallbackInfo<v8::Value>& args);
};

/*
 * Wrapper class of QuicOut
 *
 * Send media from one
 */
class QuicTransportOut: public node::ObjectWrap, public NodeEventRegistry {
 public:
  static void Init(v8::Local<v8::Object> exports);
  QuicOut* me;

 private:
  QuicTransportOut();
  ~QuicTransportOut();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void send(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addEventListener(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif  // INTERNAL_QUIC_H_
