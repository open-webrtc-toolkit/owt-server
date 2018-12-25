/*
 * Copyright (C) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <map>

#include "RTCIceTransport.h"
#include <LibNiceConnection.h>

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Value;

Nan::Persistent<Function> RTCIceTransport::constructor;

DEFINE_LOGGER(RTCIceTransport, "RTCIceTransport");

// Using unordered_map after upgrading C++ compiler to a later version.
// See http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#2148
static std::map<const RTCIceTransportEventType, const std::string> s_eventTypeNameMap = {
    { RTCIceTransportEventType::IceCandidate, "icecandidate" },
    { RTCIceTransportEventType::StateChange, "statechange" }
};

RTCIceTransport::RTCIceTransport()
    : m_localCandidates(std::vector<erizo::CandidateInfo>())
    , m_pendingRemoteCandidates(std::vector<erizo::CandidateInfo>())
{
}

RTCIceTransport::~RTCIceTransport() {}

NAN_MODULE_INIT(RTCIceTransport::Init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("RTCIceTransport").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "gather", gather);
    Nan::SetPrototypeMethod(tpl, "start", start);
    Nan::SetPrototypeMethod(tpl, "stop", stop);
    Nan::SetPrototypeMethod(tpl, "getLocalCandidates", getLocalCandidates);
    Nan::SetPrototypeMethod(tpl, "getLocalParameters", getLocalParameters);
    Nan::SetPrototypeMethod(tpl, "addRemoteCandidate", addRemoteCandidate);

    constructor.Reset(tpl->GetFunction());
    Nan::Set(target, Nan::New("RTCIceTransport").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(RTCIceTransport::newInstance)
{
    if (!info.IsConstructCall()) {
        return;
    }
    erizo::IceConfig config;
    config.ice_components = 1;
    RTCIceTransport* obj = new RTCIceTransport();
    obj->m_iceConnection.reset(erizo::LibNiceConnection::create(obj, config));
    uv_async_init(uv_default_loop(), &obj->m_async, &RTCIceTransport::eventsCallback);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(RTCIceTransport::gather)
{
    RTCIceTransport* obj = Nan::ObjectWrap::Unwrap<RTCIceTransport>(info.Holder());
    obj->m_iceConnection->start();
    ELOG_DEBUG("Start gather ICE candidates.");
}

NAN_METHOD(RTCIceTransport::start)
{
    RTCIceTransport* obj = Nan::ObjectWrap::Unwrap<RTCIceTransport>(info.Holder());
    if (obj->m_state == RTCIceTransportState::Closed) {
        return Nan::ThrowError("Invalid state.");
    }
    if (info.Length() != 1 && info.Length() != 2) {
        return Nan::ThrowTypeError("remoteParameters is mandatory.");
    }
    if (info.Length() == 2) {
        ELOG_WARN("Setting ICE role is not supported. It always performs as controlling mode.");
    }
    Nan::MaybeLocal<Value> maybeIceParameters = info[0];
    if (!maybeIceParameters.ToLocalChecked()->IsObject()) {
        return Nan::ThrowTypeError("usernameFragment and password must be set.");
    }
    Local<v8::Object> iceParametersObject = maybeIceParameters.ToLocalChecked()->ToObject();
    obj->m_remoteParameters.reset(new RTCIceParameters());
    // usernameFragment.
    Nan::Utf8String usernameFragment(Nan::Get(iceParametersObject, Nan::New("usernameFragment").ToLocalChecked()).ToLocalChecked()->ToString());
    obj->m_remoteParameters->usernameFragment = std::string(*usernameFragment);
    // password.
    Nan::Utf8String password(Nan::Get(iceParametersObject, Nan::New("password").ToLocalChecked()).ToLocalChecked());
    obj->m_remoteParameters->password = std::string(*password);
    obj->m_iceConnection->setRemoteCredentials(obj->m_remoteParameters->usernameFragment, obj->m_remoteParameters->password);

    obj->drainPendingRemoteCandidates();
    ELOG_DEBUG("Start an RTCIceTransport.");
}

NAN_METHOD(RTCIceTransport::stop)
{
    RTCIceTransport* obj = Nan::ObjectWrap::Unwrap<RTCIceTransport>(info.Holder());
    if (obj->m_state != RTCIceTransportState::Closed) {
        // TODO: Create multiple RTCIceTransport and stop them at the same time leads crash.
        // obj->m_iceConnection->close();
    }
    ELOG_DEBUG("Stop an RTCIceTransport.");
    std::lock_guard<std::mutex> lock(obj->m_eventMutex);
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&obj->m_async))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&obj->m_async), NULL);
    }
}

NAN_METHOD(RTCIceTransport::getLocalCandidates)
{
    RTCIceTransport* obj = Nan::ObjectWrap::Unwrap<RTCIceTransport>(info.Holder());
    Local<v8::Array> candidates = Nan::New<v8::Array>(obj->m_localCandidates.size());
    ELOG_DEBUG("Candidate size: %d.", candidates->Length());
    for (unsigned int i = 0; i < obj->m_localCandidates.size(); i++) {
        Nan::Set(candidates, i, RTCIceCandidate::newInstance(obj->m_localCandidates[i]));
    }
    info.GetReturnValue().Set(candidates);
}

NAN_METHOD(RTCIceTransport::getLocalParameters)
{
    RTCIceTransport* obj = Nan::ObjectWrap::Unwrap<RTCIceTransport>(info.Holder());
    if (obj->m_localParameters == nullptr) {
        obj->m_localParameters.reset(new RTCIceParameters());
        obj->m_localParameters->usernameFragment = obj->m_iceConnection->getLocalUsername();
        obj->m_localParameters->password = obj->m_iceConnection->getLocalPassword();
    }
    v8::Local<v8::Object> parameters = Nan::New<v8::Object>();
    Nan::Set(parameters, Nan::New("usernameFragment").ToLocalChecked(), Nan::New(obj->m_localParameters->usernameFragment).ToLocalChecked());
    Nan::Set(parameters, Nan::New("password").ToLocalChecked(), Nan::New(obj->m_localParameters->password).ToLocalChecked());
    info.GetReturnValue().Set(parameters);
}

NAN_METHOD(RTCIceTransport::addRemoteCandidate)
{
    RTCIceTransport* obj = Nan::ObjectWrap::Unwrap<RTCIceTransport>(info.Holder());
    RTCIceCandidate* candidate = Nan::ObjectWrap::Unwrap<RTCIceCandidate>(info[0]->ToObject());
    if (obj->m_remoteParameters == nullptr) {
        std::lock_guard<std::mutex> lock(obj->m_candidateMutex);
        obj->m_pendingRemoteCandidates.push_back({ candidate->candidateInfo() });
    } else {
        ELOG_DEBUG("Adding remote candidate: %s.", candidate->toString().c_str());
        erizo::CandidateInfo candidateInfo = candidate->candidateInfo();
        candidateInfo.username = obj->m_remoteParameters->usernameFragment;
        candidateInfo.password = obj->m_remoteParameters->password;
        candidateInfo.componentId = 1;
        obj->m_iceConnection->setRemoteCandidates({ candidateInfo }, false);
    }
}

NAUV_WORK_CB(RTCIceTransport::eventsCallback)
{
    Nan::HandleScope scope;
    RTCIceTransport* obj = reinterpret_cast<RTCIceTransport*>(async->data);
    if (!obj || obj->m_iceConnection == NULL)
        return;
    std::lock_guard<std::mutex> lock(obj->m_eventMutex);
    while (!obj->m_events.empty()) {
        Nan::MaybeLocal<v8::Value> onEvent = Nan::Get(obj->handle(), Nan::New<v8::String>("on" + s_eventTypeNameMap[obj->m_events.front().first]).ToLocalChecked()).ToLocalChecked();
        if (!onEvent.IsEmpty()) {
            v8::Local<v8::Value> onEventLocal = onEvent.ToLocalChecked();
            if (onEventLocal->IsFunction()) {
                v8::Local<v8::Function> eventCallback = onEventLocal.As<Function>();
                Nan::AsyncResource* resource = new Nan::AsyncResource(Nan::New<v8::String>("EventCallback").ToLocalChecked());
                Local<Value> args[] = { Nan::New(obj->m_events.front().second) };
                resource->runInAsyncScope(Nan::GetCurrentContext()->Global(), eventCallback, 1, args);
            }
        }
        obj->m_events.pop();
    }
}

void RTCIceTransport::notifyEvent(RTCIceTransportEventType type, v8::Local<Value> value)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    std::pair<RTCIceTransportEventType, Nan::Persistent<Value, v8::CopyablePersistentTraits<v8::Value>>> p(type, value);
    this->m_events.push(p);
    m_async.data = this;
    uv_async_send(&m_async);
}

void RTCIceTransport::onPacketReceived(erizo::packetPtr packet)
{
}

void RTCIceTransport::onCandidate(const erizo::CandidateInfo& candidate, erizo::IceConnection* conn)
{
    m_localCandidates.push_back(candidate);
    ELOG_DEBUG("Adding new local candidates.");
    v8::Local<v8::Object> iceEvent = Nan::New<v8::Object>();
    Nan::Set(iceEvent, Nan::New("candidate").ToLocalChecked(), RTCIceCandidate::newInstance(candidate));
    notifyEvent(RTCIceTransportEventType::IceCandidate, iceEvent);
}

void RTCIceTransport::updateIceState(erizo::IceState state, erizo::IceConnection* conn)
{
    //notifyEvent(RTCIceTransportEventType::StateChange, Nan::Undefined());
}

void RTCIceTransport::drainPendingRemoteCandidates()
{
    std::lock_guard<std::mutex> lock(m_candidateMutex);
    for (auto& candidate : m_pendingRemoteCandidates) {
        candidate.username = m_remoteParameters->usernameFragment;
        candidate.password = m_remoteParameters->password;
        candidate.componentId = 1;
    }
    m_iceConnection->setRemoteCandidates(m_pendingRemoteCandidates, false);
    m_pendingRemoteCandidates.clear();
}