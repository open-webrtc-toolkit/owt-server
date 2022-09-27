// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransportSession.h"
#include "Utils.h"
#include <thread>
#include <chrono>
#include <iostream>
//#include <get-uv-event-loop-napi.h>

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
    ELOG_DEBUG("QuicTransportSession::~QuicTransportSession");
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnNewStream))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnNewStream), NULL);
    }
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnClosedStream))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnClosedStream), NULL);
    }
    m_session->SetVisitor(nullptr);
    delete asyncResourceNewStream_;
    delete asyncResourceClosedStream_;
}

NAN_MODULE_INIT(QuicTransportSession::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportSession").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "onNewStream", onNewStream);
    Nan::SetPrototypeMethod(tpl, "close", close);
    Nan::SetPrototypeMethod(tpl, "createBidirectionalStream", createBidirectionalStream);
    Nan::SetPrototypeMethod(tpl, "onClosedStream", onClosedStream);
    Nan::SetPrototypeMethod(tpl, "getId", getId);
    Nan::SetPrototypeMethod(tpl, "closeStream", closeStream);

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
    uv_async_init(uv_default_loop(), &obj->m_asyncOnClosedStream, &QuicTransportSession::onClosedStreamCallback);
    obj->asyncResourceNewStream_ = new Nan::AsyncResource("newStreamCallback");
    obj->asyncResourceClosedStream_ = new Nan::AsyncResource("closedStreamCallback");
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
  ELOG_DEBUG("QuicTransportSession::onNewStream end");
}

NAN_METHOD(QuicTransportSession::onClosedStream) {
  ELOG_DEBUG("QuicTransportSession::onClosedStream");
  QuicTransportSession* obj = Nan::ObjectWrap::Unwrap<QuicTransportSession>(info.Holder());

  obj->has_streamClosed_callback_ = true;
  obj->streamClosed_callback_ = new Nan::Callback(info[0].As<Function>());
  ELOG_DEBUG("QuicTransportSession::onClosedStream end");
}

NAN_METHOD(QuicTransportSession::close) {
    ELOG_DEBUG("QuicTransportSession::close");
  QuicTransportSession* obj = Nan::ObjectWrap::Unwrap<QuicTransportSession>(info.Holder());
  obj->m_session->Stop();
  /*delete obj->asyncResource_;
  delete obj->asyncResourceStreamClosed_;*/

  delete obj->stream_callback_;
  delete obj->streamClosed_callback_;
}

NAUV_WORK_CB(QuicTransportSession::onNewStreamCallback){
    ELOG_DEBUG("QuicTransportSession::onNewStreamCallback");
    Nan::HandleScope scope;
    QuicTransportSession* obj = reinterpret_cast<QuicTransportSession*>(async->data);
    if (!obj) {
        return;
    }

    boost::mutex::scoped_lock lock(obj->mutex);

    if (obj->has_stream_callback_) {
      while (!obj->stream_messages.empty()) {
        v8::Local<v8::Object> streamObject = QuicTransportStream::newInstance(obj->stream_messages.front());
        QuicTransportStream* stream = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
        obj->stream_messages.front()->SetVisitor(stream);
         ELOG_DEBUG("stream_messages size:%d", obj->stream_messages.size());
          ELOG_DEBUG("QuicTransportSession::onNewStreamCallback call js stack");
          Local<Value> args[] = { streamObject };

          obj->asyncResourceNewStream_->runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->stream_callback_->GetFunction(), 1, args);
          obj->stream_messages.pop();
      }
    }
    ELOG_DEBUG("QuicTransportSession::onNewStreamCallback ends in session:%d", obj->m_session->Id());
}

NAUV_WORK_CB(QuicTransportSession::onClosedStreamCallback){
    ELOG_DEBUG("QuicTransportSession::onClosedStreamCallback");
    Nan::HandleScope scope;
    QuicTransportSession* obj = reinterpret_cast<QuicTransportSession*>(async->data);
    if (!obj) {
        return;
    }

    if (obj->has_streamClosed_callback_) {
      ELOG_INFO("object has stream callback");
      //boost::mutex::scoped_lock lock(obj->mutex);
      while (!obj->streamclosed_messages.empty()) {
          ELOG_INFO("streamclosed_messages is not empty");
          //auto streamid = obj->streamclosed_messages.front();
          //v8::Local<v8::Object> streamObject = QuicTransportStream::newInstance(quicStream);
          //Local<Value> args[] = { streamObject };
          Local<Value> args[] = { Nan::New(obj->streamclosed_messages.front()) };

          obj->asyncResourceClosedStream_->runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->streamClosed_callback_->GetFunction(), 1, args);
          obj->streamclosed_messages.pop();
      }
    }
    ELOG_INFO("onStreamClosedCallback ends");
}

NAN_METHOD(QuicTransportSession::getId) {
  QuicTransportSession* obj = Nan::ObjectWrap::Unwrap<QuicTransportSession>(info.Holder());
  /*uint8_t length = obj->m_session->length();
  std::string s_data(obj->m_session->Id(), length);
  ELOG_INFO("QuicTransportSession::getId:%s\n",s_data.c_str());*/
  info.GetReturnValue().Set(Nan::New(obj->m_session->Id()).ToLocalChecked());
}

NAN_METHOD(QuicTransportSession::closeStream) {
  QuicTransportSession* obj = Nan::ObjectWrap::Unwrap<QuicTransportSession>(info.Holder());
  uint32_t streamId = Nan::To<int32_t>(info[0]).FromJust();
  obj->m_session->CloseStream(streamId);
}

void QuicTransportSession::OnIncomingStream(owt::quic::QuicTransportStreamInterface* stream) {
    std::cout << "QuicTransportSession::OnIncomingStream and id is:" << stream->Id() << " in session:" << m_session->Id();
    boost::mutex::scoped_lock lock(mutex);
    this->stream_messages.push(stream);
    m_asyncOnNewStream.data = this;
    if (uv_async_send(&m_asyncOnNewStream) == 0) {
        ELOG_INFO("OnIncomingStream uv_async_send succeed and handle pending is:%d", m_asyncOnNewStream.pending); 
    } else {
        ELOG_INFO("OnIncomingStream uv_async_send failed");
    };
    //assert(false);
    ELOG_INFO("OnIncomingStream stream:%d in session:%s in thread:%d end\n", stream->Id(), m_session->Id(), std::this_thread::get_id());
}

void QuicTransportSession::OnStreamClosed(uint32_t id) {
    ELOG_DEBUG("QuicTransportSession stream:%d is closed\n", id);
    //boost::mutex::scoped_lock lock(mutex);
    this->streamclosed_messages.push(id);
    m_asyncOnClosedStream.data = this;
    uv_async_send(&m_asyncOnClosedStream); 
}