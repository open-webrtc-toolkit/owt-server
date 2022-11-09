// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AVStreamOutWrap.h"
#include <MediaFramePipeline.h>
#include <MediaFileOut.h>
#include <LiveStreamOut.h>

using namespace v8;

static std::string getString(v8::Local<v8::Value> value) {
  Nan::Utf8String value_str(Nan::To<v8::String>(value).ToLocalChecked());
  return std::string(*value_str);
}

Persistent<Function> AVStreamOutWrap::constructor;
AVStreamOutWrap::AVStreamOutWrap() {}
AVStreamOutWrap::~AVStreamOutWrap() {}

void AVStreamOutWrap::Init(Local<Object> exports)
{
    Isolate* isolate = exports->GetIsolate();
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(Nan::New("AVStreamOut").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addEventListener", addEventListener);

    constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(exports, Nan::New("AVStreamOut").ToLocalChecked(),
        Nan::GetFunction(tpl).ToLocalChecked());
}

void AVStreamOutWrap::Init(Local<Object> exports, Local<Object> module)
{
    Isolate* isolate = exports->GetIsolate();
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(Nan::New("AVStreamOut").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addEventListener", addEventListener);

    constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(module, Nan::New("exports").ToLocalChecked(),
        Nan::GetFunction(tpl).ToLocalChecked());
}

void AVStreamOutWrap::New(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if (args.Length() == 0 || !args[0]->IsObject()) {
        Nan::ThrowError("Wrong arguments");
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
    //     connection: {
    //       protocol: ('rtmp', 'rtsp', 'hls', 'dash')
    //       url: (string)
    //     }
    // }
    Local<Object> options = Nan::To<v8::Object>(args[0]).ToLocalChecked();
    bool requireAudio = Nan::To<bool>(
        Nan::Get(options, Nan::New("require_audio").ToLocalChecked()).ToLocalChecked()).FromJust();
    bool requireVideo = Nan::To<bool>(
        Nan::Get(options, Nan::New("require_video").ToLocalChecked()).ToLocalChecked()).FromJust();
    AVStreamOutWrap* obj = new AVStreamOutWrap();
    std::string type = getString(
        Nan::Get(options, Nan::New("type").ToLocalChecked()).ToLocalChecked());
    std::string url = getString(
        Nan::Get(options, Nan::New("url").ToLocalChecked()).ToLocalChecked());
    int initializeTimeout = Nan::To<int32_t>(
        Nan::Get(options, Nan::New("initializeTimeout").ToLocalChecked()).ToLocalChecked()).FromJust();
    if (type.compare("streaming") == 0) {
        Local<Object> connection = Nan::To<v8::Object>(
            Nan::Get(options, Nan::New("connection").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
        std::string protocol = getString(
            Nan::Get(connection, Nan::New("protocol").ToLocalChecked()).ToLocalChecked());
        std::string url = getString(
            Nan::Get(connection, Nan::New("url").ToLocalChecked()).ToLocalChecked());

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
            Nan::ThrowError("Unsupported AVStreamOut type");
            return;
        }

        owt_base::LiveStreamOut::StreamingOptions opts;

        opts.format = format;
        if (protocol.compare("hls") == 0) {
            Local<Object> parameters = Nan::To<v8::Object>(
                Nan::Get(connection, Nan::New("parameters").ToLocalChecked()).ToLocalChecked())
                .ToLocalChecked();
            opts.hls_time = Nan::To<int32_t>(
                Nan::Get(parameters, Nan::New("hlsTime").ToLocalChecked()).ToLocalChecked()).FromJust();
            opts.hls_list_size = Nan::To<int32_t>(
                Nan::Get(parameters, Nan::New("hlsListSize").ToLocalChecked()).ToLocalChecked()).FromJust();

            strncpy(opts.hls_method,
                    getString(Nan::Get(parameters, Nan::New("method").ToLocalChecked()).ToLocalChecked()).c_str(),
                    sizeof(opts.hls_method) - 1);
            opts.hls_method[sizeof(opts.hls_method) - 1] = '\0';

        } else if (protocol.compare("dash") == 0) {
            Local<Object> parameters = Nan::To<v8::Object>(
                Nan::Get(connection, Nan::New("parameters").ToLocalChecked()).ToLocalChecked())
                .ToLocalChecked();
            opts.dash_seg_duration = Nan::To<int32_t>(
                Nan::Get(parameters, Nan::New("dashSegDuration").ToLocalChecked()).ToLocalChecked()).FromJust();
            opts.dash_window_size = Nan::To<int32_t>(
                Nan::Get(parameters, Nan::New("dashWindowSize").ToLocalChecked()).ToLocalChecked()).FromJust();

            strncpy(opts.hls_method,
                    getString(Nan::Get(parameters, Nan::New("method").ToLocalChecked()).ToLocalChecked()).c_str(),
                    sizeof(opts.hls_method) - 1);
            opts.dash_method[sizeof(opts.dash_method) - 1] = '\0';
        }

        obj->me = new owt_base::LiveStreamOut(url, requireAudio, requireVideo, obj, initializeTimeout, opts);
    } else if (type.compare("file") == 0) {
        obj->me = new owt_base::MediaFileOut(url, requireAudio, requireVideo, obj, initializeTimeout);
    } else {
        Nan::ThrowError("Unsupported AVStreamOut type");
        return;
    }
    obj->dest = obj->me;

    if (args.Length() > 1 && args[1]->IsFunction()) {
        Nan::Set(Local<Object>::New(isolate, obj->m_store),
                 Nan::New("init").ToLocalChecked(), args[1]);
    }

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
        Nan::ThrowError("Wrong arguments");
        return;
    }
    AVStreamOutWrap* obj = ObjectWrap::Unwrap<AVStreamOutWrap>(args.Holder());
    if (!obj->me)
        return;
    Nan::Set(Local<Object>::New(isolate, obj->m_store),
             args[0], args[1]);
}
