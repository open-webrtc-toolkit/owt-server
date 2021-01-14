/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportServer.h"
#include "QuicFactory.h"
#include "QuicTransportStream.h"
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

QuicTransportServer::QuicTransportServer(int port, const std::string& pfxPath, const std::string& password)
    : m_quicServer(QuicFactory::getQuicTransportFactory()->CreateQuicTransportServer(port, pfxPath.c_str(), password.c_str()))
{
    m_quicServer->SetVisitor(this);
    ELOG_DEBUG("QuicTransportServer::QuicTransportServer");
}

NAN_MODULE_INIT(QuicTransportServer::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportServer").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

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
    int port = info[0]->IntegerValue();
    v8::String::Utf8Value pfxPath(Nan::To<v8::String>(info[1]).ToLocalChecked());
    v8::String::Utf8Value password(Nan::To<v8::String>(info[2]).ToLocalChecked());
    QuicTransportServer* obj = new QuicTransportServer(port, *pfxPath, *password);
    obj->Wrap(info.This());
    uv_async_init(uv_default_loop(), &obj->m_asyncOnConnection, &QuicTransportServer::onConnectionCallback);
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
    QuicTransportServer* obj = Nan::ObjectWrap::Unwrap<QuicTransportServer>(info.Holder());
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&obj->m_asyncOnConnection))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&obj->m_asyncOnConnection), NULL);
    }
}

NAUV_WORK_CB(QuicTransportServer::onConnectionCallback)
{
    ELOG_DEBUG("On connection callback.");
    Nan::HandleScope scope;
    QuicTransportServer* obj = reinterpret_cast<QuicTransportServer*>(async->data);
    if (obj == nullptr || obj->m_connectionsToBeNotified.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(obj->m_connectionQueueMutex);
    while (!obj->m_connectionsToBeNotified.empty()) {
        v8::Local<v8::Object> connection = QuicTransportConnection::newInstance(obj->m_connectionsToBeNotified.front());
        QuicTransportConnection* con = Nan::ObjectWrap::Unwrap<QuicTransportConnection>(connection);
        obj->m_connectionsToBeNotified.front()->SetVisitor(con);
        Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("onconnection").ToLocalChecked());
        if (!onEvent.IsEmpty()) {
            v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
            if (onEventLocal->IsFunction()) {
                v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
                Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("OnConnectionEventCallback").ToLocalChecked());
                Local<Value> args[] = { connection };
                ELOG_DEBUG("Run in async scope.");
                resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
            }
        } else {
            ELOG_DEBUG("onEvent is empty");
        }
        obj->m_connectionsToBeNotified.pop();
    }
}

void QuicTransportServer::OnEnded()
{
    ELOG_DEBUG("QuicTransport server ended.");
}

void QuicTransportServer::OnSession(owt::quic::QuicTransportSessionInterface* session)
{
    ELOG_DEBUG("New session created. Connection ID: %s.", session->ConnectionId());
    {
        std::lock_guard<std::mutex> lock(m_connectionQueueMutex);
        m_connectionsToBeNotified.push(session);
    }
    m_asyncOnConnection.data = this;
    uv_async_send(&m_asyncOnConnection);
}

void QuicTransportServer::onAuthentication(const std::string& id)
{
}

void QuicTransportServer::onClose()
{
}