// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransportServer.h"
#include "QuicFactory.h"
#include "Utils.h"
#include <thread>
#include <chrono>
#include <iostream>

#include "owt/quic/quic_transport_factory.h"
#include "owt/quic/quic_transport_session_interface.h"

//using namespace net;
using namespace owt_base;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;


DEFINE_LOGGER(QuicTransportServer, "QuicTransportServer");

Nan::Persistent<v8::Function> QuicTransportServer::s_constructor;

// QUIC Incomming
QuicTransportServer::QuicTransportServer(unsigned int port, const std::string& cert_file, const std::string& key_file)
        : m_quicServer(QuicFactory::getQuicTransportFactory()->CreateQuicTransportServer(port, cert_file.c_str(), key_file.c_str())) {
  m_quicServer->SetVisitor(this);
  printf("point address:%p\n", this);
}

QuicTransportServer::~QuicTransportServer() {
    m_quicServer->Stop();
    m_quicServer.reset();
}

NAN_MODULE_INIT(QuicTransportServer::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportServer").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "onNewSession", onNewSession);
    Nan::SetPrototypeMethod(tpl, "start", start);
    Nan::SetPrototypeMethod(tpl, "stop", stop);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportServer").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportServer::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    if (info.Length() < 3) {
        return Nan::ThrowTypeError("No enough arguments are provided.");
    }
    int port = Nan::To<int32_t>(info[0]).FromJust();
    Nan::Utf8String pfxPath(Nan::To<v8::String>(info[1]).ToLocalChecked());
    Nan::Utf8String password(Nan::To<v8::String>(info[2]).ToLocalChecked());
    QuicTransportServer* obj = new QuicTransportServer(port, *pfxPath, *password);
    //owt_base::Utils::ZeroMemory(*password, password.length());
    obj->Wrap(info.This());
    uv_async_init(uv_default_loop(), &obj->m_asyncOnNewSession, &QuicTransportServer::onNewSessionCallback);
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(QuicTransportServer::start)
{
    ELOG_DEBUG("QuicTransportServer::start");
    QuicTransportServer* obj = Nan::ObjectWrap::Unwrap<QuicTransportServer>(info.Holder());
    obj->m_quicServer->Start();
    ELOG_DEBUG("End of QuicTransportServer::start");
}

NAN_METHOD(QuicTransportServer::stop)
{
    ELOG_DEBUG("QuicTransportServer::stop");
    QuicTransportServer* obj = Nan::ObjectWrap::Unwrap<QuicTransportServer>(info.Holder());
    obj->m_quicServer->Stop();
    ELOG_DEBUG("End of QuicTransportServer::stop");
}

NAN_METHOD(QuicTransportServer::onNewSession) {
  ELOG_DEBUG("QuicTransportServer::onNewSession");
  QuicTransportServer* obj = Nan::ObjectWrap::Unwrap<QuicTransportServer>(info.Holder());

  obj->has_session_callback_ = true;
  obj->session_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAUV_WORK_CB(QuicTransportServer::onNewSessionCallback){
    ELOG_DEBUG("QuicTransportServer::onNewSessionCallback");
    Nan::HandleScope scope;
    QuicTransportServer* obj = reinterpret_cast<QuicTransportServer*>(async->data);
    if (!obj) {
        return;
    }

    boost::mutex::scoped_lock lock(obj->mutex);

    v8::Local<v8::Object> connection = QuicTransportSession::newInstance(obj->session_messages.front());
    QuicTransportSession* con = Nan::ObjectWrap::Unwrap<QuicTransportSession>(connection);
    obj->session_messages.front()->SetVisitor(con);

    if (obj->has_session_callback_) {
      while (!obj->session_messages.empty()) {
          Local<Value> args[] = { connection };
          Nan::AsyncResource resource("onNewSession");
          resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->session_callback_->GetFunction(), 1, args);
          obj->session_messages.pop();
      }
    }
}

void QuicTransportServer::OnSession(owt::quic::QuicTransportSessionInterface* session) {
    //sessions_[session->Id()] = session;
    ELOG_DEBUG("QuicTransportServer::OnSession");

    this->session_messages.push(session);
    m_asyncOnNewSession.data = this;
    uv_async_send(&m_asyncOnNewSession);
}

void QuicTransportServer::OnEnded() {
    ELOG_DEBUG("QuicTransportServer::OnEnded");
}
