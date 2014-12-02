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

#ifndef OoVooGateway_h
#define OoVooGateway_h

#include "OoVooInboundStreamProcessor.h"
#include "OoVooOutboundStreamProcessor.h"
#include "OoVooProtocolHeader.h"

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <EventRegistry.h>
#include <Gateway.h>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>
#include <ProtectedRTPReceiver.h>
#include <WebRTCFeedbackProcessor.h>

namespace oovoo_gateway {

class OoVooConnection;
class OoVooJobTimer;

class FeedbackTimerListener {
public:
    virtual void onProcessFeedback() = 0;
};

/**
 * Represents connection between WebRTC clients and ooVoo.
 * Receives media from the WebRTC client and retransmits it to ooVoo,
 * or receives media from ooVoo and retransmits it to the WebRTC client.
 */
class OoVooGateway : public woogeen_base::Gateway, public erizo::MediaSink, public OoVooProtocolEventsListener, public OoVooDataReceiver, public erizo::RTPDataReceiver, public FeedbackTimerListener {
    DECLARE_LOGGER();

public:
    OoVooGateway(const std::string& ooVooCustomParam);
    virtual ~OoVooGateway();

    /**
     * Implements Gateway interfaces
     */
    bool setPublisher(erizo::MediaSource*, const std::string& clientId, const std::string& videoResolution);
    bool setPublisher(erizo::MediaSource*, const std::string& clientId);
    void unsetPublisher();
    void addSubscriber(erizo::MediaSink*, const std::string& id);
    void removeSubscriber(const std::string& id);
    void setupAsyncEvent(const std::string& event, woogeen_base::EventRegistry*);
    void destroyAsyncEvents();
    bool clientJoin(const std::string& clientJoinUri);
    void customMessage(const std::string& message);
    std::string retrieveGatewayStatistics();
    void subscribeStream(const std::string& id, bool isAudio);
    void unsubscribeStream(const std::string& id, bool isAudio);
    void publishStream(bool isAudio);
    void unpublishStream(bool isAudio);
    void setAdditionalSourceConsumer(woogeen_base::MediaSourceConsumer*) { }

    /**
     * Implements MediaSink interfaces
     */
    int deliverAudioData(char*, int len);
    int deliverVideoData(char*, int len);
    bool acceptEncapsulatedRTPData();
    bool acceptFEC();
    bool acceptResentData();

    /**
     * Implements OoVooDataReceiver interfaces
     */
    void receiveOoVooData(const mediaPacket_t&);
    void receiveOoVooSyncMsg(uint64_t ntp, uint64_t rtp, bool isAudio, uint32_t streamId);

    /**
     * Implements RTPDataReceiver interfaces
     */
    void receiveRtpData(char*, int len, erizo::DataType, uint32_t streamId);

    /**
     * Implements FeedbackTimerListener interfaces
     */
    void onProcessFeedback();

    /**
     * Implements OoVooProtocolListener interfaces
     */
    virtual void onClientJoin(uint32_t userId, VideoCoderType vType, AudioCoderType aType, int errorCode); //Notify GW whether the client join the conference successfully
    virtual void onClientLeave(int errorCode); //Called when client is disconnect from AVS

    //Stream from webrtc to ooVoo
    virtual void onOutboundStreamCreate(uint32_t streamId, bool isAudio, int errorCode); //Notify whether the stream is created successfully
    virtual void onOutboundStreamClose(uint32_t streamId, int errorCode); //Notify whether the stream is closed
    virtual void onRequestIFrame();

    //Stream from ooVoo to webrtc
    virtual void onCreateInboundClient(std::string userName, uint32_t userId, std::string participantInfo);//New client join group
    virtual void onCloseInboundClient(uint32_t userId); //Client leave group
    virtual void onInboundStreamCreate(uint32_t userId, uint32_t streamId, bool isAudio, uint16_t resolution); //Notify webrtc whether video/audio is ready to be sent 
    virtual void onInboundStreamClose(uint32_t streamId); // Notify when ooVoo client's video/audio is off
    virtual void onDataReceive(mediaPacket_t*); //Called when there is a media packet received from ooVoo client
    virtual void onCustomMessage(void* pData, uint32_t length);

    //GW will take care of tcp/udp port setup
    virtual void onEstablishAVSConnection(std::string ip, uint32_t port, bool isUdp); //Ask GW to establish a connection to AVS
    virtual void onSentMessage(void*, uint32_t length, bool isUdp); //Send message to AVS

    virtual void onSyncMsg(uint64_t ntp, uint64_t rtp, bool isAudio, uint32_t streamId);

