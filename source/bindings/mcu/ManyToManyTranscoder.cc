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

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "node.h"
#include "ManyToManyTranscoder.h"
#include <MCU.h>

using namespace v8;

ManyToManyTranscoder::ManyToManyTranscoder() {};
ManyToManyTranscoder::~ManyToManyTranscoder() {};

void ManyToManyTranscoder::Init(Handle<Object> target) {
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    tpl->SetClassName(String::NewSymbol("ManyToManyTranscoder"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    // Prototype
    tpl->PrototypeTemplate()->Set(String::NewSymbol("close"), FunctionTemplate::New(close)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("addPublisher"), FunctionTemplate::New(addPublisher)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("removePublisher"), FunctionTemplate::New(removePublisher)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("addSubscriber"), FunctionTemplate::New(addSubscriber)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("removeSubscriber"), FunctionTemplate::New(removeSubscriber)->GetFunction());

    Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
    target->Set(String::NewSymbol("ManyToManyTranscoder"), constructor);
}

Handle<Value> ManyToManyTranscoder::New(const Arguments& args) {
  HandleScope scope;

  ManyToManyTranscoder* obj = new ManyToManyTranscoder();
  obj->me = new mcu::MCU();

  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> ManyToManyTranscoder::close(const Arguments& args) {
  HandleScope scope;

  ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
  mcu::MCU *me = obj->me;
  delete me;

  return scope.Close(Null());
}

Handle<Value> ManyToManyTranscoder::addPublisher(
    const Arguments& args) {
  HandleScope scope;
  ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
  mcu::MCU *me = (mcu::MCU*)obj->me;

  WebRtcConnection* param = ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  erizo::WebRtcConnection* wr = (erizo::WebRtcConnection*)param->me;

  erizo::MediaSource* ms = dynamic_cast<erizo::MediaSource*>(wr);
  me->addPublisher(ms);

  return scope.Close(Null());
}

Handle<Value> ManyToManyTranscoder::removePublisher(
    const Arguments& args) {
  HandleScope scope;
  ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
  mcu::MCU *me = (mcu::MCU*)obj->me;

  WebRtcConnection* param = ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  erizo::WebRtcConnection* wr = (erizo::WebRtcConnection*)param->me;

  erizo::MediaSource* ms = dynamic_cast<erizo::MediaSource*>(wr);
  me->removePublisher(ms);

  return scope.Close(Null());
}

Handle<Value> ManyToManyTranscoder::addSubscriber(
    const Arguments& args) {
  HandleScope scope;
  ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
  mcu::MCU *me = (mcu::MCU*)obj->me;

  WebRtcConnection* param = ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  erizo::WebRtcConnection* wr = (erizo::WebRtcConnection*)param->me;

  erizo::MediaSink* ms = dynamic_cast<erizo::MediaSink*>(wr);

  // get the param
  v8::String::Utf8Value param1(args[1]->ToString());

  // convert it to string
  std::string peerId = std::string(*param1);

  me->addSubscriber(ms, peerId);

  return scope.Close(Null());
}

Handle<Value> ManyToManyTranscoder::removeSubscriber(
    const Arguments& args) {
  HandleScope scope;
  ManyToManyTranscoder* obj = ObjectWrap::Unwrap<ManyToManyTranscoder>(args.This());
  mcu::MCU *me = (mcu::MCU*)obj->me;

  // get the param
  v8::String::Utf8Value param1(args[0]->ToString());

  // convert it to string
  std::string peerId = std::string(*param1);
  me->removeSubscriber(peerId);

  return scope.Close(Null());
}
