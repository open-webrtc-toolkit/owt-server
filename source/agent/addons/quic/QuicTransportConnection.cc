/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportConnection.h"
#include "QuicTransportStream.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

Nan::Persistent<v8::Function> QuicTransportConnection::s_constructor;

DEFINE_LOGGER(QuicTransportConnection, "QuicTransportConnection");

QuicTransportConnection::QuicTransportConnection()
    : m_session(nullptr)
    , m_visitor(nullptr)
{
}

QuicTransportConnection::~QuicTransportConnection()
{
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnStream))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnStream), NULL);
    }
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnClose))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnClose), NULL);
    }
    m_session->SetVisitor(nullptr);
}

NAN_MODULE_INIT(QuicTransportConnection::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportConnection").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "createBidirectionalStream", createBidirectionalStream);
    Nan::SetPrototypeMethod(tpl, "close", close);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportConnection").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

void QuicTransportConnection::setVisitor(Visitor* visitor)
{
    m_visitor = visitor;
}

void QuicTransportConnection::OnIncomingStream(owt::quic::WebTransportStreamInterface* stream)
{
    ELOG_DEBUG("OnIncomingStream.");
    {
        std::lock_guard<std::mutex> lock(m_streamQueueMutex);
        m_streamsToBeNotified.push(stream);
    }
    m_asyncOnStream.data = this;
    uv_async_send(&m_asyncOnStream);
}

void QuicTransportConnection::onEnded()
{
}

NAN_METHOD(QuicTransportConnection::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    QuicTransportConnection* obj = new QuicTransportConnection();
    obj->Wrap(info.This());
    uv_async_init(uv_default_loop(), &obj->m_asyncOnStream, &QuicTransportConnection::onStreamCallback);
    uv_async_init(uv_default_loop(), &obj->m_asyncOnClose, &QuicTransportConnection::onCloseCallback);
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Object> QuicTransportConnection::newInstance(owt::quic::WebTransportSessionInterface* session)
{
    Local<Object> connectionObject = Nan::NewInstance(Nan::New(QuicTransportConnection::s_constructor)).ToLocalChecked();
    QuicTransportConnection* obj = Nan::ObjectWrap::Unwrap<QuicTransportConnection>(connectionObject);
    obj->m_session = session;
    session->SetVisitor(obj);
    return connectionObject;
}

NAUV_WORK_CB(QuicTransportConnection::onStreamCallback)
{
    ELOG_DEBUG("OnStreamCallback.");
    Nan::HandleScope scope;
    QuicTransportConnection* obj = reinterpret_cast<QuicTransportConnection*>(async->data);
    if (obj == nullptr || obj->m_streamsToBeNotified.empty()) {
        return;
    }
    while (!obj->m_streamsToBeNotified.empty()) {
        obj->m_streamQueueMutex.lock();
        auto quicStream = obj->m_streamsToBeNotified.front();
        obj->m_streamsToBeNotified.pop();
        obj->m_streamQueueMutex.unlock();
        v8::Local<v8::Object> streamObject = QuicTransportStream::newInstance(quicStream);
        QuicTransportStream* stream = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
        quicStream->SetVisitor(stream);
        Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("onincomingstream").ToLocalChecked());
        if (!onEvent.IsEmpty()) {
            v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
            if (onEventLocal->IsFunction()) {
                v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
                Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("onincomingstream").ToLocalChecked());
                Local<Value> args[] = { streamObject };
                resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
            }
        } else {
            ELOG_DEBUG("onEvent is empty");
        }
    }
}

NAUV_WORK_CB(QuicTransportConnection::onCloseCallback) {
    ELOG_DEBUG("OnConnectionCloseCallback.");
    Nan::HandleScope scope;
    QuicTransportConnection* obj = reinterpret_cast<QuicTransportConnection*>(async->data);
    if (obj == nullptr) {
        return;
    }
    Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("onclose").ToLocalChecked());
    if (onEvent.IsEmpty()) {
        return;
    }
    v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
    if (onEventLocal->IsFunction()) {
        v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
        Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("onclose").ToLocalChecked());
        resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 0, nullptr);
    }
}

NAN_METHOD(QuicTransportConnection::createBidirectionalStream)
{
    QuicTransportConnection* obj = Nan::ObjectWrap::Unwrap<QuicTransportConnection>(info.Holder());
    auto stream = obj->m_session->CreateBidirectionalStream();
    v8::Local<v8::Object> streamObject = QuicTransportStream::newInstance(stream);
    info.GetReturnValue().Set(streamObject);
}

NAN_METHOD(QuicTransportConnection::close)
{
    QuicTransportConnection* obj = Nan::ObjectWrap::Unwrap<QuicTransportConnection>(info.Holder());
    uint32_t closeCode = 0;
    std::string reason;
    if (info.Length() != 0) {
        auto maybeCloseCode = Nan::Get(Nan::To<v8::Object>(info[0]).ToLocalChecked(), Nan::New<v8::String>("closeCode").ToLocalChecked());
        if (!maybeCloseCode.IsEmpty()) {
            closeCode = Nan::To<uint32_t>(maybeCloseCode.ToLocalChecked()).ToChecked();
        }
        auto maybeReason = Nan::Get(Nan::To<v8::Object>(info[0]).ToLocalChecked(), Nan::New<v8::String>("reason").ToLocalChecked());
        if (!maybeReason.IsEmpty()) {
            auto reasonObj = maybeReason.ToLocalChecked();
            Nan::Utf8String reasonStr(Nan::To<v8::String>(reasonObj).ToLocalChecked());
            reason = std::string(*reasonStr);
        }
    }
    obj->m_session->Close(closeCode, reason.c_str());
}

void QuicTransportConnection::onFrame(const owt_base::Frame& frame)
{
    if (frame.format != owt_base::FRAME_FORMAT_RTP) {
        ELOG_WARN("WebTransport datagram output only supports RTP packets. Received frame type %d. Discard this frame.", frame.format);
        return;
    }
    if (!m_session) {
        ELOG_WARN("WebTransport session is nullptr.");
    }
    m_session->SendOrQueueDatagram(frame.payload, frame.length);
}

void QuicTransportConnection::onVideoSourceChanged() { }

void QuicTransportConnection::OnConnectionClosed()
{
    m_asyncOnClose.data = this;
    uv_async_send(&m_asyncOnClose);
}

void QuicTransportConnection::OnDatagramReceived(const uint8_t* data, size_t length)
{
    if (length > owt_base::FeedbackMsg::kMaxBufferByteLength) {
        ELOG_WARN("The length of datagram received is larger than expected.");
        return;
    }
    owt_base::FeedbackMsg feedback = { .type = owt_base::DATA_FEEDBACK, .cmd = owt_base::RTCP_PACKET };
    feedback.buffer.len = length;
    memcpy(feedback.buffer.data, data, length);
    deliverFeedbackMsg(feedback);
}
