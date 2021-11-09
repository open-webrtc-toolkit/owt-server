// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include "QuicTransportWrap.h"

using namespace v8;

// Class QuicTransportIn
Persistent<Function> QuicTransportIn::constructor;
QuicTransportIn::QuicTransportIn() {};
QuicTransportIn::~QuicTransportIn() {};

void QuicTransportIn::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "QuicTransportIn"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getListeningPort", getListeningPort);
  NODE_SET_PROTOTYPE_METHOD(tpl, "send", send);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addEventListener", addEventListener);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "in"), tpl->GetFunction());
}

void QuicTransportIn::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  int port = args[0]->Uint32Value();
  String::Utf8Value param0(args[1]->ToString());
  std::string cert_file = std::string(*param0);
  String::Utf8Value param1(args[2]->ToString());
  std::string key_file = std::string(*param1);

  QuicTransportIn* obj = new QuicTransportIn();
  obj->me = new QuicIn(port, cert_file, key_file, obj);

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void QuicTransportIn::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  QuicTransportIn* obj = ObjectWrap::Unwrap<QuicTransportIn>(args.Holder());
  delete obj->me;
  obj->me = nullptr;
}

void QuicTransportIn::getListeningPort(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  QuicTransportIn* obj = ObjectWrap::Unwrap<QuicTransportIn>(args.This());
  QuicIn* me = obj->me;

  uint32_t port = me->getListeningPort();

  args.GetReturnValue().Set(Number::New(isolate, port));
}

void QuicTransportIn::send(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  QuicTransportIn* obj = ObjectWrap::Unwrap<QuicTransportIn>(args.Holder());
  QuicIn* me = obj->me;

  unsigned int session_id = args[0]->Uint32Value();
  unsigned int stream_id = args[1]->Uint32Value();
  String::Utf8Value param1(args[2]->ToString());
  std::string data = std::string(*param1);

  me->send(session_id, stream_id, data);
}

void QuicTransportIn::addEventListener(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }
    QuicTransportIn* obj = ObjectWrap::Unwrap<QuicTransportIn>(args.Holder());
    if (!obj->me)
        return;
    Local<Object>::New(isolate, obj->m_store)->Set(args[0], args[1]);
}

// Class InternalQuicOut
Persistent<Function> QuicTransportOut::constructor;
QuicTransportOut::QuicTransportOut() {};
QuicTransportOut::~QuicTransportOut() {};

void QuicTransportOut::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "QuicTransportOut"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "send", send);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addEventListener", addEventListener);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "out"), tpl->GetFunction());
}

void QuicTransportOut::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param0(args[0]->ToString());
  std::string ip = std::string(*param0);
  int port = args[1]->Uint32Value();

  QuicTransportOut* obj = new QuicTransportOut();
  obj->me = new QuicOut(ip, port, obj);

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void QuicTransportOut::close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  QuicTransportOut* obj = ObjectWrap::Unwrap<QuicTransportOut>(args.Holder());
  delete obj->me;
  obj->me = nullptr;
}

void QuicTransportOut::send(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  QuicTransportOut* obj = ObjectWrap::Unwrap<QuicTransportOut>(args.Holder());
  QuicOut* me = obj->me;

  unsigned int session_id = args[0]->Uint32Value();
  unsigned int stream_id = args[1]->Uint32Value();
  String::Utf8Value param1(args[2]->ToString());
  std::string data = std::string(*param1);

  me->send(session_id, stream_id, data);
}

void QuicTransportOut::addEventListener(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }
    QuicTransportOut* obj = ObjectWrap::Unwrap<QuicTransportOut>(args.Holder());
    if (!obj->me)
        return;
    Local<Object>::New(isolate, obj->m_store)->Set(args[0], args[1]);
}
