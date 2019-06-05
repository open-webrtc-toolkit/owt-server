/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicStream.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

Nan::Persistent<v8::Function> QuicStream::s_constructor;

DEFINE_LOGGER(QuicStream, "QuicStream");

QuicStream::QuicStream(P2PQuicStream* p2pQuicStream)
    : m_p2pQuicStream(p2pQuicStream)
{
    ELOG_DEBUG("QuicStream::QuicStream");
    if(!p2pQuicStream){
        ELOG_DEBUG("P2PQuicStream is nullptr.");
    }
    p2pQuicStream->SetDelegate(this);
    m_deliveryDataToCppSink=true;
}

NAN_MODULE_INIT(QuicStream::Init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicStream").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);
    Nan::SetPrototypeMethod(tpl, "write", write);
    Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
    s_constructor.Reset(tpl->GetFunction());
}

NAN_METHOD(QuicStream::newInstance)
{
    if (!info.IsConstructCall()) {
        return Nan::ThrowError("Must be called as a constructor.");
    }
    if (info.Length() != 1 || !info[0]->IsExternal()) {
        return Nan::ThrowError("Must be called internally.");
    }
    P2PQuicStream* p2pQuicStream = static_cast<P2PQuicStream*>(info[0].As<v8::External>()->Value());
    assert(p2pQuicStream);
    QuicStream* obj = new QuicStream(p2pQuicStream);
    uv_async_init(uv_default_loop(), &obj->m_asyncOnData, &QuicStream::onDataCallback);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(QuicStream::write)
{
    ELOG_DEBUG("write");
    if (info.Length() != 1) {
        return Nan::ThrowTypeError("QuicStreamWriteParameters is not provided");
    }
    QuicStream* obj = Nan::ObjectWrap::Unwrap<QuicStream>(info.Holder());
    Nan::MaybeLocal<Value> maybeQuicStreamWriteParameters = info[0];
    Local<v8::Object> quicStreamWriteParameters = maybeQuicStreamWriteParameters.ToLocalChecked()->ToObject();
    auto parametersData = Nan::Get(quicStreamWriteParameters, Nan::New("data").ToLocalChecked()).ToLocalChecked();
    char* data = node::Buffer::Data(parametersData);
    size_t dataLength = node::Buffer::Length(parametersData);
    bool finished = Nan::Get(quicStreamWriteParameters, Nan::New("finished").ToLocalChecked()).ToLocalChecked()->ToBoolean()->BooleanValue();
    obj->m_p2pQuicStream->WriteOrBufferData(quic::QuicStringPiece(data, dataLength), finished);
}

NAN_METHOD(QuicStream::addDestination){
    ELOG_DEBUG("QuicStream::addDestination.");
    QuicStream* obj = Nan::ObjectWrap::Unwrap<QuicStream>(info.Holder());
    ELOG_DEBUG("obj");
    owt_base::FrameDestination* dest = Nan::ObjectWrap::Unwrap<QuicStream>(info[0]->ToObject());
    ELOG_DEBUG("dest");
    obj->addDataDestination(dest);
    ELOG_DEBUG("addDataDestination.");
}

NAUV_WORK_CB(QuicStream::onDataCallback)
{
    ELOG_DEBUG("ondatacallback");
    Nan::HandleScope scope;
    QuicStream* obj = reinterpret_cast<QuicStream*>(async->data);
    if (!obj || obj->m_dataToBeNotified.empty()) {
        return;
    }
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::EscapableHandleScope escapableScope(isolate);
    Nan::MaybeLocal<v8::Value> onEvent1=escapableScope.Escape(obj->handle()->Get(isolate->GetCurrentContext(), Nan::New<v8::String>("ondata").ToLocalChecked()).FromMaybe(v8::Local<v8::Value>()));
    std::lock_guard<std::mutex> lock(obj->m_dataQueueMutex);
    while (!obj->m_dataToBeNotified.empty()) {
        ELOG_DEBUG("copy");
        v8::MaybeLocal<v8::Object> data = Nan::CopyBuffer((char*)(obj->m_dataToBeNotified.front().data()), obj->m_dataToBeNotified.front().size());
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

v8::Local<v8::Object> QuicStream::newInstance(P2PQuicStream* p2pQuicStream)
{
    Nan::EscapableHandleScope scope;
    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(p2pQuicStream) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(s_constructor);
    v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);
    return scope.Escape(instance);
}

void QuicStream::OnDataReceived(std::vector<uint8_t> data, bool fin)
{
    // TODO: fin is ignored.
    ELOG_DEBUG("QuicStream::OnDataReceived");
    {
        std::lock_guard<std::mutex> lock(m_dataQueueMutex);
        m_dataToBeNotified.push(data);
    }
    m_asyncOnData.data = this;
    uv_async_send(&m_asyncOnData);
    // Delivery data in C++ level.
    // TODO: Check if it has sink.
    owt_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = owt_base::FrameFormat::FRAME_FORMAT_DATA;
    frame.payload = data.data();
    frame.length = data.size();
    deliverFrame(frame);
    ELOG_DEBUG("QuicStream deliverFrame.");
}

void QuicStream::onFrame(const owt_base::Frame& frame)
{
    ELOG_DEBUG("QuicStream::onFrame.");
    m_p2pQuicStream->WriteOrBufferData(quic::QuicStringPiece((const char*)frame.payload, frame.length), false);
}