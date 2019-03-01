// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#define HAVE_STDBOOL_H  // Dealing with libre warning

#include "SipGateway.h"

#include <rtputils.h>


namespace sip_gateway {

DEFINE_LOGGER(SipGateway, "sip.SipGateway");

SipGateway::SipGateway() :
   m_asyncHandle(NULL)
{
    ELOG_DEBUG("SipGateway constructor");
    m_sipEP.reset(new SipEP(this));
}

SipGateway::~SipGateway()
{
    ELOG_DEBUG("SipGateway destructor");
    m_sipEP.reset();
}

// The main thread
bool SipGateway::sipRegister(const std::string& sipServerAddr, const std::string& userName,
                             const std::string& password, const std::string& displayName)
{
    return m_sipEP->sipRegister(sipServerAddr, userName, password, displayName);
}

// The main thread
bool SipGateway::makeCall(const std::string& calleeURI, bool requireAudio, bool requireVideo)
{
    if (m_sipEP->makeCall(calleeURI,requireAudio, requireVideo)) {
        std::string peerURI = (strncmp(calleeURI.c_str(), "sip:", 4) ? "sip:" + calleeURI : calleeURI);
        ELOG_DEBUG("makeCall CallInfo: %s", peerURI.c_str());
        insertCallInfoByPeerURI(peerURI, requireAudio, requireVideo);
        return true;
    } else {
        return false;
    }
}

// The main thread
void SipGateway::hangup(const std::string& peer)
{
    const CallInfo *call = getCallInfoByPeerURI(peer);
    if (call != NULL)
        m_sipEP->hangup(peer);
}

// The main thread
bool SipGateway::accept(const std::string& peer)
{
    if (m_sipEP->accept(peer)) {
        insertCallInfoByPeerURI(peer, true, true);
        return true;
    } else {
        notifyAsyncEvent("CallClosed", "");
        terminateCall(peer);
        return false;
    }
}

// The main thread
void SipGateway::reject(const std::string& peer)
{
    m_sipEP->reject(peer);
}

void SipGateway::helpSetCallOwner(void *call, void *owner) const
{
    m_sipEP->helpSetCallOwner(call, owner);
}
// The sipua thread
void SipGateway::onRegisterResult(bool successful)
{
    if (successful) {
        notifyAsyncEvent("SIPRegisterOK", "");
    } else {
        notifyAsyncEvent("SIPRegisterFailed", "");
    }
}

// The sipua thread
void SipGateway::onPeerRinging(const std::string& peerURI)
{
    notifyAsyncEvent("PeerRinging", peerURI.c_str());
}

// The sipua thread
void SipGateway::onCallEstablished(const std::string& peerURI, void *call,
                                   const char *audioDir, const char *videoDir)
{
    const CallInfo *info = getCallInfoByPeerURI(peerURI);
    if (!info)
        return;

    //TODO: pass red/ulpfec support from EP
    std::string str = "{\"peerURI\":\"" + peerURI + "\"," +
                       "\"audio\":" + ((audioDir!=NULL) ? ("true,\"audio_codec\":\"" + info->audioCodec + "\"," +
                                                           "\"audio_dir\": \"" + audioDir + "\"")
                                                        : "false") + "," +
                       "\"video\":" + ((videoDir!=NULL) ? ("true, \"video_codec\":\"" + info->videoCodec + "\"," +
                                                           "\"videoResolution\": \"" + info->videoResolution + "\"," +
                                                           "\"video_dir\": \"" + videoDir + "\"," + "\"support_red\":false," +
                                                           "\"support_ulpfec\":false")
                                                        : "false") + "}";
    insertOrUpdateCallInfoByPeerURI(peerURI, call, audioDir!=NULL, videoDir!=NULL);
    notifyAsyncEvent("CallEstablished", str.c_str());
    refreshVideoStream();
}

// The sipua thread
void SipGateway::onCallUpdated(const std::string& peerURI, const char *audioDir, const char *videoDir)
{
    const CallInfo *info = getCallInfoByPeerURI(peerURI);
    if (!info)
        return;

    std::string str = "{\"peerURI\":\"" + peerURI + "\"," +
                       "\"audio\":" + ((audioDir!=NULL) ? ("true,\"audio_codec\":\"" + info->audioCodec + "\"," +
                                                           "\"audio_dir\": \"" + audioDir + "\"")
                                                        : "false") + "," +
                       "\"video\":" + ((videoDir!=NULL) ? ("true, \"video_codec\":\"" + info->videoCodec + "\"," +
                                                           "\"videoResolution\": \"" + info->videoResolution + "\"," +
                                                           "\"video_dir\": \"" + videoDir + "\"," + "\"support_red\":false," +
                                                           "\"support_ulpfec\":false")
                                                        : "false") + "}";
    insertOrUpdateCallInfoByPeerURI(peerURI, info->sipCall, audioDir!=NULL, videoDir!=NULL);
    notifyAsyncEvent("CallUpdated", str.c_str());
    refreshVideoStream();
}

// The sipua thread
void SipGateway::onCallClosed(const std::string& peer, const std::string& reason)
{
    notifyAsyncEvent("CallClosed", peer.c_str());
    terminateCall(peer);
}

// The sipua thread
bool SipGateway::onSipIncomingCall(bool requireAudio, bool requireVideo, const std::string& callerIdentity)
{
    notifyAsyncEvent("IncomingCall", callerIdentity);
    return true;
}

// The sipua thread
void SipGateway::onSipAudioFmt(const std::string &peer, const std::string &codecName, unsigned int sampleRate)
{
    ELOG_DEBUG("onSipAudioFmt:%s-%s-%u", peer.c_str(), codecName.c_str(), sampleRate);
    if ((codecName == "PCMU" && sampleRate == 8000)
        || (codecName == "PCMA" && sampleRate == 8000)
        || (codecName == "opus" && sampleRate == 48000)) {
        insertOrUpdateCallInfoByPeerURI(peer, codecName, sampleRate);
    } else {
      ELOG_ERROR("not support audio fmt");
    }
}

void SipGateway::onSipVideoFmt(const std::string &peer, const std::string& codecName, unsigned int rtpClock, const std::string& fmtp)
{
    ELOG_DEBUG("onSipVideoFmt:%s-%u[%s]", codecName.c_str(), rtpClock, fmtp.c_str());
    if ((codecName == "VP8" && rtpClock == 90000)
    || (codecName == "H264" && rtpClock == 90000)
    || (codecName == "H265" && rtpClock == 90000)) {
        insertOrUpdateCallInfoByPeerURI(peer, codecName, rtpClock, fmtp);
    } else {
      ELOG_ERROR("not support video fmt");
    }
}


const CallInfo* SipGateway::getCallInfoByPeerURI(const std::string& uri)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    std::map<std::string, CallInfo>::iterator iter = m_call_map.find(uri);
    if(iter != m_call_map.end()){
        return &(iter->second);
    }
    return NULL;
}

