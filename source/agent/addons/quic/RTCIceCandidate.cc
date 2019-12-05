/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <iterator>
#include <string>

#include "RTCIceCandidate.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

Nan::Persistent<v8::Function> RTCIceCandidate::s_constructor;

DEFINE_LOGGER(RTCIceCandidate, "RTCIceCandidate");

RTCIceCandidate::RTCIceCandidate() {}

NAN_MODULE_INIT(RTCIceCandidate::Init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("RTCIceCandidate").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);
    s_constructor.Reset(tpl->GetFunction());

    Nan::SetAccessor(instanceTpl, Nan::New("candidate").ToLocalChecked(), candidateGetter);
    Nan::SetAccessor(instanceTpl, Nan::New("sdpMid").ToLocalChecked(), sdpMidGetter);
    Nan::SetAccessor(instanceTpl, Nan::New("sdpMLineIndex").ToLocalChecked(), sdpMLineIndexGetter);

    Nan::Set(target, Nan::New("RTCIceCandidate").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(RTCIceCandidate::newInstance)
{
    if (!info.IsConstructCall()) {
        return;
    }
    RTCIceCandidate* obj = new RTCIceCandidate();
    obj->m_candidate.reset(new erizo::CandidateInfo());
    if (info.Length() == 1) {
        Nan::MaybeLocal<Value> maybeIceCandidate = info[0];
        if (!maybeIceCandidate.ToLocalChecked()->IsObject()) {
            Nan::ThrowTypeError("sdpMid or sdpMLineIndex should be specified.");
            return;
        }
        Local<v8::Object> iceCandidate = maybeIceCandidate.ToLocalChecked()->ToObject();
        // sdpMid.
        Nan::Utf8String sdpMid(Nan::Get(iceCandidate, Nan::New("sdpMid").ToLocalChecked()).ToLocalChecked()->ToString());
        obj->m_sdpMid = std::string(*sdpMid);
        // candidate.
        Nan::Utf8String candidate(Nan::Get(iceCandidate, Nan::New("candidate").ToLocalChecked()).ToLocalChecked());
        obj->m_candidate = std::move(obj->deserialize(std::string(*candidate, candidate.length())));
    }
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Object> RTCIceCandidate::newInstance(const erizo::CandidateInfo& candidate)
{
    Local<Object> candidateObject = Nan::NewInstance(Nan::New(RTCIceCandidate::s_constructor)).ToLocalChecked();
    auto obj = Nan::ObjectWrap::Unwrap<RTCIceCandidate>(candidateObject);
    std::unique_ptr<erizo::CandidateInfo> candidatePtr(new erizo::CandidateInfo(candidate));
    obj->m_candidate = std::move(candidatePtr);
    obj->m_sdpMid = "";
    return candidateObject;
}

NAN_GETTER(RTCIceCandidate::candidateGetter)
{
    auto obj = Nan::ObjectWrap::Unwrap<RTCIceCandidate>(info.Holder());
    info.GetReturnValue().Set(Nan::New(obj->toString()).ToLocalChecked());
}

NAN_GETTER(RTCIceCandidate::sdpMidGetter)
{
    auto obj = Nan::ObjectWrap::Unwrap<RTCIceCandidate>(info.Holder());
    info.GetReturnValue().Set(Nan::New(obj->m_sdpMid).ToLocalChecked());
}

NAN_GETTER(RTCIceCandidate::sdpMLineIndexGetter)
{
    auto obj = Nan::ObjectWrap::Unwrap<RTCIceCandidate>(info.Holder());
    info.GetReturnValue().Set(Nan::New("0").ToLocalChecked());
}

std::string RTCIceCandidate::toString() const
{
    // Borrowed from Licode, SdpInfo::stringifyCandidate.
    std::string generation = " generation ";
    std::string hostType;
    std::ostringstream sdp;
    if (m_candidate == nullptr) {
        return "nullptr";
    }
    switch (m_candidate->hostType) {
    case erizo::HOST:
        hostType = "host";
        break;
    case erizo::SRFLX:
        hostType = "srflx";
        break;
    case erizo::PRFLX:
        hostType = "prflx";
        break;
    case erizo::RELAY:
        hostType = "relay";
        break;
    default:
        hostType = "host";
        break;
    }
    sdp << m_candidate->foundation << " " << m_candidate->componentId
        << " " << m_candidate->netProtocol << " " << m_candidate->priority << " "
        << m_candidate->hostAddress << " " << m_candidate->hostPort << " typ "
        << hostType;

    if (m_candidate->hostType == erizo::SRFLX || m_candidate->hostType == erizo::RELAY) {
        // raddr 192.168.0.12 rport 50483
        sdp << " raddr " << m_candidate->rAddress << " rport " << m_candidate->rPort;
    }

    if (m_candidate->generation >= 0) {
        sdp << generation << m_candidate->generation;
    }

    return sdp.str();
}

std::vector<std::string> RTCIceCandidate::split(const std::string& str) const
{
    std::istringstream ss{ str };
    return std::vector<std::string>(std::istream_iterator<std::string>{ ss }, std::istream_iterator<std::string>{});
}

std::unique_ptr<erizo::CandidateInfo> RTCIceCandidate::deserialize(const std::string& candidate) const
{
    ELOG_DEBUG("deserialize: %s", candidate.c_str());
    // Based on Licode SdpInfo::processCandidate.
    std::unique_ptr<erizo::CandidateInfo> cand(new erizo::CandidateInfo);
    auto pieces(split(candidate));
    static const char* types_str[] = { "host", "srflx", "prflx", "relay" };
    cand->foundation = pieces[0];
    cand->componentId = (unsigned int)strtoul(pieces[1].c_str(), nullptr, 10);

    ELOG_DEBUG("%s %u", cand->foundation.c_str(), cand->componentId);
    cand->netProtocol = pieces[2];
    // Libnice does not support tcp candidates, we ignore them.
    if (cand->netProtocol.compare("UDP") && cand->netProtocol.compare("udp")) {
        ELOG_ERROR("Libnice does not support TCP candidates. %s", pieces[2].c_str());
        return nullptr;
    }
    // a=candidate:0 1 udp 2130706432 1383 52314 typ host  generation 0
    //             0 1 2    3           4     5   6   7    8          9
    //
    // a=candidate:1367696781 1 udp 33562367 138. 49462 typ relay raddr 138.4 rport 53531 generation 0
    cand->priority = (unsigned int)strtoul(pieces[3].c_str(), nullptr, 10);
    cand->hostAddress = pieces[4];
    cand->hostPort = (unsigned int)strtoul(pieces[5].c_str(), nullptr, 10);
    if (pieces[6] != "typ") {
        ELOG_ERROR("Incorrect type.");
        return nullptr;
    }
    unsigned int type = 1111;
    int p;
    for (p = 0; p < 4; p++) {
        if (pieces[7] == types_str[p]) {
            type = p;
        }
    }
    switch (type) {
    case 0:
        cand->hostType = erizo::HOST;
        break;
    case 1:
        cand->hostType = erizo::SRFLX;
        break;
    case 2:
        cand->hostType = erizo::PRFLX;
        break;
    case 3:
        cand->hostType = erizo::RELAY;
        break;
    default:
        cand->hostType = erizo::HOST;
        break;
    }

    if (cand->hostType == erizo::SRFLX || cand->hostType == erizo::RELAY) {
        cand->rAddress = pieces[9];
        cand->rPort = (unsigned int)strtoul(pieces[11].c_str(), nullptr, 10);
    }

    if (cand->hostType == erizo::SRFLX || cand->hostType == erizo::RELAY || cand->hostType == erizo::PRFLX) {
        if (pieces.size() > 12) {
            cand->generation = (int)strtoul(pieces[12].c_str(), NULL, 10);
        }
    } else {
        if (pieces.size() > 8) {
            cand->generation = (int)strtoul(pieces[8].c_str(), NULL, 10);
        }
    }
    return cand;
}

erizo::CandidateInfo RTCIceCandidate::candidateInfo() const
{
    return *m_candidate.get();
}