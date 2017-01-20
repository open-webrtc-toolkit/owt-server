/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#ifndef SipCallConnection_h
#define SipCallConnection_h

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>

#include "EventRegistry.h"
#include "MediaDefinitions.h"


namespace sip_gateway {

class SipGateway;

class SipCallConnection : public erizo::MediaSink,
                          public erizo::MediaSource,
                          public erizo::FeedbackSource,
                          public erizo::FeedbackSink {
    DECLARE_LOGGER();

public:
    SipCallConnection(SipGateway *gateway, const std::string& peerUri);
    virtual ~SipCallConnection();


    /**
     * Implements MediaSink interfaces
     */
    int deliverAudioData(char*, int len);
    int deliverVideoData(char*, int len);

    /**
     * Implements MediaSource interfaces
     */
    int sendFirPacket();

    /*
     * Implements FeedbackSink interface
    */
    int deliverFeedback(char*, int len);

    void onSipFIR();

    void onSipAudioData(char* data, int len);
    void onSipVideoData(char* data, int len);
    void onConnectionClosed();

    void refreshVideoStream();

private:

    void *m_sipCall;
    bool running;
    const SipGateway *m_gateway;
    boost::shared_mutex m_mutex;
    std::string m_peerURI;
    std::string m_audioCodec;
    unsigned int m_audioSampleRate;
    std::string m_videoCodec;
    unsigned int m_videoRtpClock;
    int sequenceNumberFIR_;

};
}
#endif /* SipCallConnection_h */