void SipGateway::insertCallInfoByPeerURI(const std::string& uri, const bool audio, const bool video){
    ELOG_DEBUG("insertCallInfoByPeerURI %s", uri.c_str());
    boost::upgrade_lock<boost::shared_mutex> upgrade_lock(m_mutex);
    std::map<std::string, CallInfo>::iterator iter = m_call_map.find(uri);
    if(iter == m_call_map.end()){
        boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upgrade_lock);
        m_call_map.insert(std::pair<std::string, CallInfo>(uri, CallInfo(uri, audio, video)));
    }
}

void SipGateway::insertOrUpdateCallInfoByPeerURI(const std::string& uri, const std::string& vCodec, unsigned int rtpClock, const std::string& fmtp)
{
    ELOG_DEBUG("insertOrUpdateCallInfoByPeerURI %s", uri.c_str());
    boost::upgrade_lock<boost::shared_mutex> upgrade_lock(m_mutex);
    std::map<std::string, CallInfo>::iterator iter = m_call_map.find(uri);
    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upgrade_lock);
    if(iter != m_call_map.end()){
        iter->second.requireVideo = true;
        iter->second.videoCodec = vCodec;
        iter->second.videoRtpClock = rtpClock;
        iter->second.videoResolution = fmtp;
    }else{
        CallInfo newInfo(uri, false, true);
        newInfo.videoCodec = vCodec;
        newInfo.videoRtpClock = rtpClock;
        newInfo.videoResolution = fmtp;
        m_call_map.insert(std::pair<std::string, CallInfo>(uri, newInfo));
    }
}

