// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AVStreamOutWrap.h"
#include <MediaFramePipeline.h>
#include <MediaFileOut.h>
#include <LiveStreamOut.h>

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
    Local<Object> options = args[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    bool requireAudio = (*options->Get(String::NewFromUtf8(isolate, "require_audio"))->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked())->BooleanValue();
    bool requireVideo = (*options->Get(String::NewFromUtf8(isolate, "require_video"))->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked())->BooleanValue();
    AVStreamOutWrap* obj = new AVStreamOutWrap();
    std::string type = std::string(*String::Utf8Value(isolate, options->Get(String::NewFromUtf8(isolate, "type"))->ToString()));
    std::string url = std::string(*String::Utf8Value(isolate, options->Get(String::NewFromUtf8(isolate, "url"))->ToString()));
    int initializeTimeout = options->Get(String::NewFromUtf8(isolate, "initializeTimeout"))->Int32Value(Nan::GetCurrentContext()).ToChecked();
    if (type.compare("streaming") == 0) {
        Local<Object> connection = options->Get(String::NewFromUtf8(isolate, "connection"))->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        std::string protocol = std::string(*String::Utf8Value(isolate, connection->Get(String::NewFromUtf8(isolate, "protocol"))->ToString()));
        std::string url = std::string(*String::Utf8Value(isolate, connection->Get(String::NewFromUtf8(isolate, "url"))->ToString()));

        owt_base::LiveStreamOut::StreamingFormat format;
        if (protocol.compare("rtsp") == 0) {
            format = owt_base::LiveStreamOut::STREAMING_FORMAT_RTSP;
        } else if (protocol.compare("rtmp") == 0) {
            format = owt_base::LiveStreamOut::STREAMING_FORMAT_RTMP;
        } else if (protocol.compare("hls") == 0) {
            format = owt_base::LiveStreamOut::STREAMING_FORMAT_HLS;
        } else if (protocol.compare("dash") == 0) {
            format = owt_base::LiveStreamOut::STREAMING_FORMAT_DASH;
        } else {
            isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Unsupported AVStreamOut type")));
            return;
        }

        owt_base::LiveStreamOut::StreamingOptions opts;

        opts.format = format;
        if (protocol.compare("hls") == 0) {
            Local<Object> parameters = connection->Get(String::NewFromUtf8(isolate, "parameters"))->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            opts.hls_time = parameters->Get(String::NewFromUtf8(isolate, "hlsTime"))->Int32Value(Nan::GetCurrentContext()).ToChecked();
            opts.hls_list_size = parameters->Get(String::NewFromUtf8(isolate, "hlsListSize"))->Int32Value(Nan::GetCurrentContext()).ToChecked();

            strncpy(opts.hls_method, std::string(*String::Utf8Value(isolate, parameters->Get(String::NewFromUtf8(isolate, "method"))->ToString())).c_str(), sizeof(opts.hls_method) - 1);
            opts.hls_method[sizeof(opts.hls_method) - 1] = '\0';

        } else if (protocol.compare("dash") == 0) {
            Local<Object> parameters = connection->Get(String::NewFromUtf8(isolate, "parameters"))->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            opts.dash_seg_duration = parameters->Get(String::NewFromUtf8(isolate, "dashSegDuration"))->Int32Value(Nan::GetCurrentContext()).ToChecked();
            opts.dash_window_size = parameters->Get(String::NewFromUtf8(isolate, "dashWindowSize"))->Int32Value(Nan::GetCurrentContext()).ToChecked();

            strncpy(opts.dash_method, std::string(*String::Utf8Value(isolate, parameters->Get(String::NewFromUtf8(isolate, "method"))->ToString())).c_str(), sizeof(opts.dash_method) - 1);
            opts.dash_method[sizeof(opts.dash_method) - 1] = '\0';
        }

        obj->me = new owt_base::LiveStreamOut(url, requireAudio, requireVideo, obj, initializeTimeout, opts);
    } else if (type.compare("file") == 0) {
        obj->me = new owt_base::MediaFileOut(url, requireAudio, requireVideo, obj, initializeTimeout);
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
