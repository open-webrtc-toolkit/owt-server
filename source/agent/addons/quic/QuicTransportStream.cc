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
    , m_contentSessionId()
    , m_receivedContentSessionId(false)
{
}

QuicTransportStream::~QuicTransportStream()
{
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnContentSessionId))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnContentSessionId), NULL);
    }
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_asyncOnData))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_asyncOnData), NULL);
    }
    m_stream->SetVisitor(nullptr);
}

void QuicTransportStream::OnCanRead()
{
    ELOG_DEBUG("Readable size: %d", (int)m_stream->ReadableBytes());
    if (!m_receivedContentSessionId) {
        MaybeReadContentSessionId();
    } else {
        SignalOnData();
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

    Nan::SetPrototypeMethod(tpl, "write", write);

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
    uv_async_init(uv_default_loop(), &obj->m_asyncOnData, &QuicTransportStream::onData);
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(QuicTransportStream::write){
    if(info.Length()<2){
        Nan::ThrowTypeError("Data is not provided.");
        return;
    }
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(info.Holder());
    uint8_t* buffer=(uint8_t*)node::Buffer::Data(info[0]->ToObject());
    unsigned int length = info[1]->Uint32Value();
    ELOG_INFO("Before write");
    obj->m_stream->Write(buffer, length);
    ELOG_INFO("After write");
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Object> QuicTransportStream::newInstance(owt::quic::QuicTransportStreamInterface* stream)
{
    Local<Object> streamObject = Nan::NewInstance(Nan::New(QuicTransportStream::s_constructor)).ToLocalChecked();
    ELOG_DEBUG("Created stream obj");
    QuicTransportStream* obj = Nan::ObjectWrap::Unwrap<QuicTransportStream>(streamObject);
    obj->m_stream = stream;
    obj->MaybeReadContentSessionId();
    return streamObject;
}

void QuicTransportStream::MaybeReadContentSessionId()
{
    if (!m_receivedContentSessionId) {
        // Match to a content session.
        if (m_stream->ReadableBytes() > 0 && m_stream->ReadableBytes() < uuidSizeInByte) {
            ELOG_ERROR("No enough data to get content session ID.");
            m_stream->Close();
            return;
        }
        uint8_t* data = new uint8_t[uuidSizeInByte];
        m_stream->Read(data, uuidSizeInByte);
        m_contentSessionId = std::vector<uint8_t>(data, data + uuidSizeInByte);
        m_receivedContentSessionId = true;
        m_asyncOnContentSessionId.data = this;
        uv_async_send(&m_asyncOnContentSessionId);
        if (m_stream->ReadableBytes() > 0) {
            SignalOnData();
        }
    }
}

NAUV_WORK_CB(QuicTransportStream::onData){
    ELOG_DEBUG("QuicTransportStream::onData");
    Nan::HandleScope scope;
    QuicTransportStream* obj = reinterpret_cast<QuicTransportStream*>(async->data);
    if (obj == nullptr) {
        return;
    }
    Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("ondata").ToLocalChecked());
    if (!onEvent.IsEmpty()) {
        v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
        if (onEventLocal->IsFunction()) {
            v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
            Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("ondata").ToLocalChecked());
            auto readableBytes = obj->m_stream->ReadableBytes();
            ELOG_DEBUG("Readable bytes: %d", readableBytes);
            uint8_t* buffer = new uint8_t[readableBytes]; // Use a shared buffer instead to reduce performance cost on new.
            obj->m_stream->Read(buffer, readableBytes);
            Local<Value> args[] = { Nan::NewBuffer((char*)buffer, readableBytes).ToLocalChecked() };
            resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
        }
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
            Local<Value> args[]={Nan::CopyBuffer((char*)obj->m_contentSessionId.data(),uuidSizeInByte).ToLocalChecked()};
            resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
        }
    }
}

void QuicTransportStream::SignalOnData()
{
    m_asyncOnData.data = this;
    uv_async_send(&m_asyncOnData);
}