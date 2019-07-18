// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#define HAVE_STDBOOL_H  // Dealing with libre warning

#include <rtputils.h>
#include "SipEP.h"
#include "sipua.h"

using namespace erizo;

namespace sip_gateway {

DEFINE_LOGGER(SipEP, "sip.SipEP");

SipEP::SipEP(SipEPOwner* owner)
    : m_owner(owner)
    , m_sipua(nullptr)
    , m_state(INITIALISED)
    , m_reregFailCount(0)
{
    ELOG_DEBUG("SipEP()");
}

SipEP::~SipEP()
{
    ELOG_DEBUG("~SipEP");
    if (m_sipua) {
        sipua_delete(m_sipua);
    }

}

bool SipEP::sipRegister(const std::string& sipServerAddr, const std::string& userName,
                             const std::string& password, const std::string& displayName)
{
    if (m_state == INITIALISED && !m_sipua) {
        if (sipServerAddr.length() == 0 || userName.length() == 0) {
            ELOG_WARN("!!User info incorect, Create sipua failed!\n");
            return false;
        }

        if (!sipua_new(&m_sipua, this, sipServerAddr.c_str(), userName.c_str(), password.c_str(), displayName.c_str())) {
            ELOG_WARN("!!Create sipua OK!\n");
            m_state = REGISTERING;
            return true;
        } else {
            ELOG_WARN("!!Create sipua failed!\n");
            m_sipua = NULL;
            return false;
        }
    } else
        return false;
}

bool SipEP::makeCall(const std::string& calleeURI, bool requireAudio, bool requireVideo)
{
    if (m_state != REGISTERED) {
        ELOG_WARN("!!m_state NOT idle, makeCall failed!\n");
        return false;
    }

    if (calleeURI.length() == 0) {
        ELOG_WARN("!!PeerURI is empty, makeCall failed!\n");
        return false;
    }

    ELOG_DEBUG("Making a call to %s", calleeURI.c_str());
    sipua_call(m_sipua, SIPUA_BOOL(requireAudio), SIPUA_BOOL(requireVideo), calleeURI.c_str());
    return true;
}

void SipEP::hangup(const std::string& peer, void *call)
{
    if (m_state == REGISTERED) {
        sipua_hangup(m_sipua, call);
        m_owner->onCallClosed(peer, "hangup");
    }
}

bool SipEP::accept(const std::string& peer)
{
    ELOG_DEBUG("Web accepted");
    if (m_state == REGISTERED ) {
        sipua_accept(m_sipua, peer.c_str());
    }
    return true;
}

void SipEP::reject(const std::string& peer, void *call)
{
    if (m_state == REGISTERED) {
        sipua_hangup(m_sipua, call);
        m_owner->onCallClosed(peer, "reject");
    }
    ELOG_DEBUG("Manually Rejected!");
}

void SipEP::helpSetCallOwner(void *call, void *owner)
{
   sipua_set_call_owner(m_sipua, call, owner);
}

void SipEP::resetCallOwner(void *call)
{
   sipua_set_call_owner(m_sipua, call, NULL);
}

void SipEP::onRegisterResult(bool successful)
{
    if (m_state == REGISTERING) {
        if (successful) {
            ELOG_DEBUG("Register OK");

            m_state = REGISTERED;
            m_reregFailCount = 0;
        } else {
            ELOG_WARN("Register Failed");
        }
        m_owner->onRegisterResult(successful);
    }
    /*
    } else {
        if (!successful) {
            ELOG_WARN("Re-register Failed");
            if (m_state == REGISTERING) {
                m_reregFailCount += 1;
                if (m_reregFailCount > 3) {
                    sipua_hangup(m_sipua, NULL);
                    // TODO terminate all the call
                    // m_owner->onCallClosed("Re-Register Failed.");
                    m_state = REGISTERING;
                }
            }
        } else {
            m_reregFailCount = 0;
        }
    }
    */
}

bool SipEP::onSipIncomingCall(bool requireAudio, bool requireVideo, const std::string& callerIdentity)
{
    ELOG_DEBUG("An incomming call from %s", callerIdentity.c_str());
    if (m_state != REGISTERED)
        return false;
    if (m_owner->onSipIncomingCall(requireAudio, requireVideo, callerIdentity)) {
        return true;
    }

    return false;
}

void SipEP::onPeerRinging(const std::string &peer)
{
    ELOG_DEBUG("Peer ringing");
    m_owner->onPeerRinging(peer);
}

void SipEP::onCallEstablished(const std::string& peer, void *call, const char *audioDir, const char *videoDir)
{
    ELOG_DEBUG("Call established");
    m_owner->onCallEstablished(peer, call, audioDir, videoDir);
}

void SipEP::onCallUpdated(const std::string &peer, const char *audioDir, const char *videoDir)
{
    ELOG_DEBUG("Call updated.");
    m_owner->onCallUpdated(peer, audioDir, videoDir);
}

void SipEP::onCallClosed(const std::string& peer, const std::string& reason)
{
    ELOG_DEBUG("Call closed: %s", reason.c_str());
    m_owner->onCallClosed(peer, reason);
}

void SipEP::onCallLoss(const std::string& peer, const std::string& reason, void *call)
{
    ELOG_DEBUG("Call loss: %s", reason.c_str());
    sipua_hangup(m_sipua, call);
    m_owner->onCallClosed(peer, reason);
}


void SipEP::onSipAudioFmt(const std::string& peer, const std::string& codecName, unsigned int sampleRate) {
   m_owner->onSipAudioFmt(peer, codecName, sampleRate);
}
void SipEP::onSipVideoFmt(const std::string& peer, const std::string& codecName, unsigned int rtpClock, const std::string& fmtp) {
   m_owner->onSipVideoFmt(peer, codecName, rtpClock, fmtp);
}

}// end of namespace sip_gateway.