    boost::shared_ptr<erizo::MediaSource> publisher;

private:
    typedef struct {
        uint32_t userId;
        bool isAudio;
    } OoVooStreamInfo;

    struct GatewayStatistics {
        GatewayStatistics()
            : packetsReceived(0)
            , packetsLost(0)
        {
        }

        uint32_t packetsReceived;
        uint32_t packetsLost;
    };

    void closeAllSubscribers();
    void closeWebRTCClient();

    // libuv - uv_async_send() to notify node thread
    void notifyAsyncEvent(const std::string& event, const std::string& data);
    void notifyAsyncEvent(const std::string& event, uint32_t data);
    std::map<std::string, woogeen_base::EventRegistry*> m_asyncHandles;
    boost::mutex m_asyncMutex;

    boost::shared_mutex m_publisherMutex;

    std::map<uint32_t, std::vector<boost::shared_ptr<erizo::MediaSink>>> m_subscribers;
    boost::shared_mutex m_subscriberMutex;
    erizo::FeedbackSink* m_feedbackSink;
    std::atomic<bool> m_isClientLeaving;

    uint32_t m_webRTCClientId;
    // We now only support maximal one audio and one video streams for a single WebRTC client.
    uint32_t m_webRTCVideoStreamId;
    uint32_t m_webRTCAudioStreamId;
    VideoResolutionType m_webRTCVideoResolution;
    VideoCoderType m_videoCodec;
    AudioCoderType m_audioCodec;
    // Count the number of P-frames already delivered by the webrtc client since
    // last time an I-Frame is received by the Gateway.
    // This is currently used to proactively trigger FIR feedback to the webrtc
    // client at a fixed interval. We need this "workaround" now,
    // because ooVoo doesn't support IFrame request and the RTCP feedback from
    // another webrtc client (a subscriber to this publisher) has no way to be
    // bridged to the publisher.
    // The counter will be wrapped when an IFrame is received,
    // TODO: Remove me in the future please.
    std::atomic<uint32_t> m_webRTCPFramesDelivered;
    uint32_t m_timesRemainingForFIR;

    std::atomic<uint32_t> m_averageRTT;
    GatewayStatistics m_gatewayStats;

    std::map<uint32_t, OoVooStreamInfo> m_ooVooStreamInfoMap;
    boost::shared_mutex m_ooVooStreamMutex;
    std::map<uint32_t, boost::shared_ptr<woogeen_base::WebRTCFeedbackProcessor>> m_feedbackProcessors;
    boost::shared_mutex m_feedbackMutex;

    boost::shared_ptr<OoVooOutboundStreamProcessor> m_outboundStreamProcessor;
    boost::scoped_ptr<OoVooInboundStreamProcessor> m_inboundStreamProcessor;
    boost::scoped_ptr<woogeen_base::ProtectedRTPReceiver> m_videoReceiver;
    boost::scoped_ptr<woogeen_base::ProtectedRTPReceiver> m_audioReceiver;

    boost::scoped_ptr<OoVooJobTimer> m_ooVooTimer;
    boost::shared_ptr<OoVooProtocolStack> m_ooVoo;
    boost::scoped_ptr<OoVooConnection> m_ooVooConnection;
};

class OoVooJobTimer : public JobTimer {
    DECLARE_LOGGER();

public:
    OoVooJobTimer(OoVooProtocolStack* ooVoo);
    ~OoVooJobTimer();

    virtual void setStopped();
    virtual void setRunning();
    virtual bool isRunning();
    virtual void setTimer(uint64_t frequency);

    // A timer to trigger the Gateway to send the feedback to ooVoo and the
    // WebRTC client. We "steal" the thread which is used by the ooVoo Job timer
    // because that thread is actually almost idle and we don't want to introduce
    // yet another thread for a new infrequent timer.
    void setFeedbackTimer(uint32_t frequency);
    void registerFeedbackTimerListener(FeedbackTimerListener*);
    void cancelFeedbackTimer();

private:
    void timerHandler(const boost::system::error_code&);
    void feedbackTimerHandler(const boost::system::error_code&);

    bool m_isRunning;
    bool m_isStopping;
    uint64_t m_freq;

    uint32_t m_feedbackFreq;
    FeedbackTimerListener* m_fbTimerListener;
    OoVooProtocolStack* m_ooVoo;

    boost::asio::io_service m_io_service;
    boost::scoped_ptr<boost::asio::deadline_timer> m_timer;
    boost::scoped_ptr<boost::asio::deadline_timer> m_feedbackTimer;
    boost::thread m_workThread;
};

} /* namespace oovoo_gateway */
#endif /* OoVooGateway_h */
