/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RTCQuicStream.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

Nan::Persistent<v8::Function> RTCQuicStream::s_constructor;

DEFINE_LOGGER(RTCQuicStream, "RTCQuicStream");

RTCQuicStream::RTCQuicStream(owt::quic::P2PQuicStreamInterface* p2pQuicStream)
    : m_p2pQuicStream(p2pQuicStream)
{
    ELOG_DEBUG("RTCQuicStream::RTCQuicStream");
    if (!p2pQuicStream) {
        ELOG_DEBUG("owt::quic::P2PQuicStreamInterface is nullptr.");
    }
    p2pQuicStream->SetDelegate(this);
}

NAN_MODULE_INIT(RTCQuicStream::Init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("RTCQuicStream").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);
    Nan::SetPrototypeMethod(tpl, "write", write);
    s_constructor.Reset(tpl->GetFunction());
}

NAN_METHOD(RTCQuicStream::newInstance)
{
    ELOG_DEBUG("Default newInstance.");
    if (!info.IsConstructCall()) {
        return Nan::ThrowError("Must be called as a constructor.");
    }
    if (info.Length() != 1 || !info[0]->IsExternal()) {
        return Nan::ThrowError("Must be called internally.");
    }
    owt::quic::P2PQuicStreamInterface* p2pQuicStream = static_cast<owt::quic::P2PQuicStreamInterface*>(info[0].As<v8::External>()->Value());
    ELOG_DEBUG("Before assert.");
    assert(p2pQuicStream);
    RTCQuicStream* obj = new RTCQuicStream(p2pQuicStream);
    uv_async_init(uv_default_loop(), &obj->m_asyncOnData, &RTCQuicStream::onDataCallback);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(RTCQuicStream::write)
{
    ELOG_DEBUG("write");
    if (info.Length() != 1) {
        return Nan::ThrowTypeError("QuicStreamWriteParameters is not provided");
    }
    RTCQuicStream* obj = Nan::ObjectWrap::Unwrap<RTCQuicStream>(info.Holder());
    Nan::MaybeLocal<Value> maybeQuicStreamWriteParameters = info[0];
    Local<v8::Object> quicStreamWriteParameters = maybeQuicStreamWriteParameters.ToLocalChecked()->ToObject();
    auto parametersData = Nan::Get(quicStreamWriteParameters, Nan::New("data").ToLocalChecked()).ToLocalChecked();
    char* data = node::Buffer::Data(parametersData);
    size_t dataLength = node::Buffer::Length(parametersData);
    bool finished = Nan::Get(quicStreamWriteParameters, Nan::New("finished").ToLocalChecked()).ToLocalChecked()->ToBoolean()->BooleanValue();
    obj->m_p2pQuicStream->WriteOrBufferData(reinterpret_cast<uint8_t*>(data), dataLength, finished);
}

NAUV_WORK_CB(RTCQuicStream::onDataCallback)
{
    ELOG_DEBUG("ondatacallback");
    Nan::HandleScope scope;
    RTCQuicStream* obj = reinterpret_cast<RTCQuicStream*>(async->data);
    if (!obj || obj->m_dataToBeNotified.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(obj->m_dataQueueMutex);
    while (!obj->m_dataToBeNotified.empty()) {
        v8::MaybeLocal<v8::Object> data = Nan::CopyBuffer((char*)(obj->m_dataToBeNotified.front().data()), obj->m_dataToBeNotified.front().size());
        assert(!obj->handle()->IsNull());
        Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("ondata").ToLocalChecked());
        if (!onEvent.IsEmpty()) {
            v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
            if (onEventLocal->IsFunction()) {
                ELOG_DEBUG("onEventLocal is function.");
                v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
                Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("EventCallback").ToLocalChecked());
                Local<Value> args[] = { data.ToLocalChecked() };
                resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
            }
        }
        obj->m_dataToBeNotified.pop();
    }
}

v8::Local<v8::Object> RTCQuicStream::newInstance(owt::quic::P2PQuicStreamInterface* p2pQuicStream)
{
    ELOG_DEBUG("New instance with P2PQuicStream.");
    assert(p2pQuicStream);
    Nan::EscapableHandleScope scope;
    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(p2pQuicStream) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(s_constructor);
    ELOG_DEBUG("Before cons->NewInstance");
    v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);
    return scope.Escape(instance);
}

void RTCQuicStream::OnDataReceived(std::vector<uint8_t> data, bool fin)
{
    // TODO: fin is ignored.
    ELOG_DEBUG("RTCQuicStream::OnDataReceived");
    {
        std::lock_guard<std::mutex> lock(m_dataQueueMutex);
        m_dataToBeNotified.push(data);
    }
    m_asyncOnData.data = this;
    uv_async_send(&m_asyncOnData);
}

RTCQuicStream::~RTCQuicStream(){
    ELOG_DEBUG("RTCQuicStream::~RTCQuicStream");
}