// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VideoFrameConstructorWrapper.h"
#include "CallBaseWrapper.h"

using namespace v8;

Nan::Persistent<Function> VideoFrameConstructor::constructor;

VideoFrameConstructor::VideoFrameConstructor()
  : me(nullptr)
  , src(nullptr)
  , parent(nullptr)
{}

VideoFrameConstructor::~VideoFrameConstructor() {}

NAN_MODULE_INIT(VideoFrameConstructor::Init) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("VideoFrameConstructor").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "close", close);
  Nan::SetPrototypeMethod(tpl, "bindTransport", bindTransport);
  Nan::SetPrototypeMethod(tpl, "unbindTransport", unbindTransport);
  Nan::SetPrototypeMethod(tpl, "enable", enable);
  Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
  Nan::SetPrototypeMethod(tpl, "removeDestination", removeDestination);
  Nan::SetPrototypeMethod(tpl, "setBitrate", setBitrate);
  Nan::SetPrototypeMethod(tpl, "setPreferredLayers", setPreferredLayers);
  Nan::SetPrototypeMethod(tpl, "requestKeyFrame", requestKeyFrame);
  Nan::SetPrototypeMethod(tpl, "source", source);

  constructor.Reset(tpl->GetFunction());
  Nan::Set(target, Nan::New("VideoFrameConstructor").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());

  VideoFrameSource::Init(target);
}

NAN_METHOD(VideoFrameConstructor::New) {
  if (info.IsConstructCall()) {
    VideoFrameConstructor* obj = new VideoFrameConstructor();
    if (info.Length() <= 3) {
      int transportccExt = (info.Length() >= 2) ? info[1]->IntegerValue(Nan::GetCurrentContext()).ToChecked() : -1;
      CallBase* baseWrapper = (info.Length() >= 3)
        ? Nan::ObjectWrap::Unwrap<CallBase>(Nan::To<v8::Object>(info[2]).ToLocalChecked())
        : nullptr;
      if (baseWrapper) {
        obj->me = new owt_base::VideoFrameConstructor(baseWrapper->rtcAdapter, obj, transportccExt);
      } else if (transportccExt > 0) {
        obj->me = new owt_base::VideoFrameConstructor(obj, transportccExt);
      } else {
        obj->me = new owt_base::VideoFrameConstructor(obj);
      }
    } else {
      // CallBase* baseWrapper = Nan::ObjectWrap::Unwrap<CallBase>(
      //   Nan::To<v8::Object>(info[0]).ToLocalChecked());
      VideoFrameConstructor* parent =
        Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(
          Nan::To<v8::Object>(info[1]).ToLocalChecked());
      Nan::Utf8String param2(Nan::To<v8::String>(info[2]).ToLocalChecked());
      std::string layerId = std::string(*param2);
      int spatialId = info[3]->IntegerValue(Nan::GetCurrentContext()).ToChecked();
      int temporalId = info[4]->IntegerValue(Nan::GetCurrentContext()).ToChecked();

      obj->me = new owt_base::VideoFrameConstructor(parent->me);
      obj->me->setPreferredLayers(spatialId, temporalId);
      obj->layerId = layerId;
      obj->parent = parent->me;
      obj->parent->addChildProcessor(layerId, obj->me);
    }

    obj->src = obj->me;
    obj->msink = obj->me;

    obj->Callback_ = new Nan::Callback(info[0].As<Function>());
    obj->asyncResource_ = new Nan::AsyncResource("VideoFrameCallback");
    uv_async_init(uv_default_loop(), &obj->async_, &VideoFrameConstructor::Callback);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    // const int argc = 1;
    // v8::Local<v8::Value> argv[argc] = {info[0]};
    // v8::Local<v8::Function> cons = Nan::New(constructor);
    // info.GetReturnValue().Set(cons->NewInstance(argc, argv));
  }
}

NAN_METHOD(VideoFrameConstructor::close) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  if (obj->parent) {
    obj->parent->removeChildProcessor(obj->layerId);
  }
  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&obj->async_))) {
    uv_close(reinterpret_cast<uv_handle_t*>(&obj->async_), NULL);
  }
  delete obj->asyncResource_;
  delete me;
}

NAN_METHOD(VideoFrameConstructor::bindTransport) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  MediaFilter* param = Nan::ObjectWrap::Unwrap<MediaFilter>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  erizo::MediaSource* source = param->msource;

  me->bindTransport(source, source->getFeedbackSink());
}

NAN_METHOD(VideoFrameConstructor::unbindTransport) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  me->unbindTransport();
}

NAN_METHOD(VideoFrameConstructor::addDestination) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  FrameDestination* param = node::ObjectWrap::Unwrap<FrameDestination>(info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameDestination* dest = param->dest;

  me->addVideoDestination(dest);
}

NAN_METHOD(VideoFrameConstructor::removeDestination) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  FrameDestination* param = node::ObjectWrap::Unwrap<FrameDestination>(info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameDestination* dest = param->dest;

  me->removeVideoDestination(dest);
}

NAN_METHOD(VideoFrameConstructor::setBitrate) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  int bitrate = info[0]->IntegerValue(Nan::GetCurrentContext()).ToChecked();

  me->setBitrate(bitrate);
}

NAN_METHOD(VideoFrameConstructor::setPreferredLayers) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  int spatialId = info[0]->IntegerValue(Nan::GetCurrentContext()).ToChecked();
  int temporalId = info[1]->IntegerValue(Nan::GetCurrentContext()).ToChecked();

  me->setPreferredLayers(spatialId, temporalId);
}

NAN_METHOD(VideoFrameConstructor::requestKeyFrame) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  me->RequestKeyFrame();
}

NAN_METHOD(VideoFrameConstructor::enable) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  owt_base::VideoFrameConstructor* me = obj->me;

  bool b = (info[0]->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked())->BooleanValue();
  me->enable(b);
}

void VideoFrameConstructor::onVideoInfo(const std::string& message) {
  boost::mutex::scoped_lock lock(mutex);
  this->videoInfoMsgs.push(message);
  async_.data = this;
  uv_async_send(&async_);
}

NAUV_WORK_CB(VideoFrameConstructor::Callback) {
  Nan::HandleScope scope;
  VideoFrameConstructor* obj = reinterpret_cast<VideoFrameConstructor*>(async->data);
  if (!obj || obj->me == NULL)
    return;
  boost::mutex::scoped_lock lock(obj->mutex);
  while (!obj->videoInfoMsgs.empty()) {
    Local<Value> args[] = {Nan::New(obj->videoInfoMsgs.front().c_str()).ToLocalChecked()};
    obj->asyncResource_->runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->Callback_->GetFunction(), 1, args);
    obj->videoInfoMsgs.pop();
  }
}

NAN_METHOD(VideoFrameConstructor::source) {
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {info.Holder()};
  v8::Local<v8::Function> cons = Nan::New(VideoFrameSource::constructor);
  info.GetReturnValue().Set(Nan::NewInstance(cons, 1, argv).ToLocalChecked());
}

// Source object for VideoFrameConstructor
Nan::Persistent<Function> VideoFrameSource::constructor;

NAN_MODULE_INIT(VideoFrameSource::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("VideoFrameSource").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  constructor.Reset(tpl->GetFunction());
}

NAN_METHOD(VideoFrameSource::New) {
  if (info.Length() == 1) {
    VideoFrameConstructor* parent = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(
      info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
    VideoFrameSource* obj = new VideoFrameSource();
    obj->me = parent->me;
    obj->src = obj->me;
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }
}
