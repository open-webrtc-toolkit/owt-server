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

#include "WebRtcConnection.h"
#include "MediaStream.h"

#include <future>  // NOLINT

#include <json.hpp>
#include "IOThreadPool.h"
#include "ThreadPool.h"

using v8::HandleScope;
using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Persistent;
using v8::Exception;
using v8::Value;
using json = nlohmann::json;

std::string getString(v8::Local<v8::Value> value) {
  v8::String::Utf8Value value_str(Nan::To<v8::String>(value).ToLocalChecked()); \
  return std::string(*value_str);
}

Nan::Persistent<Function> WebRtcConnection::constructor;

WebRtcConnection::WebRtcConnection() {
}

WebRtcConnection::~WebRtcConnection() {
  if (me.get() != nullptr) {
    me->setWebRtcConnectionEventListener(NULL);
  }
}

NAN_MODULE_INIT(WebRtcConnection::Init) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("WebRtcConnection").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "stop", stop);
  Nan::SetPrototypeMethod(tpl, "close", close);
  Nan::SetPrototypeMethod(tpl, "init", init);
  Nan::SetPrototypeMethod(tpl, "setRemoteSdp", setRemoteSdp);
  Nan::SetPrototypeMethod(tpl, "addRemoteCandidate", addRemoteCandidate);
  Nan::SetPrototypeMethod(tpl, "removeRemoteCandidate", removeRemoteCandidate);
  Nan::SetPrototypeMethod(tpl, "getLocalSdp", getLocalSdp);
  Nan::SetPrototypeMethod(tpl, "getCurrentState", getCurrentState);
  Nan::SetPrototypeMethod(tpl, "createOffer", createOffer);
  Nan::SetPrototypeMethod(tpl, "addMediaStream", addMediaStream);
  Nan::SetPrototypeMethod(tpl, "removeMediaStream", removeMediaStream);

  Nan::SetPrototypeMethod(tpl, "setAudioSsrc", setAudioSsrc);
  Nan::SetPrototypeMethod(tpl, "getAudioSsrcMap", getAudioSsrcMap);
  Nan::SetPrototypeMethod(tpl, "setVideoSsrcList", setVideoSsrcList);
  Nan::SetPrototypeMethod(tpl, "getVideoSsrcMap", getVideoSsrcMap);

  constructor.Reset(tpl->GetFunction());
  Nan::Set(target, Nan::New("WebRtcConnection").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}


