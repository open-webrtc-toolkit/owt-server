// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "InternalClientWrapper.h"

using namespace v8;

DEFINE_LOGGER(InternalClient, "InternalClientWrapper");

Nan::Persistent<Function> InternalClient::constructor;

InternalClient::InternalClient() {
  async_stats_ = new uv_async_t;
  stats_callback_ = nullptr;
  uv_async_init(uv_default_loop(), async_stats_, &InternalClient::statsCallback);
}

static void destroyAsyncHandle(uv_handle_t *handle) {
  delete handle;
  handle = nullptr;
}

InternalClient::~InternalClient() {
  boost::mutex::scoped_lock lock(stats_lock);
  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(async_stats_))) {
    ELOG_DEBUG("Closing Stats handle");
    uv_close(reinterpret_cast<uv_handle_t*>(async_stats_), destroyAsyncHandle);
  }
  async_stats_ = nullptr;
}

NAN_MODULE_INIT(InternalClient::Init) {
  // Constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("InternalClient").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "close", close);
  Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
  Nan::SetPrototypeMethod(tpl, "removeDestination", removeDestination);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("InternalClient").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(InternalClient::New) {
  if (info.Length() < 4) {
    Nan::ThrowError("Wrong number of arguments");
  }

  if (info.IsConstructCall()) {
    Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string streamId = std::string(*param0);

    Nan::Utf8String param1(Nan::To<v8::String>(info[1]).ToLocalChecked());
    std::string protocol = std::string(*param1);

    Nan::Utf8String param2(Nan::To<v8::String>(info[2]).ToLocalChecked());
    std::string ip = std::string(*param2);

    unsigned int port = Nan::To<unsigned int>(info[3]).FromJust();

    InternalClient* obj = new InternalClient();
    obj->me = new owt_base::InternalClient(
        streamId, protocol, ip, port, obj);
    if (info.Length() > 4 && info[4]->IsFunction()) {
      obj->stats_callback_ = new Nan::Callback(info[4].As<Function>());
    }
    obj->src = obj->me;
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    ELOG_WARN("Not construct call");
  }
}

NAN_METHOD(InternalClient::close) {
  InternalClient* obj = ObjectWrap::Unwrap<InternalClient>(info.Holder());
  obj->src = nullptr;
  delete obj->me;
  obj->me = nullptr;
}

NAN_METHOD(InternalClient::addDestination) {
  InternalClient* obj = ObjectWrap::Unwrap<InternalClient>(info.Holder());
  owt_base::InternalClient* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string track = std::string(*param0);

  FrameDestination* param =
    ObjectWrap::Unwrap<FrameDestination>(
      info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->addAudioDestination(dest);
  } else if (track == "video") {
    me->addVideoDestination(dest);
  }
}

NAN_METHOD(InternalClient::removeDestination) {
  InternalClient* obj = ObjectWrap::Unwrap<InternalClient>(info.Holder());
  owt_base::InternalClient* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string track = std::string(*param0);

  FrameDestination* param =
    ObjectWrap::Unwrap<FrameDestination>(
      info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->removeAudioDestination(dest);
  } else if (track == "video") {
    me->removeVideoDestination(dest);
  }
}

NAUV_WORK_CB(InternalClient::statsCallback) {
  Nan::HandleScope scope;
  InternalClient* obj = reinterpret_cast<InternalClient*>(async->data);
  if (!obj || !obj->me || !obj->stats_callback_) {
    return;
  }

  boost::mutex::scoped_lock lock(obj->stats_lock);
  while (!obj->stats_messages.empty()) {
    Local<Value> args[] = {
      Nan::New(obj->stats_messages.front().c_str()).ToLocalChecked()
    };
    Nan::AsyncResource resource("InternalServer.statsCallback");
    resource.runInAsyncScope(Nan::GetCurrentContext()->Global(),
                             obj->stats_callback_->GetFunction(),
                             1, args);
    obj->stats_messages.pop();
  }
}

void InternalClient::onConnected() {
  boost::mutex::scoped_lock lock(stats_lock);
  if (!async_stats_ || !stats_callback_) {
    return;
  }
  stats_messages.push("connected");
  async_stats_->data = this;
  uv_async_send(async_stats_);
}

void InternalClient::onDisconnected() {
  boost::mutex::scoped_lock lock(stats_lock);
  if (!async_stats_ || !stats_callback_) {
    return;
  }
  stats_messages.push("disconnected");
  async_stats_->data = this;
  uv_async_send(async_stats_);
}
