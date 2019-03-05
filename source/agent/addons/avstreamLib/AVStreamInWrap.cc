// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AVStreamInWrap.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <MediaFileIn.h>
#include <LiveStreamIn.h>

using namespace v8;

Persistent<Function> AVStreamInWrap::constructor;
AVStreamInWrap::AVStreamInWrap()
{
}
AVStreamInWrap::~AVStreamInWrap() {}

void AVStreamInWrap::Init(Handle<Object> exports, Handle<Object> module)
{
    Isolate* isolate = exports->GetIsolate();
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "AVStreamIn"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    // Prototype
    SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
    NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

    constructor.Reset(isolate, tpl->GetFunction());
    module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());
}

void AVStreamInWrap::Init(Handle<Object> exports)
{
    Isolate* isolate = exports->GetIsolate();
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "AVStreamIn"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    // Prototype
    SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
    NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "AVStreamIn"), tpl->GetFunction());
}

void AVStreamInWrap::New(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if (args.Length() == 0 || !args[0]->IsObject()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }

    Local<String> keyUrl = String::NewFromUtf8(isolate, "url");
    Local<String> keyTransport = String::NewFromUtf8(isolate, "transport");
    Local<String> keyBufferSize = String::NewFromUtf8(isolate, "buffer_size");
    Local<String> keyAudio = String::NewFromUtf8(isolate, "has_audio");
    Local<String> keyVideo = String::NewFromUtf8(isolate, "has_video");
    owt_base::LiveStreamIn::Options param{};
    Local<Object> options = args[0]->ToObject();
    if (options->Has(keyUrl))
        param.url = std::string(*String::Utf8Value(options->Get(keyUrl)->ToString()));
    if (options->Has(keyTransport))
        param.transport = std::string(*String::Utf8Value(options->Get(keyTransport)->ToString()));
    if (options->Has(keyBufferSize))
        param.bufferSize = options->Get(keyBufferSize)->Uint32Value();
    if (options->Has(keyAudio))
        param.enableAudio = std::string(*String::Utf8Value(options->Get(keyAudio)->ToString()));
    if (options->Has(keyVideo))
        param.enableVideo = std::string(*String::Utf8Value(options->Get(keyVideo)->ToString()));

    AVStreamInWrap* obj = new AVStreamInWrap();
    std::string type = std::string(*String::Utf8Value(options->Get(String::NewFromUtf8(isolate, "type"))->ToString()));
    if (type.compare("streaming") == 0)
        obj->me = new owt_base::LiveStreamIn(param, obj);
    else if (type.compare("file") == 0)
        obj->me = new owt_base::MediaFileIn();
    else {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Unsupported AVStreamIn type")));
        return;
    }

    if (args.Length() > 1 && args[1]->IsFunction())
        Local<Object>::New(isolate, obj->m_store)->Set(String::NewFromUtf8(isolate, "status"), args[1]);

    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
}

void AVStreamInWrap::close(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    AVStreamInWrap* obj = ObjectWrap::Unwrap<AVStreamInWrap>(args.Holder());
    if (obj->me) {
        delete obj->me;
        obj->m_store.Reset();
        obj->me = nullptr;
    }
}

void AVStreamInWrap::addDestination(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    AVStreamInWrap* obj = ObjectWrap::Unwrap<AVStreamInWrap>(args.Holder());
    if (!obj->me)
        return;
    std::string track = std::string(*String::Utf8Value(args[0]->ToString()));
    FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
    owt_base::FrameDestination* dest = param->dest;

    if (track == "audio")
        obj->me->addAudioDestination(dest);
    else if (track == "video")
        obj->me->addVideoDestination(dest);
}

void AVStreamInWrap::removeDestination(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    AVStreamInWrap* obj = ObjectWrap::Unwrap<AVStreamInWrap>(args.Holder());
    if (!obj->me)
        return;
    std::string track = std::string(*String::Utf8Value(args[0]->ToString()));
    FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
    owt_base::FrameDestination* dest = param->dest;

    if (track == "audio")
        obj->me->removeAudioDestination(dest);
    else if (track == "video")
        obj->me->removeVideoDestination(dest);
}
