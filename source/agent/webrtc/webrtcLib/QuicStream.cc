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
    //quartcStream->SetDelegate(this);
}

NAN_MODULE_INIT(QuicStream::Init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicStream").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);
    Nan::SetPrototypeMethod(tpl, "write", write);
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
    P2PQuicStream* p2pQuicStream(static_cast<P2PQuicStream*>(info[0].As<v8::External>()->Value()));
    QuicStream* obj = new QuicStream(p2pQuicStream);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(QuicStream::write){
    ELOG_DEBUG("write");
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