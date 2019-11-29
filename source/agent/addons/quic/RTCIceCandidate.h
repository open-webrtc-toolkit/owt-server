/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_RTCICECANDIDATE_H_
#define WEBRTC_RTCICECANDIDATE_H_

#include <memory>

#include <logger.h>
#include <SdpInfo.h>
#include <nan.h>


// Node.js addon of RTCIceCandidate.
// https://w3c.github.io/webrtc-pc/#rtcicecandidate-interface
class RTCIceCandidate : public Nan::ObjectWrap {
  DECLARE_LOGGER();

  public:
  static NAN_MODULE_INIT(Init);
  static v8::Local<v8::Object> newInstance(const erizo::CandidateInfo& candidate);
  erizo::CandidateInfo candidateInfo() const;
  std::string toString() const;

  private:
  explicit RTCIceCandidate();

  static NAN_METHOD(newInstance);
  static NAN_GETTER(candidateGetter);
  static NAN_GETTER(sdpMidGetter);
  static NAN_GETTER(sdpMLineIndexGetter);

  // Deserialize candidate and set it to m_candidate. Return false if error happened.
  std::unique_ptr<erizo::CandidateInfo> deserialize(const std::string& candidate) const;
  std::vector<std::string> split(const std::string& str) const;

  std::unique_ptr<erizo::CandidateInfo> m_candidate;
  std::string m_sdpMid;
  static Nan::Persistent<v8::Function> s_constructor;
};

#endif