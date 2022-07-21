// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransportSession.h"
#include "Utils.h"
#include <thread>
#include <chrono>
#include <iostream>

//using namespace net;
using namespace owt_base;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

DEFINE_LOGGER(QuicTransportSession, "QuicTransportSession");

Nan::Persistent<v8::Function> QuicTransportSession::s_constructor;

// QUIC Incomming
QuicTransportSession::QuicTransportSession()
        : m_session(nullptr) {
}

QuicTransportSession::~QuicTransportSession() {
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnNewStream))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnNewStream), NULL);
    }
    m_session->SetVisitor(nullptr);
}

NAN_MODULE_INIT(QuicTransportSession::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportSession").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "onNewStream", onNewStream);
    Nan::SetPrototypeMethod(tpl, "createBidirectionalStream", createBidirectionalStream);
    Nan::SetPrototypeMethod(tpl, "getId", getId);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportSession").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportSession::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    
    QuicTransportSession* obj = new QuicTransportSession();
    obj->Wrap(info.This());
    uv_async_init(uv_default_loop(), &obj->m_asyncOnNewStream, &QuicTransportSession::onNewStreamCallback);
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Object> QuicTransportSession::newInstance(owt::quic::QuicTransportSessionInterface* session)
{
    ELOG_DEBUG("QuicTransportSession::newInstance");
    Local<Object> connectionObject = Nan::NewInstance(Nan::New(QuicTransportSession::s_constructor)).ToLocalChecked();
    QuicTransportSession* obj = Nan::ObjectWrap::Unwrap<QuicTransportSession>(connectionObject);
    obj->m_session = session;
    return connectionObject;
}

NAN_METHOD(QuicTransportSession::createBidirectionalStream){
    ELOG_DEBUG("QuicTransportSession::createBidirectionalStream");
    QuicTransportSession* obj = Nan::ObjectWrap::Unwrap<QuicTransportSession>(info.Holder());
    auto stream=obj->m_session->CreateBidirectionalStream();
    v8::Local<v8::Object> streamObject = QuicTransportStream::newInstance(stream);
    QuicTransportStream* clientStream = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
    stream->SetVisitor(clientStream);
    info.GetReturnValue().Set(streamObject);
}

NAN_METHOD(QuicTransportSession::onNewStream) {
    ELOG_DEBUG("QuicTransportSession::onNewStream");
  QuicTransportSession* obj = Nan::ObjectWrap::Unwrap<QuicTransportSession>(info.Holder());

  obj->has_stream_callback_ = true;
  obj->stream_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAUV_WORK_CB(QuicTransportSession::onNewStreamCallback){
    ELOG_DEBUG("QuicTransportSession::onNewStreamCallback");
    Nan::HandleScope scope;
    QuicTransportSession* obj = reinterpret_cast<QuicTransportSession*>(async->data);
    if (!obj) {
        return;
    }

    obj->m_sessionQueueMutex.lock();

    v8::Local<v8::Object> streamObject = QuicTransportStream::newInstance(obj->stream_messages.front());
    QuicTransportStream* stream = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
    obj->stream_messages.front()->SetVisitor(stream);

    if (obj->has_stream_callback_) {
      while (!obj->stream_messages.empty()) {
          Local<Value> args[] = { streamObject };
          Nan::AsyncResource resource("onNewStream");
          resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->stream_callback_->GetFunction(), 1, args);
          obj->stream_messages.pop();
      }
    }
    obj->m_sessionQueueMutex.unlock();
}

NAN_METHOD(QuicTransportSession::getId) {
  QuicTransportSession* obj = Nan::ObjectWrap::Unwrap<QuicTransportSession>(info.Holder());
  uint8_t length = obj->m_session->length();
  std::string s_data(obj->m_session->Id(), length);
  std::cout << "QuicTransportSession::getId:" << s_data;
  info.GetReturnValue().Set(Nan::New(s_data).ToLocalChecked());
}

void QuicTransportSession::OnIncomingStream(owt::quic::QuicTransportStreamInterface* stream) {
    std::cout << "QuicTransportSession::OnIncomingStream and id is:" << stream->Id();
    //streams_[stream->Id()] = stream;
    m_sessionQueueMutex.lock();
    this->stream_messages.push(stream);
    m_sessionQueueMutex.unlock();
    m_asyncOnNewStream.data = this;
    uv_async_send(&m_asyncOnNewStream);
}