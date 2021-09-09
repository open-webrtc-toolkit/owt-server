// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef CALLBASEWRAPPER_H
#define CALLBASEWRAPPER_H

#include <RtcAdapter.h>
#include <nan.h>

/*
 * Wrapper class of RtcAdapter for call
 */
class CallBase : public Nan::ObjectWrap {
 public:
  static NAN_MODULE_INIT(Init);

  std::shared_ptr<rtc_adapter::RtcAdapter> rtcAdapter;
 private:
  CallBase();
  ~CallBase();

  static NAN_METHOD(New);
  static NAN_METHOD(close);

  static Nan::Persistent<v8::Function> constructor;
};

#endif
