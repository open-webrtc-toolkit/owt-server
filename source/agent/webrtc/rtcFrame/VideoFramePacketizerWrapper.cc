// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "MediaWrapper.h"
#include "VideoFramePacketizerWrapper.h"
#include "CallBaseWrapper.h"
#include "WebRtcConnection.h"

using namespace v8;

Persistent<Function> VideoFramePacketizer::constructor;
VideoFramePacketizer::VideoFramePacketizer() {};
VideoFramePacketizer::~VideoFramePacketizer() {};

void VideoFramePacketizer::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "VideoFramePacketizer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);

  NODE_SET_PROTOTYPE_METHOD(tpl, "bindTransport", bindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unbindTransport", unbindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "enable", enable);
  NODE_SET_PROTOTYPE_METHOD(tpl, "ssrc", getSsrc);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getTotalBitrate", getTotalBitrate);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getRetransmitBitrate", getRetransmitBitrate);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getEstimatedBandwidth", getEstimatedBandwidth);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "VideoFramePacketizer"), tpl->GetFunction());
}

void VideoFramePacketizer::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  bool supportRED = (args[0]->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked())->BooleanValue();
  bool supportULPFEC = (args[1]->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked())->BooleanValue();
  int transportccExt = (args.Length() >= 3) ? args[2]->IntegerValue(Nan::GetCurrentContext()).ToChecked() : -1;
  std::string mid;
  int midExtId = -1;
  if (args.Length() >= 5) {
    v8::String::Utf8Value param4(isolate, Nan::To<v8::String>(args[3]).ToLocalChecked());
    mid = std::string(*param4);
    midExtId = args[4]->IntegerValue(Nan::GetCurrentContext()).ToChecked();
  }
  bool selfRequestKeyframe = (args.Length() >= 6)
      ? (args[5]->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked())->BooleanValue()
      : false;

  CallBase* baseWrapper = (args.Length() >= 7)
      ? Nan::ObjectWrap::Unwrap<CallBase>(Nan::To<v8::Object>(args[6]).ToLocalChecked())
      : nullptr;

  bool enableBandwidthEstimation = (args.Length() >= 8)
      ? (args[7]->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked())->BooleanValue()
      : false;

  VideoFramePacketizer* obj = new VideoFramePacketizer();
  owt_base::VideoFramePacketizer::Config config;
  config.enableRed = supportRED;
  config.enableUlpfec = supportULPFEC;
  config.transportccExt = transportccExt;
  config.mid = mid;
  config.midExtId = midExtId;
  config.enableBandwidthEstimation = enableBandwidthEstimation;
  if (baseWrapper) {
    config.rtcAdapter = baseWrapper->rtcAdapter;
  }

  if (transportccExt > 0) {
    config.enableTransportcc = true;
    config.selfRequestKeyframe = false;
    obj->me = new owt_base::VideoFramePacketizer(config);
  } else if (selfRequestKeyframe) {
    config.enableTransportcc = false;
    config.selfRequestKeyframe = true;
    obj->me = new owt_base::VideoFramePacketizer(config);
  } else {
    config.enableTransportcc = false;
    obj->me = new owt_base::VideoFramePacketizer(config);
  }
  obj->dest = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void VideoFramePacketizer::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;
  delete me;
}

void VideoFramePacketizer::bindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;

  MediaFilter* param = Nan::ObjectWrap::Unwrap<MediaFilter>(Nan::To<v8::Object>(args[0]).ToLocalChecked());
  erizo::MediaSink* transport = param->msink;

  me->bindTransport(transport);
}

void VideoFramePacketizer::unbindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;

  me->unbindTransport();
}

void VideoFramePacketizer::enable(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;

  bool b = (args[0]->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked())->BooleanValue();
  me->enable(b);
}

void VideoFramePacketizer::getSsrc(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;

  uint32_t ssrc = me->getSsrc();
  args.GetReturnValue().Set(Number::New(isolate, ssrc));
}

void VideoFramePacketizer::getTotalBitrate(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;

  uint32_t bitrate = me->getTotalBitrate();
  args.GetReturnValue().Set(Number::New(isolate, bitrate));
}

void VideoFramePacketizer::getRetransmitBitrate(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;

  uint32_t bitrate = me->getRetransmitBitrate();
  args.GetReturnValue().Set(Number::New(isolate, bitrate));
}

void VideoFramePacketizer::getEstimatedBandwidth(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;

  uint32_t bitrate = me->getEstimatedBandwidth();
  args.GetReturnValue().Set(Number::New(isolate, bitrate));
}
