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

#include "AVStreamOutWrap.h"
#include <MediaFileOut.h>
#include <MediaFramePipeline.h>
#include <RtspOut.h>
#include <VideoHelper.h>

using namespace v8;

Persistent<Function> AVStreamOutWrap::constructor;
AVStreamOutWrap::AVStreamOutWrap() {}
AVStreamOutWrap::~AVStreamOutWrap() {}

void AVStreamOutWrap::Init(Handle<Object> exports)
{
    Isolate* isolate = exports->GetIsolate();
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "AVStreamOut"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addEventListener", addEventListener);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "AVStreamOut"), tpl->GetFunction());
}

void AVStreamOutWrap::Init(Handle<Object> exports, Handle<Object> module)
{
    Isolate* isolate = exports->GetIsolate();
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "AVStreamOut"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addEventListener", addEventListener);

    constructor.Reset(isolate, tpl->GetFunction());
    module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());
}

void AVStreamOutWrap::New(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if (args.Length() == 0 || !args[0]->IsObject()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }

    // essential options: {
    //     type: (required, 'avstream', or 'file')
    //     require_audio: (required, true or false)
    //     require_video: (required, true or false)
    //     audio_codec: (optional, string, 'pcm_raw' for rtsp/rtmp, 'pcmu', 'opus_48000_2' for recording, etc.),
    //     video_codec: (optional, string, 'h264', 'vp8', etc.),
    //     video_resolution: (required when require_video === true, string),
    //     url: (required, string),
    //     interval: (required, only for 'file')
    // }
    Local<Object> options = args[0]->ToObject();
    bool requireAudio = *options->Get(String::NewFromUtf8(isolate, "require_audio"))->ToBoolean();
    bool requireVideo = *options->Get(String::NewFromUtf8(isolate, "require_video"))->ToBoolean();
    woogeen_base::AVStreamOut::AVOptions audioOption, videoOption, *pAudio = nullptr, *pVideo = nullptr;
    if (requireAudio) {
        audioOption.codec = std::string(*String::Utf8Value(options->Get(String::NewFromUtf8(isolate, "audio_codec"))->ToString()));
        if (audioOption.codec.compare("pcmu") == 0) { // "pcmu"
            audioOption.spec.audio.sampleRate = 8000;
            audioOption.spec.audio.channels = 1;
        } else { // "pcm_raw", "opus_48000_2"
            audioOption.spec.audio.sampleRate = 48000;
            audioOption.spec.audio.channels = 2;
        }
        pAudio = &audioOption;
    }
    if (requireVideo) {
        videoOption.codec = std::string(*String::Utf8Value(options->Get(String::NewFromUtf8(isolate, "video_codec"))->ToString()));
        std::string resolution = std::string(*String::Utf8Value(options->Get(String::NewFromUtf8(isolate, "video_resolution"))->ToString()));
        woogeen_base::VideoSize vSize{ 0, 0 };
        woogeen_base::VideoResolutionHelper::getVideoSize(resolution, vSize);
        videoOption.spec.video.width = vSize.width;
        videoOption.spec.video.height = vSize.height;
        pVideo = &videoOption;
    }
    AVStreamOutWrap* obj = new AVStreamOutWrap();
    std::string type = std::string(*String::Utf8Value(options->Get(String::NewFromUtf8(isolate, "type"))->ToString()));
    std::string url = std::string(*String::Utf8Value(options->Get(String::NewFromUtf8(isolate, "url"))->ToString()));
    if (type.compare("avstream") == 0)
        obj->me = new woogeen_base::RtspOut(url, pAudio, pVideo, obj);
    else if (type.compare("file") == 0) {
        int snapshotInterval = options->Get(String::NewFromUtf8(isolate, "interval"))->IntegerValue();
        obj->me = new woogeen_base::MediaFileOut(url, pAudio, pVideo, snapshotInterval, obj);
    } else {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Unsupported AVStreamOut type")));
        return;
    }
    obj->dest = obj->me;

    if (args.Length() > 1 && args[1]->IsFunction())
        Local<Object>::New(isolate, obj->m_store)->Set(String::NewFromUtf8(isolate, "init"), args[1]);

    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
}

void AVStreamOutWrap::close(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    AVStreamOutWrap* obj = ObjectWrap::Unwrap<AVStreamOutWrap>(args.Holder());
    if (obj->me) {
        delete obj->me;
        obj->m_store.Reset();
        obj->me = nullptr;
    }
}

void AVStreamOutWrap::addEventListener(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }
    AVStreamOutWrap* obj = ObjectWrap::Unwrap<AVStreamOutWrap>(args.Holder());
    if (!obj->me)
        return;
    Local<Object>::New(isolate, obj->m_store)->Set(args[0], args[1]);
}
