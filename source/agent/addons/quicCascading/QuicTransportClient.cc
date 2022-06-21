// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransportClient.h"
#include "QuicFactory.h"
/*#include <thread>
#include <chrono>*/
#include <iostream>

//using namespace net;
using namespace owt_base;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

const char TDT_FEEDBACK_MSG = 0x5A;
const char TDT_MEDIA_FRAME = 0x8F;
const size_t INIT_BUFF_SIZE = 80000;

Nan::Persistent<v8::Function> QuicTransportClient::s_constructor;

DEFINE_LOGGER(QuicTransportClient, "QuicTransportClient");

// QUIC Outgoing
QuicTransportClient::QuicTransportClient(const char* dest_ip, int dest_port)
        : m_quicClient(QuicFactory::getQuicTransportFactory()->CreateQuicTransportClient(dest_ip, dest_port)) {
    m_quicClient->SetVisitor(this);
}

QuicTransportClient::~QuicTransportClient() {
    m_quicClient->Stop();
    m_quicClient.reset();
}

NAN_MODULE_INIT(QuicTransportClient::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportClient").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "connect", connect);
    Nan::SetPrototypeMethod(tpl, "onConnection", onConnection);
    Nan::SetPrototypeMethod(tpl, "onNewStream", onNewStream);
    Nan::SetPrototypeMethod(tpl, "createBidirectionalStream", createBidirectionalStream);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportClient").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportClient::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    if (info.Length() < 2) {
        return Nan::ThrowTypeError("No enough arguments are provided.");
    }
    
    Nan::Utf8String paramId(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string host = std::string(*paramId);
    int port = Nan::To<int32_t>(info[1]).FromJust();
    QuicTransportClient* obj = new QuicTransportClient(host.c_str(), port);
    obj->Wrap(info.This());
    uv_async_init(uv_default_loop(), &obj->m_asyncOnNewStream, &QuicTransportClient::onNewStreamCallback);
    uv_async_init(uv_default_loop(), &obj->m_asyncOnConnected, &QuicTransportClient::onConnectedCallback);
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(QuicTransportClient::connect) {
  ELOG_DEBUG("QuicTransportClient::connect");
  QuicTransportClient* obj = Nan::ObjectWrap::Unwrap<QuicTransportClient>(info.Holder());
  obj->m_quicClient->Start();
  ELOG_DEBUG("End of QuicTransportClient::connect");
}

NAN_METHOD(QuicTransportClient::onNewStream) {
  QuicTransportClient* obj = Nan::ObjectWrap::Unwrap<QuicTransportClient>(info.Holder());

  obj->has_stream_callback_ = true;
  obj->stream_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAN_METHOD(QuicTransportClient::onConnection) {
  ELOG_DEBUG("QuicTransportClient::onConnection");
  QuicTransportClient* obj = Nan::ObjectWrap::Unwrap<QuicTransportClient>(info.Holder());

  obj->has_connected_callback_ = true;
  obj->connected_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAN_METHOD(QuicTransportClient::createBidirectionalStream) {
  ELOG_DEBUG("QuicTransportClient::createBidirectionalStream");
  QuicTransportClient* obj = Nan::ObjectWrap::Unwrap<QuicTransportClient>(info.Holder());
  auto stream=obj->m_quicClient->CreateBidirectionalStream();
  v8::Local<v8::Object> streamObject = QuicTransportStream::newInstance(stream);
  QuicTransportStream* clientStream = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
  stream->SetVisitor(clientStream);
  info.GetReturnValue().Set(streamObject);
}

NAUV_WORK_CB(QuicTransportClient::onNewStreamCallback){
    ELOG_DEBUG("QuicTransportClient::onNewStreamCallback");
    Nan::HandleScope scope;
    QuicTransportClient* obj = reinterpret_cast<QuicTransportClient*>(async->data);
    if (!obj) {
        return;
    }

    if (obj->has_stream_callback_) {
      ELOG_INFO("object has stream callback");
      while (!obj->stream_messages.empty()) {
          ELOG_INFO("stream_messages is not empty");
          obj->m_streamQueueMutex.lock();
          auto quicStream=obj->stream_messages.front();
          obj->m_streamQueueMutex.unlock();
          v8::Local<v8::Object> streamObject = QuicTransportStream::newInstance(quicStream);
          QuicTransportStream* stream = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
          quicStream->SetVisitor(stream);
          Local<Value> args[] = { streamObject };
          Nan::AsyncResource resource("onNewStream");
          resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->stream_callback_->GetFunction(), 1, args);
          obj->stream_messages.pop();
      }
    }
    ELOG_INFO("onNewStreamCallback ends");
}

NAUV_WORK_CB(QuicTransportClient::onConnectedCallback){
    ELOG_DEBUG("QuicTransportClient::onConnectedCallback");
    Nan::HandleScope scope;
    QuicTransportClient* obj = reinterpret_cast<QuicTransportClient*>(async->data);
    if (!obj) {
        return;
    }

    if (obj->has_connected_callback_) {
      Local<Value> args[] = {};
      Nan::AsyncResource resource("connectedCallback");
      resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->connected_callback_->GetFunction(), 1, args);
    }
}

void QuicTransportClient::OnIncomingStream(owt::quic::QuicTransportStreamInterface* stream) {
    ELOG_DEBUG("QuicTransportClient::OnIncomingStream with stream id:%d\n", stream->Id());
    //streams_[stream->Id()] = stream;
    m_streamQueueMutex.lock();
    ELOG_INFO("stream_messages push stream");
    this->stream_messages.push(stream);
    m_streamQueueMutex.unlock();
    m_asyncOnNewStream.data = this;
    uv_async_send(&m_asyncOnNewStream);
    ELOG_INFO("OnIncomingStream ends");
}

void QuicTransportClient::OnConnected() {
    ELOG_DEBUG("QuicTransportClient::OnConnected");
    m_asyncOnConnected.data = this;
    uv_async_send(&m_asyncOnConnected);
}

void QuicTransportClient::OnConnectionFailed() {
    ELOG_DEBUG("QuicTransport client connection failed.");
}