void SipGateway::insertOrUpdateCallInfoByPeerURI(const std::string& uri, const std::string& aCodec, unsigned int sampleRate)
{
    ELOG_DEBUG("insertOrUpdateCallInfoByPeerURI %s", uri.c_str());
    boost::upgrade_lock<boost::shared_mutex> upgrade_lock(m_mutex);
    std::map<std::string, CallInfo>::iterator iter = m_call_map.find(uri);
    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upgrade_lock);
    if(iter != m_call_map.end()){
        iter->second.requireAudio = true;
        iter->second.audioCodec = aCodec;
        iter->second.audioSampleRate = sampleRate;
    }else{
        CallInfo newInfo(uri, true, false);
        newInfo.audioCodec = aCodec;
        newInfo.audioSampleRate = sampleRate;
        m_call_map.insert(std::pair<std::string, CallInfo>(uri, newInfo));
    }
}

void SipGateway::insertOrUpdateCallInfoByPeerURI(const std::string& uri, void *call, bool audio, bool video)
{
    ELOG_DEBUG("insertOrUpdateCallInfoByPeerURI %s", uri.c_str());
    boost::upgrade_lock<boost::shared_mutex> upgrade_lock(m_mutex);
    std::map<std::string, CallInfo>::iterator iter = m_call_map.find(uri);
    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upgrade_lock);
    if(iter != m_call_map.end()){
        iter->second.requireAudio = audio;
        iter->second.requireVideo = video;
        iter->second.sipCall = call;
    }else{
        CallInfo newInfo(uri, audio, video);
        newInfo.sipCall = call;
        m_call_map.insert(std::pair<std::string, CallInfo>(uri, newInfo));
    }
}

void SipGateway::refreshVideoStream()
{
    ELOG_DEBUG("refreshVideoStream");
}

// The main thread or sipua thread
bool SipGateway::terminateCall(const std::string &peer)
{
    boost::upgrade_lock<boost::shared_mutex> upgrade_lock(m_mutex);
    std::map<std::string, CallInfo>::iterator iter = m_call_map.find(peer);
    if(iter != m_call_map.end()){
        boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(upgrade_lock);
        m_call_map.erase(iter);
        return true;
    }
    return false;
}


// The main thread
void SipGateway::setEventRegistry(EventRegistry* handle)
{
    m_asyncHandle = handle;
    ELOG_DEBUG("setEventRegistry - add listener %p", handle);
}

void SipGateway::notifyAsyncEvent(const std::string& event, const std::string& data)
{
   if (m_asyncHandle) {
       m_asyncHandle->notifyAsyncEvent(event, data);
       ELOG_DEBUG("notifyAsyncEvent - event: %s", event.c_str());
    } else {
       ELOG_DEBUG("notifyAsyncEvent - handler not set");
    }

}

void SipGateway::notifyAsyncEvent(const std::string& event, uint32_t data)
{
   if (m_asyncHandle) {
       std::ostringstream message;
       message << data;
       m_asyncHandle->notifyAsyncEvent(event, message.str());
       ELOG_DEBUG("notifyAsyncEvent - event: %s", event.c_str());
   } else {
       ELOG_DEBUG("notifyAsyncEvent - handler not set");
   }
}

}// end of namespace sip_gateway.
