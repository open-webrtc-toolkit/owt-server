/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportStream.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

DEFINE_LOGGER(QuicTransportStream, "QuicTransportStream");

Nan::Persistent<v8::Function> QuicTransportStream::s_constructor;

const int uuidSizeInByte = 16;

QuicTransportStream::QuicTransportStream()
    : QuicTransportStream(nullptr)
{
}
QuicTransportStream::QuicTransportStream(owt::quic::QuicTransportStreamInterface* stream)
    : m_stream(stream)
    , m_contentSessionId("")
    , m_receivedContentSessionId(false)
{
}

QuicTransportStream::~QuicTransportStream()
{
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnContentSessionId))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnContentSessionId), NULL);
    }
    m_stream->SetVisitor(nullptr);
}

void QuicTransportStream::OnCanRead()
{
    ELOG_DEBUG("Readable size: %d", (int)m_stream->ReadableBytes());
    if (!m_receivedContentSessionId) {
        MaybeReadContentSessionId();
    }
    ELOG_DEBUG("On can read.");
}

void QuicTransportStream::OnCanWrite()
{
    ELOG_DEBUG("On can write.");
}

NAN_MODULE_INIT(QuicTransportStream::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportStream").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    // Nan::SetPrototypeMethod(tpl, "start", start);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportStream").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportStream::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    QuicTransportStream* obj = new QuicTransportStream();
    obj->Wrap(info.This());
    uv_async_init(uv_default_loop(), &obj->m_asyncOnContentSessionId, &QuicTransportStream::onContentSessionId);
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Object> QuicTransportStream::newInstance(owt::quic::QuicTransportStreamInterface* stream)
{
    Local<Object> streamObject = Nan::NewInstance(Nan::New(QuicTransportStream::s_constructor)).ToLocalChecked();
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
    obj->m_stream = stream;
    obj->MaybeReadContentSessionId();
    return streamObject;
}

void QuicTransportStream::MaybeReadContentSessionId()
{
    ELOG_DEBUG("Readable size: %d", (int)m_stream->ReadableBytes());
    if (!m_receivedContentSessionId) {
        // Match to a content session.
        if (m_stream->ReadableBytes() < uuidSizeInByte) {
            ELOG_ERROR("No enough data to get content session ID.");
            m_stream->Close();
            return;
        }
        uint8_t* data = new uint8_t[uuidSizeInByte];
        m_stream->Read(data, uuidSizeInByte);
        m_contentSessionId = std::string(data, data + uuidSizeInByte);
        delete[] data;
        m_receivedContentSessionId = true;
        ELOG_INFO("Content session ID: %s", m_contentSessionId);
        m_asyncOnContentSessionId.data = this;
        uv_async_send(&m_asyncOnContentSessionId);
    }
}

NAUV_WORK_CB(QuicTransportStream::onContentSessionId)
{
    Nan::HandleScope scope;
    QuicTransportStream* obj = reinterpret_cast<QuicTransportStream*>(async->data);
    if (obj == nullptr) {
        return;
    }
    Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("oncontentsessionid").ToLocalChecked());
    if (!onEvent.IsEmpty()) {
        v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
        if (onEventLocal->IsFunction()) {
            v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
            Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("oncontentsessionid").ToLocalChecked());
            Local<Value> args[] = { Nan::New(obj->m_contentSessionId).ToLocalChecked() };
            resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
        }
    }
}