NAN_METHOD(WebRtcConnection::New) {
  if (info.Length() < 7) {
    Nan::ThrowError("Wrong number of arguments");
  }

  if (info.IsConstructCall()) {
    // Invoked as a constructor with 'new WebRTC()'
    ThreadPool* thread_pool = Nan::ObjectWrap::Unwrap<ThreadPool>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
    IOThreadPool* io_thread_pool = Nan::ObjectWrap::Unwrap<IOThreadPool>(Nan::To<v8::Object>(info[1]).ToLocalChecked());
    v8::String::Utf8Value paramId(Nan::To<v8::String>(info[2]).ToLocalChecked());
    std::string wrtcId = std::string(*paramId);
    v8::String::Utf8Value param(Nan::To<v8::String>(info[3]).ToLocalChecked());
    std::string stunServer = std::string(*param);
    int stunPort = info[4]->IntegerValue();
    int minPort = info[5]->IntegerValue();
    int maxPort = info[6]->IntegerValue();
    bool trickle = (info[7]->ToBoolean())->BooleanValue();
    v8::String::Utf8Value json_param(Nan::To<v8::String>(info[8]).ToLocalChecked());
    bool use_nicer = (info[9]->ToBoolean())->BooleanValue();
    std::string media_config_string = std::string(*json_param);
    json media_config = json::parse(media_config_string);
    std::vector<erizo::RtpMap> rtp_mappings;

    if (media_config.find("rtpMappings") != media_config.end()) {
      json rtp_map_json = media_config["rtpMappings"];
      for (json::iterator it = rtp_map_json.begin(); it != rtp_map_json.end(); ++it) {
        erizo::RtpMap rtp_map;
        if (it.value()["payloadType"].is_number()) {
          rtp_map.payload_type = it.value()["payloadType"];
        } else {
          continue;
        }
        if (it.value()["encodingName"].is_string()) {
          rtp_map.encoding_name = it.value()["encodingName"];
        } else {
          continue;
        }
        if (it.value()["mediaType"].is_string()) {
          if (it.value()["mediaType"] == "video") {
            rtp_map.media_type = erizo::VIDEO_TYPE;
          } else if (it.value()["mediaType"] == "audio") {
            rtp_map.media_type = erizo::AUDIO_TYPE;
          } else {
            continue;
          }
        } else {
          continue;
        }
        if (it.value()["clockRate"].is_number()) {
          rtp_map.clock_rate = it.value()["clockRate"];
        }
        if (rtp_map.media_type == erizo::AUDIO_TYPE) {
          if (it.value()["channels"].is_number()) {
            rtp_map.channels = it.value()["channels"];
          }
        }
        if (it.value()["formatParameters"].is_object()) {
          json format_params_json = it.value()["formatParameters"];
          for (json::iterator params_it = format_params_json.begin();
              params_it != format_params_json.end(); ++params_it) {
            std::string value = params_it.value();
            std::string key = params_it.key();
            rtp_map.format_parameters.insert(rtp_map.format_parameters.begin(),
                std::pair<std::string, std::string> (key, value));
          }
        }
        if (it.value()["feedbackTypes"].is_array()) {
          json feedback_types_json = it.value()["feedbackTypes"];
          for (json::iterator feedback_it = feedback_types_json.begin();
              feedback_it != feedback_types_json.end(); ++feedback_it) {
              rtp_map.feedback_types.push_back(*feedback_it);
          }
        }
        rtp_mappings.push_back(rtp_map);
      }
    }

    std::vector<erizo::ExtMap> ext_mappings;
    unsigned int value = 0;
    if (media_config.find("extMappings") != media_config.end()) {
      json ext_map_json = media_config["extMappings"];
      for (json::iterator ext_map_it = ext_map_json.begin(); ext_map_it != ext_map_json.end(); ++ext_map_it) {
        ext_mappings.push_back({value++, *ext_map_it});
      }
    }

    erizo::IceConfig iceConfig;
    if (info.Length() == 16) {
      v8::String::Utf8Value param2(Nan::To<v8::String>(info[10]).ToLocalChecked());
      std::string turnServer = std::string(*param2);
      int turnPort = info[11]->IntegerValue();
      v8::String::Utf8Value param3(Nan::To<v8::String>(info[12]).ToLocalChecked());
      std::string turnUsername = std::string(*param3);
      v8::String::Utf8Value param4(Nan::To<v8::String>(info[13]).ToLocalChecked());
      std::string turnPass = std::string(*param4);
      v8::String::Utf8Value param5(Nan::To<v8::String>(info[14]).ToLocalChecked());
      std::string network_interface = std::string(*param5);

      std::vector<std::string> ipAddresses;
      Local<v8::Array> array = Local<v8::Array>::Cast(info[15]);
      for (unsigned int i = 0; i < array->Length(); i++) {
        if (Nan::Has(array, i).FromJust()) {
          v8::String::Utf8Value addr(Nan::To<v8::String>(Nan::Get(array, i).ToLocalChecked()).ToLocalChecked());
          ipAddresses.push_back(std::string(*addr));
        }
      }

      iceConfig.turn_server = turnServer;
      iceConfig.turn_port = turnPort;
      iceConfig.turn_username = turnUsername;
      iceConfig.turn_pass = turnPass;
      iceConfig.ip_addresses = ipAddresses;
    }


    iceConfig.stun_server = stunServer;
    iceConfig.stun_port = stunPort;
    iceConfig.min_port = minPort;
    iceConfig.max_port = maxPort;
    iceConfig.should_trickle = trickle;
    iceConfig.use_nicer = use_nicer;

    std::shared_ptr<erizo::Worker> worker = thread_pool->me->getLessUsedWorker();
    std::shared_ptr<erizo::IOWorker> io_worker = io_thread_pool->me->getLessUsedIOWorker();

    WebRtcConnection* obj = new WebRtcConnection();
    obj->me = std::make_shared<erizo::WebRtcConnection>(worker, io_worker, wrtcId, iceConfig,
                                                        rtp_mappings, ext_mappings, obj);

    uv_async_init(uv_default_loop(), &obj->async_, &WebRtcConnection::eventsCallback);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    // TODO(pedro) Check what happens here
  }
}

