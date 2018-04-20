/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#include "VideoFrameConstructorWrapper.h"

using namespace v8;

Nan::Persistent<Function> VideoFrameConstructor::constructor;

VideoFrameConstructor::VideoFrameConstructor() {};
VideoFrameConstructor::~VideoFrameConstructor() {};

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
  Nan::SetPrototypeMethod(tpl, "requestKeyFrame", requestKeyFrame);

  constructor.Reset(tpl->GetFunction());
  Nan::Set(target, Nan::New("VideoFrameConstructor").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(VideoFrameConstructor::New) {
  if (info.IsConstructCall()) {
    VideoFrameConstructor* obj = new VideoFrameConstructor();
    obj->me = new woogeen_base::VideoFrameConstructor(obj);
    obj->src = obj->me;
    obj->msink = obj->me;

    obj->Callback_ = new Nan::Callback(info[0].As<Function>());
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
  woogeen_base::VideoFrameConstructor* me = obj->me;

  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&obj->async_))) {
    uv_close(reinterpret_cast<uv_handle_t*>(&obj->async_), NULL);
  }

  delete me;
}

NAN_METHOD(VideoFrameConstructor::bindTransport) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  WebRtcConnection* param = Nan::ObjectWrap::Unwrap<WebRtcConnection>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  auto wr = std::shared_ptr<erizo::WebRtcConnection>(param->me).get();

  //MediaSink* param = Nan::ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSource* source = wr;

  me->bindTransport(source, source->getFeedbackSink());
}

NAN_METHOD(VideoFrameConstructor::unbindTransport) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  me->unbindTransport();
}

NAN_METHOD(VideoFrameConstructor::addDestination) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  FrameDestination* param = node::ObjectWrap::Unwrap<FrameDestination>(info[0]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

  me->addVideoDestination(dest);
}

NAN_METHOD(VideoFrameConstructor::removeDestination) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  FrameDestination* param = node::ObjectWrap::Unwrap<FrameDestination>(info[0]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

  me->removeVideoDestination(dest);
}

NAN_METHOD(VideoFrameConstructor::setBitrate) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  int bitrate = info[0]->IntegerValue();

  me->setBitrate(bitrate);
}

NAN_METHOD(VideoFrameConstructor::requestKeyFrame) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  me->RequestKeyFrame();
}

NAN_METHOD(VideoFrameConstructor::enable) {
  VideoFrameConstructor* obj = Nan::ObjectWrap::Unwrap<VideoFrameConstructor>(info.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  bool b = (info[0]->ToBoolean())->BooleanValue();
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
    Nan::MakeCallback(Nan::GetCurrentContext()->Global(), obj->Callback_->GetFunction(), 1, args);
    obj->videoInfoMsgs.pop();
  }
}

