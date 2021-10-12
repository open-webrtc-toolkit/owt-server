// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "MediaStream.h"
#include "WebRtcConnection.h"

#include <future>  // NOLINT

#include <json.hpp>
#include "ThreadPool.h"

using v8::HandleScope;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Persistent;
using v8::Exception;
using v8::Value;
using json = nlohmann::json;

DEFINE_LOGGER(MediaStream, "MediaStreamWrapper");

static std::string getString(v8::Local<v8::Value> value) {
  Nan::Utf8String value_str(Nan::To<v8::String>(value).ToLocalChecked());
  return std::string(*value_str);
}

StatCallWorker::StatCallWorker(Nan::Callback *callback, std::weak_ptr<erizo::MediaStream> weak_stream)
    : Nan::AsyncWorker{callback}, weak_stream_{weak_stream}, stat_{""} {
}

void StatCallWorker::Execute() {
  std::promise<std::string> stat_promise;
  std::future<std::string> stat_future = stat_promise.get_future();
  if (auto stream = weak_stream_.lock()) {
    stream->getJSONStats([&stat_promise] (std::string stats) {
      stat_promise.set_value(stats);
    });
  }
  stat_future.wait();
  stat_ = stat_future.get();
}

void StatCallWorker::HandleOKCallback() {
  Local<Value> argv[] = {
    Nan::New<v8::String>(stat_).ToLocalChecked()
  };
  callback->Call(1, argv, async_resource);
}

void destroyAsyncHandle(uv_handle_t *handle) {
  delete handle;
}

Nan::Persistent<Function> MediaStream::constructor;

MediaStream::MediaStream() : closed_{false}, id_{"undefined"} {
  async_stats_ = new uv_async_t;
  async_event_ = new uv_async_t;
  uv_async_init(uv_default_loop(), async_stats_, &MediaStream::statsCallback);
  uv_async_init(uv_default_loop(), async_event_, &MediaStream::eventCallback);
}

MediaStream::~MediaStream() {
  close();
  ELOG_DEBUG("%s, message: Destroyed", toLog());
}

void MediaStream::close() {
  ELOG_DEBUG("%s, message: Trying to close", toLog());
  if (closed_) {
    ELOG_DEBUG("%s, message: Already closed", toLog());
    return;
  }
  ELOG_DEBUG("%s, message: Closing", toLog());
  if (me) {
    me->setMediaStreamStatsListener(nullptr);
    me->setMediaStreamEventListener(nullptr);
    me->close();
    me.reset();
  }
  has_stats_callback_ = false;
  has_event_callback_ = false;
  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(async_stats_))) {
    ELOG_DEBUG("%s, message: Closing Stats handle", toLog());
    uv_close(reinterpret_cast<uv_handle_t*>(async_stats_), destroyAsyncHandle);
  }
  async_stats_ = nullptr;
  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(async_event_))) {
    ELOG_DEBUG("%s, message: Closing MediaStreamEvent handle", toLog());
    uv_close(reinterpret_cast<uv_handle_t*>(async_event_), destroyAsyncHandle);
  }
  async_event_ = nullptr;
  closed_ = true;
  ELOG_DEBUG("%s, message: Closed", toLog());
}

std::string MediaStream::toLog() {
  return "id: " + id_;
}

NAN_MODULE_INIT(MediaStream::Init) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("MediaStream").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "close", close);
  Nan::SetPrototypeMethod(tpl, "init", init);
  Nan::SetPrototypeMethod(tpl, "setAudioReceiver", setAudioReceiver);
  Nan::SetPrototypeMethod(tpl, "setVideoReceiver", setVideoReceiver);
  Nan::SetPrototypeMethod(tpl, "getCurrentState", getCurrentState);
  Nan::SetPrototypeMethod(tpl, "generatePLIPacket", generatePLIPacket);
  Nan::SetPrototypeMethod(tpl, "getStats", getStats);
  Nan::SetPrototypeMethod(tpl, "getPeriodicStats", getPeriodicStats);
  Nan::SetPrototypeMethod(tpl, "setFeedbackReports", setFeedbackReports);
  Nan::SetPrototypeMethod(tpl, "setSlideShowMode", setSlideShowMode);
  Nan::SetPrototypeMethod(tpl, "muteStream", muteStream);
  Nan::SetPrototypeMethod(tpl, "setMaxVideoBW", setMaxVideoBW);
  Nan::SetPrototypeMethod(tpl, "setQualityLayer", setQualityLayer);
  Nan::SetPrototypeMethod(tpl, "enableSlideShowBelowSpatialLayer", enableSlideShowBelowSpatialLayer);
  Nan::SetPrototypeMethod(tpl, "onMediaStreamEvent", onMediaStreamEvent);
  Nan::SetPrototypeMethod(tpl, "setVideoConstraints", setVideoConstraints);
  Nan::SetPrototypeMethod(tpl, "setMetadata", setMetadata);
  Nan::SetPrototypeMethod(tpl, "enableHandler", enableHandler);
  Nan::SetPrototypeMethod(tpl, "disableHandler", disableHandler);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("MediaStream").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}