std::future<void> stopAsync(const std::shared_ptr<erizo::MediaStream> media_stream) {
  auto promise = std::make_shared<std::promise<void>>();
  if (media_stream) {
    media_stream->getWorker()->task([promise, media_stream] {
      media_stream->getFeedbackSource()->setFeedbackSink(nullptr);
      media_stream->setVideoSink(nullptr);
      media_stream->setAudioSink(nullptr);
      media_stream->setEventSink(nullptr);
      promise->set_value();
    });
  } else {
    promise->set_value();
  }
  return promise->get_future();
}

NAN_METHOD(WebRtcConnection::stop) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());

  obj->me->forEachMediaStream([] (const std::shared_ptr<erizo::MediaStream> &media_stream) {
    std::future<void> future = stopAsync(media_stream);
    future.wait();
  });
}

NAN_METHOD(WebRtcConnection::close) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  obj->me->setWebRtcConnectionEventListener(NULL);
  obj->me->close();
  obj->me.reset();
  obj->hasCallback_ = false;

  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&obj->async_))) {
    uv_close(reinterpret_cast<uv_handle_t*>(&obj->async_), NULL);
  }
}

NAN_METHOD(WebRtcConnection::init) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;

  obj->eventCallback_ = new Nan::Callback(info[0].As<Function>());
  bool r = me->init();

  info.GetReturnValue().Set(Nan::New(r));
}

NAN_METHOD(WebRtcConnection::createOffer) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;
  if (info.Length() < 3) {
    Nan::ThrowError("Wrong number of arguments");
  }
  bool video_enabled = info[0]->BooleanValue();
  bool audio_enabled = info[1]->BooleanValue();
  bool bundle = info[2]->BooleanValue();

  bool r = me->createOffer(video_enabled, audio_enabled, bundle);
  info.GetReturnValue().Set(Nan::New(r));
}

NAN_METHOD(WebRtcConnection::setRemoteSdp) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;
  if (!me) {
    return;
  }

  v8::String::Utf8Value param(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string sdp = std::string(*param);

  v8::String::Utf8Value stream_id_param(Nan::To<v8::String>(info[1]).ToLocalChecked());
  std::string stream_id = std::string(*stream_id_param);

  bool r = me->setRemoteSdp(sdp, stream_id);

  info.GetReturnValue().Set(Nan::New(r));
}

NAN_METHOD(WebRtcConnection::addRemoteCandidate) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;

  v8::String::Utf8Value param(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string mid = std::string(*param);

  int sdpMLine = info[1]->IntegerValue();

  v8::String::Utf8Value param2(Nan::To<v8::String>(info[2]).ToLocalChecked());
  std::string sdp = std::string(*param2);

  bool r = me->addRemoteCandidate(mid, sdpMLine, sdp);

  info.GetReturnValue().Set(Nan::New(r));
}

NAN_METHOD(WebRtcConnection::removeRemoteCandidate) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;

  v8::String::Utf8Value param(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string mid = std::string(*param);

  int sdpMLine = info[1]->IntegerValue();

  v8::String::Utf8Value param2(Nan::To<v8::String>(info[2]).ToLocalChecked());
  std::string sdp = std::string(*param2);

  bool r = me->removeRemoteCandidate(mid, sdpMLine, sdp);
  info.GetReturnValue().Set(Nan::New(r));
}

NAN_METHOD(WebRtcConnection::addMediaStream) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;
  if (!me) {
    return;
  }

  MediaStream* param = Nan::ObjectWrap::Unwrap<MediaStream>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  auto wr = std::shared_ptr<erizo::MediaStream>(param->me);

  me->addMediaStream(wr);
}

NAN_METHOD(WebRtcConnection::removeMediaStream) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;
  if (!me) {
    return;
  }

  v8::String::Utf8Value param(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string streamId = std::string(*param);

  me->forEachMediaStream([streamId] (const std::shared_ptr<erizo::MediaStream> &media_stream) {
    if (media_stream->getId() == streamId) {
      std::future<void> future = stopAsync(media_stream);
      future.wait();
    }
  });
}

