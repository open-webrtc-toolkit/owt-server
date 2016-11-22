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
    void hangup(const std::string& peer);
    bool accept(const std::string& peer);
    void reject(const std::string& peer);
    void helpSetCallOwner(void *call, void* owner);

    void onRegisterResult(bool successful);
    void onPeerRinging(const std::string &peer);
    void onCallEstablished(const std::string& peer, void *call, const char *audioDir, const char *videoDir);
    void onCallClosed(const std::string& peer, const std::string& reason);
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
