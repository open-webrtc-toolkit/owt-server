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
    IceCandidate,
    StateChange
};

enum class RTCIceTransportState : int {
    New,
    Checking,
    Connected,
    Completed,
    Disconnected,
    Failed,
    Closed
};

// https://w3c.github.io/webrtc-pc/#dom-rtcicerole
enum class RTCIceRole:uint8_t{
    Controlling,
    Controlled
};

// https://w3c.github.io/webrtc-pc/#dom-rtciceparameters
struct RTCIceParameters {
    std::string usernameFragment;
    std::string password;
};

// Node.js addon of IceTransport Extensions for WebRTC.
// https://w3c.github.io/webrtc-ice/
class RTCIceTransport : public Nan::ObjectWrap {
    DECLARE_LOGGER();

public:
    // RTCIceTransport could be splitted into two class for front end and backend. This kind of listener could be placed in backend.
    class QuicListener {
    public:
        virtual void onReadPacket(const char* buffer, size_t buffer_length) = 0;
        virtual void onReadyToWrite() = 0;
    };

    static NAN_MODULE_INIT(Init);
    erizo::IceConnection* iceConnection();
    void setQuicListener(QuicListener* quicListener);

    // Defined in erizo::IceConnectionListener.
    virtual void onPacketReceived(erizo::packetPtr packet);
    virtual void onCandidate(const erizo::CandidateInfo& candidate, erizo::IceConnection* conn);
    virtual void updateIceState(erizo::IceState state, erizo::IceConnection* conn);

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

    static NAN_GETTER(roleGetter);

    static NAUV_WORK_CB(onCandidateCallback);
    static NAUV_WORK_CB(onStateUpdateCallback);

    void drainPendingRemoteCandidates();

    static Nan::Persistent<v8::Function> constructor;

    std::unique_ptr<erizo::IceConnection> m_iceConnection;
    std::shared_ptr<erizo::IceConnectionListener> m_iceConnectionListener;
    std::unique_ptr<RTCIceParameters> m_localParameters;
    std::unique_ptr<RTCIceParameters> m_remoteParameters;
    std::mutex m_candidateMutex;
    std::vector<erizo::CandidateInfo> m_localCandidates;
    std::queue<erizo::CandidateInfo> m_unnotifiedLocalCandidates; // onicecandidate is not triggered for these candidates.
    std::vector<erizo::CandidateInfo> m_pendingRemoteCandidates; // Receive remote candidates before having remote parameters.
    uv_async_t m_asyncOnCandidate;
    std::mutex m_stateUpdateMutex;
    std::queue<erizo::IceState> m_unnotifiedState;
    uv_async_t m_asyncStateUpdate;
    RTCIceTransportState m_state;
    QuicListener* m_quicListener;
};

#endif