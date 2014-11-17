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

#include "MediaSourceConsumer.h"

using namespace v8;

MediaSourceConsumer::MediaSourceConsumer() {};
MediaSourceConsumer::~MediaSourceConsumer() {};

void MediaSourceConsumer::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("MediaSourceConsumer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("close"),
      FunctionTemplate::New(close)->GetFunction());

  Persistent<Function> constructor =
      Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("MediaSourceConsumer"), constructor);
}

Handle<Value> MediaSourceConsumer::New(const Arguments& args) {
  HandleScope scope;

  MediaSourceConsumer* obj = new MediaSourceConsumer();
  obj->me = woogeen_base::MediaSourceConsumer::createMediaSourceConsumerInstance();

  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> MediaSourceConsumer::close(const Arguments& args) {
  HandleScope scope;
  MediaSourceConsumer* obj = ObjectWrap::Unwrap<MediaSourceConsumer>(args.This());
  woogeen_base::MediaSourceConsumer* me = obj->me;

  delete me;

  return scope.Close(Null());
}

