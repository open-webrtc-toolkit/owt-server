// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AVStreamInWrap.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <MediaFileIn.h>
#include <LiveStreamIn.h>

using namespace v8;

static std::string getString(v8::Local<v8::Value> value) {
  Nan::Utf8String value_str(Nan::To<v8::String>(value).ToLocalChecked());
  return std::string(*value_str);
}

Persistent<Function> AVStreamInWrap::constructor;
AVStreamInWrap::AVStreamInWrap()
{
}
AVStreamInWrap::~AVStreamInWrap() {}

void AVStreamInWrap::Init(Local<Object> exports, Local<Object> module)
{
    Isolate* isolate = exports->GetIsolate();
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(Nan::New("AVStreamIn").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    // Prototype
    SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
    NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

    constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(module, Nan::New("exports").ToLocalChecked(),
        Nan::GetFunction(tpl).ToLocalChecked());
}

void AVStreamInWrap::Init(Local<Object> exports)
{
    Isolate* isolate = exports->GetIsolate();
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(Nan::New("AVStreamIn").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    // Prototype
    SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
    NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

    constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(exports, Nan::New("AVStreamIn").ToLocalChecked(),
        Nan::GetFunction(tpl).ToLocalChecked());
}

void AVStreamInWrap::New(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if (args.Length() == 0 || !args[0]->IsObject()) {
        Nan::ThrowError("Wrong arguments");
        return;
    }

    Local<String> keyUrl = Nan::New("url").ToLocalChecked();
    Local<String> keyTransport = Nan::New("transport").ToLocalChecked();
    Local<String> keyBufferSize = Nan::New("buffer_size").ToLocalChecked();
    Local<String> keyAudio = Nan::New("has_audio").ToLocalChecked();
    Local<String> keyVideo = Nan::New("has_video").ToLocalChecked();
    owt_base::LiveStreamIn::Options param{};
    Local<Object> options = Nan::To<v8::Object>(args[0]).ToLocalChecked();
    if (Nan::Has(options, keyUrl).FromMaybe(false))
        param.url = getString(Nan::Get(options, keyUrl).ToLocalChecked());
    if (Nan::Has(options, keyTransport).FromMaybe(false))
        param.transport = getString(Nan::Get(options, keyTransport).ToLocalChecked());
    if (Nan::Has(options, keyBufferSize).FromMaybe(false))
        param.bufferSize = Nan::To<int32_t>(Nan::Get(options, keyBufferSize).ToLocalChecked())
                           .FromJust();
    if (Nan::Has(options, keyAudio).FromMaybe(false))
        param.enableAudio = getString(Nan::Get(options, keyAudio).ToLocalChecked());
    if (Nan::Has(options, keyVideo).FromMaybe(false))
        param.enableVideo = getString(Nan::Get(options, keyVideo).ToLocalChecked());

    AVStreamInWrap* obj = new AVStreamInWrap();
    std::string type = getString(Nan::Get(options, Nan::New("type").ToLocalChecked())
                                 .ToLocalChecked());
    if (type.compare("streaming") == 0)
        obj->me = new owt_base::LiveStreamIn(param, obj);
    else if (type.compare("file") == 0)
        obj->me = new owt_base::MediaFileIn();
    else {
        Nan::ThrowError("Unsupported AVStreamIn type");
        return;
    }

    if (args.Length() > 1 && args[1]->IsFunction()) {
        Nan::Set(Local<Object>::New(isolate, obj->m_store),
                 Nan::New("status").ToLocalChecked(), args[1]);
    }

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
    std::string track = getString(args[0]);
    FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(
        Nan::To<v8::Object>(args[1]).ToLocalChecked());
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
    std::string track = getString(args[0]);
    FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(
        Nan::To<v8::Object>(args[1]).ToLocalChecked());
    owt_base::FrameDestination* dest = param->dest;

    if (track == "audio")
        obj->me->removeAudioDestination(dest);
    else if (track == "video")
        obj->me->removeVideoDestination(dest);
}
