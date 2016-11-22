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
