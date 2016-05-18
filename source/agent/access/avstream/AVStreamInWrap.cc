/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#include "AVStreamInWrap.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <MediaFileIn.h>
#include <RtspIn.h>

using namespace v8;

Persistent<Function> AVStreamInWrap::constructor;
AVStreamInWrap::AVStreamInWrap(bool a, bool v)
    : enableAudio{ a }
    , enableVideo{ v }
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
    Local<String> keyH264 = String::NewFromUtf8(isolate, "h264");
    woogeen_base::RtspIn::Options param{};
    Local<Object> options = args[0]->ToObject();
    if (options->Has(keyUrl))
        param.url = std::string(*String::Utf8Value(options->Get(keyUrl)->ToString()));
    if (options->Has(keyTransport))
        param.transport = std::string(*String::Utf8Value(options->Get(keyTransport)->ToString()));
    if (options->Has(keyBufferSize)) {
        param.bufferSize = options->Get(keyBufferSize)->Uint32Value();
        if (param.bufferSize <= 0)
            param.bufferSize = 1024 * 1024 * 2;
    }
    if (options->Has(keyAudio))
        param.enableAudio = *options->Get(keyAudio)->ToBoolean();
    if (options->Has(keyVideo))
        param.enableVideo = *options->Get(keyVideo)->ToBoolean();
    if (options->Has(keyH264))
        param.enableH264 = *options->Get(keyH264)->ToBoolean();
    else
        param.enableH264 = true;

    AVStreamInWrap* obj = new AVStreamInWrap(param.enableAudio, param.enableVideo);
    std::string type = std::string(*String::Utf8Value(options->Get(String::NewFromUtf8(isolate, "type"))->ToString()));
    if (type.compare("avstream") == 0)
        obj->me = new woogeen_base::RtspIn(param, obj);
    else if (type.compare("file") == 0)
        obj->me = new woogeen_base::MediaFileIn();
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
    woogeen_base::FrameDestination* dest = param->dest;

    if (obj->enableAudio && track == "audio")
        obj->me->addAudioDestination(dest);
    else if (obj->enableVideo && track == "video")
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
    woogeen_base::FrameDestination* dest = param->dest;

    if (obj->enableAudio && track == "audio")
        obj->me->removeAudioDestination(dest);
    else if (obj->enableVideo && track == "video")
        obj->me->removeVideoDestination(dest);
}