extern "C" {
void ep_register_result(void* gateway, sipua_bool successful)
{
    sip_gateway::SipEP* obj = static_cast<sip_gateway::SipEP*>(gateway);
    obj->onRegisterResult(NATURAL_BOOL(successful));
}

int ep_incoming_call(void* gateway, sipua_bool audio, sipua_bool video, const char* callerURI)
{
    sip_gateway::SipEP* obj = static_cast<sip_gateway::SipEP*>(gateway);
    if (obj->onSipIncomingCall(NATURAL_BOOL(audio), NATURAL_BOOL(video), callerURI)) {
        return 0;
    } else {
        return -1;
    }
}


void ep_peer_ringing(void* gateway, const char *peer)
{
    sip_gateway::SipEP* obj = static_cast<sip_gateway::SipEP*>(gateway);
    obj->onPeerRinging(peer);
}

void ep_call_closed(void* gateway, const char *peer, const char* reason)
{
    sip_gateway::SipEP* obj = static_cast<sip_gateway::SipEP*>(gateway);
    (void)reason;
    obj->onCallClosed(peer, reason);
}

void ep_call_loss(void* gateway, const char *peer, const char* reason, void* call)
{
    sip_gateway::SipEP* obj = static_cast<sip_gateway::SipEP*>(gateway);
    (void)reason;
    obj->onCallLoss(peer, reason, call);
}

void ep_call_established(void* gateway, const char *peer, void *call, const char *audio_dir, const char *video_dir)
{
    sip_gateway::SipEP* obj = static_cast<sip_gateway::SipEP*>(gateway);
    //TODO get the peer id
    obj->onCallEstablished(peer, call, audio_dir, video_dir);
}

void ep_call_updated(void* gateway, const char *peer, const char *audio_dir, const char *video_dir)
{
    sip_gateway::SipEP* obj = static_cast<sip_gateway::SipEP*>(gateway);
    obj->onCallUpdated(peer, audio_dir, video_dir);
}

void ep_update_audio_params(void* gateway, const char* peer, const char* cdcname, int srate, int ch, const char* fmtp)
{

    sip_gateway::SipEP* obj = static_cast<sip_gateway::SipEP*>(gateway);
    (void)ch;
    (void)fmtp;
    obj->onSipAudioFmt(peer, cdcname, static_cast<int>(srate));
}

void ep_update_video_params(void* gateway, const char* peer, const char* cdcname, int bitrate, int packetsize, int fps, const char* fmtp)
{
    sip_gateway::SipEP* obj = static_cast<sip_gateway::SipEP*>(gateway);
    (void)bitrate;
    (void)packetsize;
    (void)fps;
    obj->onSipVideoFmt(peer, cdcname, 90000, fmtp ? fmtp : "");
}

} /*extern "C"*/

