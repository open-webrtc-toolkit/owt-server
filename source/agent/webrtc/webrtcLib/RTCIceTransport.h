/*
 * Copyright (C) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_RTCICETRANSPORT_H_
#define WEBRTC_RTCICETRANSPORT_H_

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <IceConnection.h>
#include <nan.h>

#include "RTCIceCandidate.h"

enum class RTCIceTransportEventType : int {
    IceCandidate = 1,
    StateChange
};

enum class RTCIceTransportState : int {
    New = 1,
    Checking,
    Connected,
    Completed,
    Disconnected,
    Failed,
    Closed
};

// https://w3c.github.io/webrtc-pc/#dom-rtciceparameters
struct RTCIceParameters {
    std::string usernameFragment;
    std::string password;
};

// Node.js addon of IceTransport Extensions for WebRTC.
// https://w3c.github.io/webrtc-ice/
class RTCIceTransport : public erizo::IceConnectionListener,
                        public Nan::ObjectWrap {
    DECLARE_LOGGER();

public:
    static NAN_MODULE_INIT(Init);

protected:
    // Defined in erizo::IceConnectionListener.
    virtual void onPacketReceived(erizo::packetPtr packet) override;
    virtual void onCandidate(const erizo::CandidateInfo& candidate, erizo::IceConnection* conn) override;
    virtual void updateIceState(erizo::IceState state, erizo::IceConnection* conn) override;

    virtual void notifyEvent(RTCIceTransportEventType type, v8::Local<v8::Value> value);

private:
    RTCIceTransport();
    ~RTCIceTransport();

    static NAN_METHOD(newInstance);
    static NAN_METHOD(gather);
    static NAN_METHOD(start);
    static NAN_METHOD(stop);
    static NAN_METHOD(getLocalCandidates);
    static NAN_METHOD(getLocalParameters);
    static NAN_METHOD(addRemoteCandidate);

    static NAUV_WORK_CB(eventsCallback);

    void drainPendingRemoteCandidates();

    static Nan::Persistent<v8::Function> constructor;

    std::unique_ptr<erizo::IceConnection> m_iceConnection;
    std::unique_ptr<RTCIceParameters> m_localParameters;
    std::unique_ptr<RTCIceParameters> m_remoteParameters;
    std::mutex m_candidateMutex;
    std::vector<erizo::CandidateInfo> m_localCandidates;
    std::vector<erizo::CandidateInfo> m_pendingRemoteCandidates; // Receive remote candidates before having remote parameters.
    uv_async_t m_async;
    std::mutex m_eventMutex;
    std::queue<std::pair<RTCIceTransportEventType, Nan::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>>>> m_events;
    RTCIceTransportState m_state;
};

#endif