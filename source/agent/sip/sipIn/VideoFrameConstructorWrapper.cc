// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VideoFrameConstructorWrapper.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "SipCallConnection.h"
#include "../../addons/common/NodeEventRegistry.h"

using namespace v8;

Persistent<Function> VideoFrameConstructor::constructor;
VideoFrameConstructor::VideoFrameConstructor() {};
VideoFrameConstructor::~VideoFrameConstructor() {};

void VideoFrameConstructor::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "VideoFrameConstructor"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);

  NODE_SET_PROTOTYPE_METHOD(tpl, "bindTransport", bindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unbindTransport", unbindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setBitrate", setBitrate);
  NODE_SET_PROTOTYPE_METHOD(tpl, "requestKeyFrame", requestKeyFrame);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addEventListener", addEventListener);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "VideoFrameConstructor"), tpl->GetFunction());
}

void VideoFrameConstructor::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = new VideoFrameConstructor();
  obj->me = new owt_base::VideoFrameConstructor(obj);
  obj->src = obj->me;
  obj->msink = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void VideoFrameConstructor::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;
  delete me;
}

void VideoFrameConstructor::bindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  SipCallConnection* param = ObjectWrap::Unwrap<SipCallConnection>(args[0]->ToObject());
  sip_gateway::SipCallConnection* transport = param->me;

  me->bindTransport(transport, transport);
}

void VideoFrameConstructor::unbindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  me->unbindTransport();
}

void VideoFrameConstructor::addDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[0]->ToObject());
  owt_base::FrameDestination* dest = param->dest;

  me->addVideoDestination(dest);
}

void VideoFrameConstructor::removeDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[0]->ToObject());
  owt_base::FrameDestination* dest = param->dest;

  me->removeVideoDestination(dest);
}

void VideoFrameConstructor::setBitrate(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  int bitrate = args[0]->IntegerValue();

  me->setBitrate(bitrate);
}

void VideoFrameConstructor::requestKeyFrame(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  me->RequestKeyFrame();
}

void VideoFrameConstructor::onVideoInfo(const std::string& message) {
    notifyAsyncEvent("mediaInfo", message);
}

void VideoFrameConstructor::addEventListener(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }
    VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
    if (!obj->me)
        return;
    Local<Object>::New(isolate, obj->m_store)->Set(args[0], args[1]);
}