NAN_METHOD(WebRtcConnection::setAudioSsrc) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;
  if (!me) {
    return;
  }

  std::string stream_id = getString(info[0]);
  me->getLocalSdpInfo()->audio_ssrc_map[stream_id] = info[1]->IntegerValue();
}

NAN_METHOD(WebRtcConnection::getAudioSsrcMap) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;
  if (!me) {
    return;
  }

  Local<v8::Object> audio_ssrc_map = Nan::New<v8::Object>();
  for (auto const& audio_ssrcs : me->getLocalSdpInfo()->audio_ssrc_map) {
    audio_ssrc_map->Set(Nan::New(audio_ssrcs.first.c_str()).ToLocalChecked(),
                        Nan::New(audio_ssrcs.second));
  }
  info.GetReturnValue().Set(audio_ssrc_map);
}

NAN_METHOD(WebRtcConnection::setVideoSsrcList) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;
  if (!me) {
    return;
  }

  std::string stream_id = getString(info[0]);
  v8::Local<v8::Array> video_ssrc_array = v8::Local<v8::Array>::Cast(info[1]);
  std::vector<uint32_t> video_ssrc_list;

  for (unsigned int i = 0; i < video_ssrc_array->Length(); i++) {
    v8::Handle<v8::Value> val = video_ssrc_array->Get(i);
    unsigned int numVal = val->IntegerValue();
    video_ssrc_list.push_back(numVal);
  }
  me->getLocalSdpInfo()->video_ssrc_map[stream_id] = video_ssrc_list;
}

NAN_METHOD(WebRtcConnection::getVideoSsrcMap) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;
  if (!me) {
    return;
  }

  Local<v8::Object> video_ssrc_map = Nan::New<v8::Object>();
  for (auto const& video_ssrcs : me->getLocalSdpInfo()->video_ssrc_map) {
    v8::Local<v8::Array> array = Nan::New<v8::Array>(video_ssrcs.second.size());
    uint index = 0;
    for (uint32_t ssrc : video_ssrcs.second) {
      Nan::Set(array, index++, Nan::New(ssrc));
    }
    video_ssrc_map->Set(Nan::New(video_ssrcs.first.c_str()).ToLocalChecked(), array);
  }
  info.GetReturnValue().Set(video_ssrc_map);
}

NAN_METHOD(WebRtcConnection::getLocalSdp) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;

  std::string sdp = me->getLocalSdp();

  info.GetReturnValue().Set(Nan::New(sdp.c_str()).ToLocalChecked());
}

NAN_METHOD(WebRtcConnection::getCurrentState) {
  WebRtcConnection* obj = Nan::ObjectWrap::Unwrap<WebRtcConnection>(info.Holder());
  std::shared_ptr<erizo::WebRtcConnection> me = obj->me;

  int state = me->getCurrentState();

  info.GetReturnValue().Set(Nan::New(state));
}

// Async methods
void WebRtcConnection::notifyEvent(erizo::WebRTCEvent event, const std::string& message, const std::string& stream_id) {
  boost::mutex::scoped_lock lock(mutex);
  this->eventSts.push(event);
  this->eventMsgs.emplace(message, stream_id);
  async_.data = this;
  uv_async_send(&async_);
}

NAUV_WORK_CB(WebRtcConnection::eventsCallback) {
  Nan::HandleScope scope;
  WebRtcConnection* obj = reinterpret_cast<WebRtcConnection*>(async->data);
  if (!obj || obj->me == NULL)
    return;
  boost::mutex::scoped_lock lock(obj->mutex);
  while (!obj->eventSts.empty()) {
    Local<Value> args[] = {
      Nan::New(obj->eventSts.front()),
      Nan::New(obj->eventMsgs.front().first.c_str()).ToLocalChecked(),
      Nan::New(obj->eventMsgs.front().second.c_str()).ToLocalChecked()
    };
    Nan::MakeCallback(Nan::GetCurrentContext()->Global(), obj->eventCallback_->GetFunction(), 3, args);
    obj->eventMsgs.pop();
    obj->eventSts.pop();
  }
}
