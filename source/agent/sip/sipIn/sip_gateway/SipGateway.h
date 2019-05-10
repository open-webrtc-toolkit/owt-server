// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SipGateway_h
#define SipGateway_h

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>
#include <vector>

#include "EventRegistry.h"
#include "SipEP.h"


namespace sip_gateway {
struct call;

typedef enum {
    SIGNALLING = 0,
    SPEAKING,
} UAState;


struct CallInfo {
    std::string peerURI;
    bool        requireAudio;
    bool        requireVideo;
    std::string audioCodec;
    int         audioSampleRate;
    std::string videoCodec;
    int         videoRtpClock;
    std::string videoResolution;
    void        *sipCall;
    CallInfo(const std::string &peer, bool audio, bool video) :
        peerURI(peer),
        requireAudio(audio),
        requireVideo(video),
        sipCall(NULL) {
    }
};

class SipGateway : public SipEPOwner {
    DECLARE_LOGGER();

public:
    SipGateway();
    virtual ~SipGateway();

    /**
     * Implements Addon(SipGateway.cc) interfaces
     */
    bool sipRegister(const std::string& sipServerAddr, const std::string& userName,
                               const std::string& password, const std::string& displayName);
    bool makeCall(const std::string& calleeURI, bool requireAudio, bool requireVideo);
    void hangup(const std::string& peer);
    bool accept(const std::string& peer);
    void reject(const std::string& peer);
    void helpSetCallOwner(void *call, void *owner) const;
    void resetCallOwner(void *call) const;

    void setEventRegistry(EventRegistry*);
    const CallInfo* getCallInfoByPeerURI(const std::string& peer);
    /**
     * Implements SipEPOwner interfaces
     */
    void onRegisterResult(bool successful);
    bool onSipIncomingCall(bool requireAudio, bool requireVideo, const std::string& callerIdentity);
    void onPeerRinging(const std::string& peerURI);
    void onCallEstablished(const std::string& peerURI, void *call, const char *audioDir, const char *videoDir);
    void onCallClosed(const std::string& peer, const std::string& reason);
    void onCallUpdated(const std::string& peerURI, const char *audioDir, const char *videoDir);

    void onSipAudioFmt(const std::string& peer, const std::string& codecName, unsigned int sampleRate);
    void onSipVideoFmt(const std::string& peer, const std::string& codecName, unsigned int rtpClock, const std::string& ftmp);


private:
    void refreshVideoStream();
    bool terminateCall(const std::string &peer);
    void insertCallInfoByPeerURI(const std::string& uri, const bool audio, const bool video);
    void insertOrUpdateCallInfoByPeerURI(const std::string& uri, void *call, bool audio, bool video);
    void insertOrUpdateCallInfoByPeerURI(const std::string& uri, const std::string& aCodec, unsigned int sampleRate);
    void insertOrUpdateCallInfoByPeerURI(const std::string& uri, const std::string& vCodec, unsigned int rtpClock, const std::string& fmtp);

    boost::scoped_ptr<SipEP> m_sipEP;
    boost::shared_mutex m_mutex;
    std::map<std::string, CallInfo> m_call_map;
    // MediaInfo m_sipMediaInfo;

    // libuv - uv_async_send() to notify node thread
    void notifyAsyncEvent(const std::string& event, const std::string& data);
    void notifyAsyncEvent(const std::string& event, uint32_t data);
    EventRegistry* m_asyncHandle;
};

}

#endif /* SipGateway_h */
