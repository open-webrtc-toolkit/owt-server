// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "InternalServerWrapper.h"

using namespace v8;

DEFINE_LOGGER(InternalServer, "InternalServerWrapper");

Nan::Persistent<Function> InternalServer::constructor;

InternalServer::InternalServer() {
  async_stats_ = new uv_async_t;
  stats_callback_ = nullptr;
  uv_async_init(uv_default_loop(), async_stats_, &InternalServer::statsCallback);
}

static void destroyAsyncHandle(uv_handle_t *handle) {
  delete handle;
  handle = nullptr;
}

InternalServer::~InternalServer() {
  boost::mutex::scoped_lock lock(stats_lock);
  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(async_stats_))) {
    ELOG_DEBUG("Closing Stats handle");
    uv_close(reinterpret_cast<uv_handle_t*>(async_stats_), destroyAsyncHandle);
  }
  async_stats_ = nullptr;
}

NAN_MODULE_INIT(InternalServer::Init) {
  // Constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("InternalServer").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "close", close);
  Nan::SetPrototypeMethod(tpl, "getListeningPort", getListeningPort);
  Nan::SetPrototypeMethod(tpl, "addSource", addSource);
  Nan::SetPrototypeMethod(tpl, "removeSource", removeSource);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("InternalServer").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(InternalServer::New) {
  if (info.Length() < 3) {
    Nan::ThrowError("Wrong number of arguments");
  }

  if (info.IsConstructCall()) {
    Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string protocol = std::string(*param0);

    unsigned int minPort = Nan::To<unsigned int>(info[1]).FromJust();
    unsigned int maxPort = Nan::To<unsigned int>(info[2]).FromJust();

    InternalServer* obj = new InternalServer();
    obj->me = new owt_base::InternalServer(
        protocol, minPort, maxPort, obj);
    if (info.Length() > 3 && info[3]->IsFunction()) {
      obj->stats_callback_ = new Nan::Callback(info[3].As<Function>());
    }
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    ELOG_WARN("Not construct call");
  }
}

NAN_METHOD(InternalServer::close) {
  InternalServer* obj = ObjectWrap::Unwrap<InternalServer>(info.Holder());
  owt_base::InternalServer* me = obj->me;
  delete me;
  obj->me = nullptr;
}

NAN_METHOD(InternalServer::getListeningPort) {
  InternalServer* obj = ObjectWrap::Unwrap<InternalServer>(info.Holder());
  owt_base::InternalServer* me = obj->me;

  uint32_t port = me->getListeningPort();

  info.GetReturnValue().Set(Nan::New(port));
}

NAN_METHOD(InternalServer::addSource) {
  InternalServer* obj = ObjectWrap::Unwrap<InternalServer>(info.Holder());
  owt_base::InternalServer* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string streamId = std::string(*param0);

  FrameSource* param =
    ObjectWrap::Unwrap<FrameSource>(
      info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameSource* src = param->src;

  me->addSource(streamId, src);
}

NAN_METHOD(InternalServer::removeSource) {
  InternalServer* obj = ObjectWrap::Unwrap<InternalServer>(info.Holder());
  owt_base::InternalServer* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string streamId = std::string(*param0);

  me->removeSource(streamId);
}

NAUV_WORK_CB(InternalServer::statsCallback) {
  Nan::HandleScope scope;
  InternalServer* obj = reinterpret_cast<InternalServer*>(async->data);
  if (!obj || !obj->me || !obj->stats_callback_) {
    return;
  }
  boost::mutex::scoped_lock lock(obj->stats_lock);
  while (!obj->stats_messages.empty()) {
    Local<Value> args[] = {
      Nan::New(obj->stats_messages.front().first.c_str()).ToLocalChecked(),
      Nan::New(obj->stats_messages.front().second.c_str()).ToLocalChecked()
    };
    Nan::AsyncResource resource("InternalServer.statsCallback");
    resource.runInAsyncScope(Nan::GetCurrentContext()->Global(),
                             obj->stats_callback_->GetFunction(),
                             2, args);
    obj->stats_messages.pop();
  }
}

void InternalServer::onConnected(const std::string& id) {
  boost::mutex::scoped_lock lock(stats_lock);
  if (!async_stats_ || !stats_callback_) {
    return;
  }
  stats_messages.emplace("connected", id);
  async_stats_->data = this;
  uv_async_send(async_stats_);
}

void InternalServer::onDisconnected(const std::string& id) {
  boost::mutex::scoped_lock lock(stats_lock);
  if (!async_stats_ || !stats_callback_) {
    return;
  }
  stats_messages.emplace("disconnected", id);
  async_stats_->data = this;
  uv_async_send(async_stats_);
}