NAN_METHOD(MediaStream::New) {
  if (info.Length() < 3) {
    Nan::ThrowError("Wrong number of arguments");
  }

  if (info.IsConstructCall()) {
    // Invoked as a constructor with 'new MediaStream()'
    ThreadPool* thread_pool = Nan::ObjectWrap::Unwrap<ThreadPool>(Nan::To<v8::Object>(info[0]).ToLocalChecked());

    WebRtcConnection* connection =
     Nan::ObjectWrap::Unwrap<WebRtcConnection>(Nan::To<v8::Object>(info[1]).ToLocalChecked());

    std::shared_ptr<erizo::WebRtcConnection> wrtc = connection->me;

    std::string wrtc_id = getString(info[2]);
    std::string stream_label = getString(info[3]);

    bool is_publisher = Nan::To<bool>(info[5]).FromJust();

    MediaStream* obj = new MediaStream();
    // Share same worker with connection
    obj->me = std::make_shared<erizo::MediaStream>(wrtc->getWorker(), wrtc, wrtc_id, stream_label, is_publisher);
    obj->msink = obj->me.get();
    obj->msource = obj->me.get();
    obj->id_ = wrtc_id;
    obj->label_ = stream_label;
    ELOG_DEBUG("%s, message: Created", obj->toLog());
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    obj->asyncResource_ = new Nan::AsyncResource("MediaStreamCallback");
  } else {
    // TODO(pedro) Check what happens here
  }
}

NAN_METHOD(MediaStream::close) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  if (obj) {
    delete obj->asyncResource_;
    obj->close();
  }
}

NAN_METHOD(MediaStream::init) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }
  bool r = me->init();

  info.GetReturnValue().Set(Nan::New(r));
}

NAN_METHOD(MediaStream::setSlideShowMode) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  bool v = Nan::To<bool>(info[0]).FromJust();
  me->setSlideShowMode(v);
  info.GetReturnValue().Set(Nan::New(v));
}

NAN_METHOD(MediaStream::muteStream) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  bool mute_video = Nan::To<bool>(info[0]).FromJust();
  bool mute_audio = Nan::To<bool>(info[1]).FromJust();
  me->muteStream(mute_video, mute_audio);
}

NAN_METHOD(MediaStream::setMaxVideoBW) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  int max_video_bw = info[0]->IntegerValue(Nan::GetCurrentContext()).ToChecked();
  me->setMaxVideoBW(max_video_bw);
}

NAN_METHOD(MediaStream::setVideoConstraints) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }
  int max_video_width = Nan::To<int32_t>(info[0]).FromJust();
  int max_video_height = Nan::To<int32_t>(info[1]).FromJust();
  int max_video_frame_rate = Nan::To<int32_t>(info[2]).FromJust();
  me->setVideoConstraints(max_video_width, max_video_height, max_video_frame_rate);
}

NAN_METHOD(MediaStream::setMetadata) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  std::string metadata_string = getString(info[0]);
  json metadata_json = json::parse(metadata_string);
  std::map<std::string, std::string> metadata;
  for (json::iterator item = metadata_json.begin(); item != metadata_json.end(); ++item) {
    std::string value = item.value().dump();
    if (item.value().is_object()) {
      value = "[object]";
    }
    if (item.value().is_string()) {
      value = item.value();
    }
    metadata[item.key()] = value;
  }

  me->setMetadata(metadata);

  info.GetReturnValue().Set(Nan::New(true));
}


NAN_METHOD(MediaStream::getCurrentState) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  int state = me->getCurrentState();

  info.GetReturnValue().Set(Nan::New(state));
}


NAN_METHOD(MediaStream::setAudioReceiver) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  MediaSink* param = Nan::ObjectWrap::Unwrap<MediaSink>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  erizo::MediaSink *mr = param->msink;

  me->setAudioSink(mr);
  me->setEventSink(mr);
}

NAN_METHOD(MediaStream::setVideoReceiver) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  MediaSink* param = Nan::ObjectWrap::Unwrap<MediaSink>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  erizo::MediaSink *mr = param->msink;

  me->setVideoSink(mr);
  me->setEventSink(mr);
}


