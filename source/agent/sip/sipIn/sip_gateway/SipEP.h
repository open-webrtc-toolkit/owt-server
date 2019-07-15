// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SipEP_h
#define SipEP_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>

#include "MediaDefinitions.h"

struct sipua_entity;

namespace sip_gateway {

class SipEPOwner {
public:
    virtual void onRegisterResult(bool successful) = 0;
    virtual bool onSipIncomingCall(bool requireAudio, bool requireVideo, const std::string& callerIdentity) = 0;
    virtual void onPeerRinging(const std::string& peerURI) = 0;
    virtual void onCallEstablished(const std::string& peerURI, void *call, const char *audioDir, const char *videoDir) = 0;
    virtual void onCallClosed(const std::string& peer, const std::string& reason)= 0;
    virtual void onCallUpdated(const std::string& peerURI, const char *audioDir, const char *videoDir) = 0;
    virtual void onSipAudioFmt(const std::string& peer, const std::string& codecName, unsigned int sampleRate) = 0;
    virtual void onSipVideoFmt(const std::string& peer, const std::string& codecName, unsigned int rtpClock, const std::string& fmtp) = 0;

};


class SipEP  {
    DECLARE_LOGGER();
    typedef enum {INITIALISED = 1,
                  REGISTERING,
                  REGISTERED } UAState;

public:
    SipEP(SipEPOwner* owner);
    virtual ~SipEP();

    bool sipRegister(const std::string& sipServerAddr, const std::string& userName,
                     const std::string& password, const std::string& displayName);
    bool makeCall(const std::string& calleeURI, bool requireAudio, bool requireVideo);
    void hangup(const std::string& peer, void *call);
    bool accept(const std::string& peer);
    void reject(const std::string& peer, void *call);
    void helpSetCallOwner(void *call, void* owner);
    void resetCallOwner(void *call);

    void onRegisterResult(bool successful);
    void onPeerRinging(const std::string &peer);
    void onCallEstablished(const std::string& peer, void *call, const char *audioDir, const char *videoDir);
    void onCallClosed(const std::string& peer, const std::string& reason);
    void onCallLoss(const std::string& peer, const std::string& reason, void *call);
    void onCallUpdated(const std::string& peer, const char *audioDir, const char *videoDir);
    bool onSipIncomingCall(bool requireAudio, bool requireVideo, const std::string& callerIdentity);
    void onSipAudioFmt(const std::string& peer, const std::string& codecName, unsigned int sampleRate);
    void onSipVideoFmt(const std::string& peer, const std::string& codecName, unsigned int rtpClock, const std::string& fmtp);
private:
    void terminateCall(const std::string& peer);

    SipEPOwner* m_owner; // the js gateway
    struct sipua_entity* m_sipua;

    UAState m_state;
    int m_reregFailCount;
}/*class SipEP*/;

}/*namespace sip_gateway*/

#endif
