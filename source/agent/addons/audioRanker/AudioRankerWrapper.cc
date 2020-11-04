// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioRankerWrapper.h"
#include "MediaFramePipelineWrapper.h"

using namespace v8;

Nan::Persistent<Function> AudioRanker::constructor;

AudioRanker::AudioRanker() {}
AudioRanker::~AudioRanker() {}

NAN_MODULE_INIT(AudioRanker::Init) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("AudioRanker").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "close", close);
  Nan::SetPrototypeMethod(tpl, "addOutput", addOutput);
  Nan::SetPrototypeMethod(tpl, "addInput", addInput);
  Nan::SetPrototypeMethod(tpl, "removeInput", removeInput);

  constructor.Reset(tpl->GetFunction());
  Nan::Set(target, Nan::New("AudioRanker").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(AudioRanker::New) {
  if (info.Length() < 1) {
    Nan::ThrowError("Wrong number of arguments");
  }

  if (info.IsConstructCall()) {
    AudioRanker* obj = new AudioRanker();

    obj->asyncResource_ = new Nan::AsyncResource("RankChange");
    obj->callback_ = new Nan::Callback(info[0].As<Function>());
    uv_async_init(uv_default_loop(), &obj->async_, &AudioRanker::Callback);

    if (info.Length() > 2) {
        bool detectMute = Nan::To<bool>(info[1]).FromJust();
        int changeInterval = Nan::To<int>(info[2]).FromJust();
        obj->me = new owt_base::AudioRanker(obj, detectMute, changeInterval);
    } else {
        obj->me = new owt_base::AudioRanker(obj);
    }

    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    // Not construct call
  }
}

NAN_METHOD(AudioRanker::close) {
  AudioRanker* obj = Nan::ObjectWrap::Unwrap<AudioRanker>(info.Holder());

  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&obj->async_))) {
    uv_close(reinterpret_cast<uv_handle_t*>(&obj->async_), NULL);
  }

  delete obj->me;
  obj->me = nullptr;
  delete obj->callback_;
  delete obj->asyncResource_;
}

NAN_METHOD(AudioRanker::addOutput) {
  AudioRanker* obj = Nan::ObjectWrap::Unwrap<AudioRanker>(info.Holder());
  owt_base::AudioRanker* me = obj->me;

  FrameDestination* param = node::ObjectWrap::Unwrap<FrameDestination>(info[0]->ToObject());
  owt_base::FrameDestination* dest = param->dest;

  me->addOutput(dest);
}

NAN_METHOD(AudioRanker::addInput) {
  AudioRanker* obj = Nan::ObjectWrap::Unwrap<AudioRanker>(info.Holder());
  owt_base::AudioRanker* me = obj->me;

  FrameSource* param = node::ObjectWrap::Unwrap<FrameSource>(info[0]->ToObject());
  owt_base::FrameSource* src = param->src;

  Nan::Utf8String streamIdPara(Nan::To<v8::String>(info[1]).ToLocalChecked());
  std::string streamId = std::string(*streamIdPara);

  Nan::Utf8String ownerIdPara(Nan::To<v8::String>(info[2]).ToLocalChecked());
  std::string ownerId = std::string(*ownerIdPara);

  me->addInput(src, streamId, ownerId);
}

NAN_METHOD(AudioRanker::removeInput) {
  AudioRanker* obj = Nan::ObjectWrap::Unwrap<AudioRanker>(info.Holder());
  owt_base::AudioRanker* me = obj->me;

  Nan::Utf8String streamIdPara(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string streamId = std::string(*streamIdPara);

  me->removeInput(streamId);
}

void AudioRanker::onRankChange(std::vector<std::pair<std::string, std::string>> updates) {
  boost::mutex::scoped_lock lock(mutex);
  std::ostringstream jsonChange;
  /*
   * The JS callback json
   * [
   *    ["streamID0", "ownerID0"],
   *    ["streamID1", "ownerID1"],
   *    ......
   *    ["streamIDn", "ownerIDn"]
   * ]
   */
  jsonChange.str("");
  jsonChange << "[";
  for (size_t i = 0; i < updates.size(); i++) {
    jsonChange << "[\"" << updates[i].first << "\",\""
        << updates[i].second << "\"]";
    if (i + 1 < updates.size()) {
      jsonChange << ",";
    }
  }
  jsonChange << "]";
  this->jsonChanges.push(jsonChange.str());
  async_.data = this;
  uv_async_send(&async_);
}

NAUV_WORK_CB(AudioRanker::Callback) {
  Nan::HandleScope scope;
  AudioRanker* obj = reinterpret_cast<AudioRanker*>(async->data);
  if (!obj || obj->me == NULL)
    return;
  boost::mutex::scoped_lock lock(obj->mutex);
  while (!obj->jsonChanges.empty()) {
    Local<Value> args[] = {Nan::New(obj->jsonChanges.front().c_str()).ToLocalChecked()};
    obj->asyncResource_->runInAsyncScope(
      Nan::GetCurrentContext()->Global(), obj->callback_->GetFunction(), 1, args);
    obj->jsonChanges.pop();
  }
}