NAN_METHOD(MediaStream::generatePLIPacket) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());

  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }
  me->sendPLI();
}

NAN_METHOD(MediaStream::enableHandler) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  std::string name = getString(info[0]);

  me->enableHandler(name);
  return;
}

NAN_METHOD(MediaStream::disableHandler) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  std::string name = getString(info[0]);

  me->disableHandler(name);
}

NAN_METHOD(MediaStream::setQualityLayer) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  int spatial_layer = Nan::To<int32_t>(info[0]).FromJust();
  int temporal_layer = Nan::To<int32_t>(info[1]).FromJust();

  me->setQualityLayer(spatial_layer, temporal_layer);
}

NAN_METHOD(MediaStream::enableSlideShowBelowSpatialLayer) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  bool enabled = Nan::To<bool>(info[0]).FromJust();
  int spatial_layer = Nan::To<int32_t>(info[1]).FromJust();
  me->enableSlideShowBelowSpatialLayer(enabled, spatial_layer);
}

NAN_METHOD(MediaStream::getStats) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  if (!obj->me || info.Length() != 1) {
    return;
  }
  Nan::Callback *callback = new Nan::Callback(info[0].As<Function>());
  AsyncQueueWorker(new StatCallWorker(callback, obj->me));
}

NAN_METHOD(MediaStream::getPeriodicStats) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  if (obj->me == nullptr || info.Length() != 1) {
    return;
  }
  obj->me->setMediaStreamStatsListener(obj);
  obj->has_stats_callback_ = true;
  obj->stats_callback_ = new Nan::Callback(info[0].As<Function>());
}

NAN_METHOD(MediaStream::setFeedbackReports) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }

  bool v = Nan::To<bool>(info[0]).FromJust();
  int fbreps = Nan::To<int32_t>(info[1]).FromJust();  // From bps to Kbps
  me->setFeedbackReports(v, fbreps);
}

NAN_METHOD(MediaStream::onMediaStreamEvent) {
  MediaStream* obj = Nan::ObjectWrap::Unwrap<MediaStream>(info.Holder());
  std::shared_ptr<erizo::MediaStream> me = obj->me;
  if (!me) {
    return;
  }
  me ->setMediaStreamEventListener(obj);
  obj->has_event_callback_ = true;
  obj->event_callback_ = new Nan::Callback(info[0].As<Function>());
}

void MediaStream::notifyStats(const std::string& message) {
  if (!this->has_stats_callback_) {
    return;
  }
  if (!async_stats_) {
    return;
  }
  boost::mutex::scoped_lock lock(mutex);
  this->stats_messages.push(message);
  async_stats_->data = this;
  uv_async_send(async_stats_);
}

void MediaStream::notifyMediaStreamEvent(const std::string& type, const std::string& message) {
  if (!this->has_event_callback_) {
    return;
  }
  if (!async_event_) {
    return;
  }
  boost::mutex::scoped_lock lock(mutex);
  this->event_messages.push(std::make_pair(type, message));
  async_event_->data = this;
  uv_async_send(async_event_);
}

NAUV_WORK_CB(MediaStream::statsCallback) {
  Nan::HandleScope scope;
  MediaStream* obj = reinterpret_cast<MediaStream*>(async->data);
  if (!obj || !obj->me) {
    return;
  }
  boost::mutex::scoped_lock lock(obj->mutex);
  if (obj->has_stats_callback_) {
    while (!obj->stats_messages.empty()) {
      Local<Value> args[] = {Nan::New(obj->stats_messages.front().c_str()).ToLocalChecked()};
      obj->asyncResource_->runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->stats_callback_->GetFunction(), 1, args);
      obj->stats_messages.pop();
    }
  }
}

NAUV_WORK_CB(MediaStream::eventCallback) {
  Nan::HandleScope scope;
  MediaStream* obj = reinterpret_cast<MediaStream*>(async->data);
  if (!obj || !obj->me) {
    return;
  }
  boost::mutex::scoped_lock lock(obj->mutex);
  ELOG_DEBUG("%s, message: eventsCallback", obj->toLog());
  if (obj->has_event_callback_) {
      while (!obj->event_messages.empty()) {
          Local<Value> args[] = {Nan::New(obj->event_messages.front().first.c_str()).ToLocalChecked(),
              Nan::New(obj->event_messages.front().second.c_str()).ToLocalChecked()};
          obj->asyncResource_->runInAsyncScope(Nan::GetCurrentContext()->Global(), obj->event_callback_->GetFunction(), 2, args);
          obj->event_messages.pop();
      }
  }
  ELOG_DEBUG("%s, message: eventsCallback finished", obj->toLog());
}
