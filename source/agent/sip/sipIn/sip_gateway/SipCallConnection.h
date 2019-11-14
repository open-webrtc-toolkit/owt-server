// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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
    int deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet) override;
    int deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet) override;
    int deliverEvent_(erizo::MediaEventPtr event) override { return 0; };
    // int deliverAudioData(char*, int len);
    // int deliverVideoData(char*, int len);

    /**
     * Implements MediaSource interfaces
     */
    void close() override;

    int sendFirPacket();

    /*
     * Implements FeedbackSink interface
    */
    int deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet);
    int sendPLI();
    //int deliverFeedback(char*, int len);

    void onSipFIR();
    void onSipGNACK(uint32_t ssrcPacket, uint32_t ssrcMedia, uint32_t n, uint32_t* pid_blp);

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
