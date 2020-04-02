/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportServer.h"
#include "QuicFactory.h"
#include "owt/quic/quic_transport_factory.h"
#include "owt/quic/quic_transport_session_interface.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

Nan::Persistent<v8::Function> QuicTransportServer::s_constructor;

DEFINE_LOGGER(QuicTransportServer, "QuicTransportServer");

QuicTransportServer::QuicTransportServer(int port)
    : m_quicServer(QuicFactory::getQuicTransportFactory()->CreateQuicTransportServer(port, "", "", ""))
{
    m_quicServer->SetVisitor(this);
    ELOG_DEBUG("QuicTransportServer::QuicTransportServer");
}

NAN_MODULE_INIT(QuicTransportServer::Init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportServer").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "start", start);
    Nan::SetPrototypeMethod(tpl, "stop", stop);

    s_constructor.Reset(tpl->GetFunction());
    Nan::Set(target, Nan::New("QuicTransportServer").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportServer::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    if (info.Length() == 0) {
        return Nan::ThrowTypeError("Port is required.");
    }
    // Default port number is not specified in https://tools.ietf.org/html/draft-vvv-webtransport-quic-01.
    QuicTransportServer* obj = new QuicTransportServer(7700);
    obj->Wrap(info.This());
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
}

void QuicTransportServer::OnEnded()
{
    ELOG_DEBUG("QuicTransport server ended.");
}

void QuicTransportServer::OnSession(owt::quic::QuicTransportSessionInterface* session)
{
    ELOG_DEBUG("New session created.");
}