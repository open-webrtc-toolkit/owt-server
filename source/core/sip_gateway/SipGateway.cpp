/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

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
        CallInfo info("sip:" + calleeURI, requireAudio,requireVideo);
        ELOG_DEBUG("makeCall CallInfo: %s", info.peerURI.c_str());
        boost::shared_lock<boost::shared_mutex> lock(m_mutex);
        m_call_vector.push_back(info);
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
        CallInfo info(peer, true, true);
        boost::shared_lock<boost::shared_mutex> lock(m_mutex);
        m_call_vector.push_back(info);
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
void SipGateway::onCallEstablished(const std::string& peerURI, void *call, bool video)
{
    const CallInfo *info = getCallInfoByPeerURI(peerURI);
    std::string str = "{\"peerURI\":\"" + peerURI + "\"," +
                       "\"audio\":true,\"audio_codec\":\"" + info->audioCodec + "\"," +
                       "\"video\":" + ((video) ? ("true, \"video_codec\":\"" + info->videoCodec + "\"," +
                                                        "\"videoResolution\": \"" + info->videoResolution + "\"")
                                                : "false") + "}";
    if (updateCallInfoByPeerURI(peerURI, call, true, video)) {
        notifyAsyncEvent("CallEstablished", str.c_str());
        refreshVideoStream();
    } else {
        ELOG_ERROR("can not establish call: %s", peerURI.c_str());
    }
}

// The sipua thread
void SipGateway::onCallUpdated(const std::string& peerURI, bool video)
{
    const CallInfo *info = getCallInfoByPeerURI(peerURI);
    std::string str = "{\"peerURI\":\"" + peerURI + "\"," +
                       "\"audio\":true,\"audio_codec\":\"" + info->audioCodec + "\"," +
                       "\"video\":" + ((video) ? ("true, \"video_codec\":\"" + info->videoCodec + "\"," +
                                                        "\"videoResolution\": \"" + info->videoResolution + "\"")
                                                : "false") + "}";
    if (updateCallInfoByPeerURI(peerURI, info->sipCall, true, video)) {
        notifyAsyncEvent("CallUpdated", str.c_str());
        refreshVideoStream();
    } else {
        ELOG_ERROR("can not update call: %s", peerURI.c_str());
    }
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
        if (!updateCallInfoByPeerURI(peer, codecName, sampleRate)) {
            ELOG_ERROR("signal error with set audio fmt");
        }
    } else {
      ELOG_ERROR("not support audio fmt");
    }
}

void SipGateway::onSipVideoFmt(const std::string &peer, const std::string& codecName, unsigned int rtpClock, const std::string& fmtp)
{
    ELOG_DEBUG("onSipVideoFmt:%s-%u[%s]", codecName.c_str(), rtpClock, fmtp.c_str());
    if ((codecName == "VP8" && rtpClock == 90000)
    || (codecName == "H264" && rtpClock == 90000)) {
        if (!updateCallInfoByPeerURI(peer, codecName, rtpClock, fmtp)) {
            ELOG_ERROR("signal error with set video fmt");
        }
    } else {
      ELOG_ERROR("not support video fmt");
    }
}


const CallInfo* SipGateway::getCallInfoByPeerURI(const std::string& uri)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    std::vector<CallInfo>::const_iterator iter = m_call_vector.begin();
    for(; iter != m_call_vector.end(); ++iter) {
        if (uri == iter->peerURI) {
            return &(*iter);
        }
    }
    return NULL;
}

bool SipGateway::updateCallInfoByPeerURI(const std::string& uri, const std::string& vCodec, unsigned int rtpClock, const std::string& fmtp)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    std::vector<CallInfo>::iterator iter = m_call_vector.begin();
    for(; iter != m_call_vector.end(); ++iter) {
        if (uri == iter->peerURI) {
            iter->videoCodec = vCodec;
            iter->videoRtpClock = rtpClock;
            iter->videoResolution = fmtp;
            return true;
        }
    }
    return false;
}

bool SipGateway::updateCallInfoByPeerURI(const std::string& uri, const std::string& aCodec, unsigned int sampleRate)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    std::vector<CallInfo>::iterator iter = m_call_vector.begin();
    for(; iter != m_call_vector.end(); ++iter) {
        if (uri == iter->peerURI) {
            iter->audioCodec = aCodec;
            iter->audioSampleRate = sampleRate;
            return true;
        }
    }
    return false;
}

bool SipGateway::updateCallInfoByPeerURI(const std::string& uri, void *call, bool audio, bool video)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    std::vector<CallInfo>::iterator iter = m_call_vector.begin();
    for(; iter != m_call_vector.end(); ++iter) {
        if (uri == iter->peerURI) {
            iter->sipCall = call;
            iter->requireAudio = audio;
            iter->requireVideo = video;
            return true;
        }
    }
    return false;
}

void SipGateway::refreshVideoStream()
{
    ELOG_DEBUG("refreshVideoStream");
}

// The main thread or sipua thread
bool SipGateway::terminateCall(const std::string &peer)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    std::vector<CallInfo>::iterator iter = m_call_vector.begin();
    for(; iter != m_call_vector.end(); ++iter) {
        if (peer == iter->peerURI)
            m_call_vector.erase(iter);
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
