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

#include "WebRTCGateway.h"
#include "MediaMuxerFactory.h"

#include <ProtectedRTPSender.h>

using namespace erizo;

namespace mcu {

DEFINE_LOGGER(WebRTCGateway, "mcu.WebRTCGateway");

const std::string EXTERNALOUTPUT_ID = "ExternalOutput";

WebRTCGateway::WebRTCGateway()
    : m_pendingIFrameRequests(0)
    , m_externalOutput(nullptr)
{
}

WebRTCGateway::~WebRTCGateway()
{
    closeAll();
}

int WebRTCGateway::deliverAudioData(char* buf, int len)
{
    if (len <= 0 || !m_publisherStatus.hasAudio())
        return 0;

    m_audioReceiver->deliverAudioData(buf, len);

    {
        boost::shared_lock<boost::shared_mutex> lock(m_sinkMutex);
        if (audioSink_)
            audioSink_->deliverAudioData(buf, len);
    }

    return len;
}

int WebRTCGateway::deliverVideoData(char* buf, int len)
{
    if (len <= 0 || !m_publisherStatus.hasVideo())
        return 0;

    m_videoReceiver->deliverVideoData(buf, len);

    {
        boost::shared_lock<boost::shared_mutex> lock(m_sinkMutex);
        if (videoSink_)
            videoSink_->deliverVideoData(buf, len);
    }

    return len;
}

void WebRTCGateway::receiveRtpData(char* rtpdata, int len, DataType type, uint32_t streamId)
{
    if (m_subscribers.empty() || len <= 0)
        return;

    assert(type == VIDEO || type == AUDIO);

    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(rtpdata);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT) { // Sender Report
        std::map<std::string, SubscriberInfo>::iterator it;
        boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            woogeen_base::ProtectedRTPSender* sender = type == VIDEO ? it->second.videoSender.get() : it->second.audioSender.get();
            if (sender && (type == VIDEO ? it->second.status.hasVideo() : it->second.status.hasAudio()))
                sender->sendSenderReport(rtpdata, len, type);
        }
        return;
    }

    RTPHeader* rtp = reinterpret_cast<RTPHeader*>(rtpdata);
    int headerLength = rtp->getHeaderLength();
    assert(headerLength <= len);
    if (type == AUDIO) {
        std::map<std::string, SubscriberInfo>::iterator it;
        boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if (!it->second.status.hasAudio())
                continue;
            woogeen_base::ProtectedRTPSender* sender = it->second.audioSender.get();
            if (sender)
                sender->sendPacket(rtpdata, len - headerLength, headerLength, type);
        }
        return;
    }

    assert(type == VIDEO);
    char encapsulated[MAX_DATA_PACKET_SIZE];
    if (len < MAX_DATA_PACKET_SIZE) {
        memcpy(encapsulated, rtpdata, headerLength);
        RTPHeader* newRtp = reinterpret_cast<RTPHeader*>(encapsulated);
        // One more byte for the RED header.
        newRtp->setPayloadType(RED_90000_PT);
        redheader* red = reinterpret_cast<redheader*>(&encapsulated[headerLength]);
        red->payloadtype = rtp->getPayloadType();
        red->follow = 0;
        // RED header length is 1.
        memcpy(&encapsulated[headerLength + 1], rtpdata + headerLength, len - headerLength);
    }

    std::map<std::string, SubscriberInfo>::iterator it;
    boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
    for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
        if (!it->second.status.hasVideo())
            continue;
        woogeen_base::ProtectedRTPSender* sender = it->second.videoSender.get();
        if (sender) {
            if (sender->encapsulatedRTPDataEnabled() && len < MAX_DATA_PACKET_SIZE)
                sender->sendPacket(encapsulated, len - headerLength + 1, headerLength, type);
            else
                sender->sendPacket(rtpdata, len - headerLength, headerLength, type);
        }
    }
}

void WebRTCGateway::handleIntraFrameRequest()
{
    ++m_pendingIFrameRequests;
}

bool WebRTCGateway::addPublisher(MediaSource* publisher, const std::string& id)
{
    if (m_publisher) {
        ELOG_WARN("Publisher already exists: %p, id %s, ignoring the new set request (%p, %s)",
            m_publisher.get(), m_participantId.c_str(), publisher, id.c_str());
        return false;
    }

    m_postProcessedMediaReceiver.reset(new IncomingRTPBridge(this));
    m_audioReceiver.reset(new woogeen_base::ProtectedRTPReceiver(m_postProcessedMediaReceiver));
    m_videoReceiver.reset(new woogeen_base::ProtectedRTPReceiver(m_postProcessedMediaReceiver));

    m_participantId = id;
    m_publisher.reset(publisher);

    // Set the NACK status of the RTPReceiver(s).
    FeedbackSink* feedbackSink = m_publisher->getFeedbackSink();
    bool enableNack = feedbackSink && feedbackSink->acceptNACK();
    m_audioReceiver->setNACKStatus(enableNack);
    m_videoReceiver->setNACKStatus(enableNack);
    // Set the feedback sink of the RTPReceiver (usually used for it to send immediate NACK messages).
    FeedbackSource* fbSource = m_audioReceiver->getFeedbackSource();
    if (fbSource)
        fbSource->setFeedbackSink(feedbackSink);
    fbSource = m_videoReceiver->getFeedbackSource();
    if (fbSource)
        fbSource->setFeedbackSink(feedbackSink);

    videoSourceSSRC_ = publisher->getVideoSourceSSRC();
    audioSourceSSRC_ = publisher->getAudioSourceSSRC();
    sourcefbSink_ = feedbackSink;
    videoDataType_ = publisher->getVideoDataType();
    audioDataType_ = publisher->getAudioDataType();
    videoPayloadType_ = publisher->getVideoPayloadType();
    audioPayloadType_ = publisher->getAudioPayloadType();

    publisher->setAudioSink(this);
    publisher->setVideoSink(this);

    m_feedbackTimer.reset(new woogeen_base::JobTimer(1, this));

    return true;
}

void WebRTCGateway::removePublisher(const std::string& id)
{
    if (!m_publisher || id != m_participantId) {
        ELOG_WARN("Publisher doesn't exist; can't unset the publisher");
        return;
    }

    if (!m_subscribers.empty()) {
        ELOG_WARN("There're still %zu subscribers to current publisher; can't unset the publisher", m_subscribers.size());
        return;
    }

    m_feedbackTimer->stop();

    m_publisher.reset();
}

void WebRTCGateway::addSubscriber(MediaSink* subscriber, const std::string& id, const std::string& options)
{
    ELOG_DEBUG("Adding subscriber to %u(a), %u(v)", m_publisher->getAudioSourceSSRC(), m_publisher->getVideoSourceSSRC());

    uint32_t audioSSRC = m_publisher->getAudioSourceSSRC();
    uint32_t videoSSRC = m_publisher->getVideoSourceSSRC();
    subscriber->setAudioSinkSSRC(audioSSRC);
    subscriber->setVideoSinkSSRC(videoSSRC);

    if (!m_iFrameRequestBridge)
        m_iFrameRequestBridge.reset(new IFrameRequestBridge(this));

    boost::shared_ptr<OutgoingRTPBridge> mediaBridge(new OutgoingRTPBridge(boost::shared_ptr<MediaSink>(subscriber)));
    boost::shared_ptr<woogeen_base::ProtectedRTPSender> videoSender(new woogeen_base::ProtectedRTPSender(0, mediaBridge.get()));
    boost::shared_ptr<woogeen_base::ProtectedRTPSender> audioSender(new woogeen_base::ProtectedRTPSender(0, mediaBridge.get()));
    boost::shared_ptr<woogeen_base::WebRTCFeedbackProcessor> feedback(new woogeen_base::WebRTCFeedbackProcessor(0));

    if (videoSender) {
        videoSender->setNACKStatus(subscriber->acceptResentData());
        videoSender->enableEncapsulatedRTPData(subscriber->acceptEncapsulatedRTPData());
        videoSender->setFecStatus(subscriber->acceptFEC());
    }

    if (feedback) {
        feedback->initVideoFeedbackReactor(0, videoSSRC, videoSender, m_iFrameRequestBridge);
        feedback->initAudioFeedbackReactor(0, audioSSRC, audioSender);

        FeedbackSource* fbsource = subscriber->getFeedbackSource();
        if (fbsource) {
            ELOG_DEBUG("adding fbsource");
            fbsource->setFeedbackSink(feedback.get());
        }
    }

    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    m_subscribers[id] = {feedback, mediaBridge, videoSender, audioSender, woogeen_base::MediaEnabling()};
}

void WebRTCGateway::removeSubscriber(const std::string& id)
{
    ELOG_DEBUG("Removing subscriber: id is %s", id.c_str());

    std::vector<SubscriberInfo> removedSubscribers;
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<std::string, SubscriberInfo>::iterator it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        SubscriberInfo& subscriber = it->second;
        if (subscriber.feedback) {
            subscriber.feedback->resetVideoFeedbackReactor();
            subscriber.feedback->resetAudioFeedbackReactor();
        }
        removedSubscribers.push_back(subscriber);
        m_subscribers.erase(it);
    }
    lock.unlock();
}

void WebRTCGateway::subscribeStream(const std::string& id, bool isAudio)
{
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<std::string, SubscriberInfo>::iterator it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        SubscriberInfo& subscriber = it->second;
        isAudio ? subscriber.status.enableAudio() : subscriber.status.enableVideo();
    }
    lock.unlock();
}

void WebRTCGateway::unsubscribeStream(const std::string& id, bool isAudio)
{
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<std::string, SubscriberInfo>::iterator it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        SubscriberInfo& subscriber = it->second;
        isAudio ? subscriber.status.disableAudio() : subscriber.status.disableVideo();
    }
    lock.unlock();
}

void WebRTCGateway::publishStream(const std::string& id, bool isAudio)
{
    isAudio ? m_publisherStatus.enableAudio() : m_publisherStatus.enableVideo();
}

void WebRTCGateway::unpublishStream(const std::string& id, bool isAudio)
{
    isAudio ? m_publisherStatus.disableAudio() : m_publisherStatus.disableVideo();
}

void WebRTCGateway::setAudioSink(MediaSink* sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_sinkMutex);
    audioSink_ = sink;
}

void WebRTCGateway::setVideoSink(MediaSink* sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_sinkMutex);
    videoSink_ = sink;
}

int WebRTCGateway::sendFirPacket()
{
    return m_publisher ? m_publisher->sendFirPacket() : -1;
}

int WebRTCGateway::setVideoCodec(const std::string& codecName, unsigned int clockRate)
{
    return m_publisher ? m_publisher->setVideoCodec(codecName, clockRate) : -1;
}

int WebRTCGateway::setAudioCodec(const std::string& codecName, unsigned int clockRate)
{
    return m_publisher ? m_publisher->setAudioCodec(codecName, clockRate) : -1;
}

// FIXME: No subscriber removal until WebRTCGateway's gone
woogeen_base::FrameProvider* WebRTCGateway::getVideoFrameProvider()
{
    // Create an external output instance
    if (!m_externalOutput) {
        // m_externalOutput acts as a subscriber
        m_externalOutput = new ExternalOutput();

        addSubscriber(m_externalOutput, EXTERNALOUTPUT_ID, "");
    }

    // Send I-Frame request to the publisher
    ++m_pendingIFrameRequests;

    ELOG_DEBUG("Get the VideoFrameProvider");
    return m_externalOutput->getVideoFrameProvider();
}

// FIXME: No subscriber removal until WebRTCGateway's gone
woogeen_base::FrameProvider* WebRTCGateway::getAudioFrameProvider()
{
    // Create an external output instance
    if (!m_externalOutput) {
        // m_externalOutput acts as a subscriber
        m_externalOutput = new ExternalOutput();

        addSubscriber(m_externalOutput, EXTERNALOUTPUT_ID, "");
    }

    ELOG_DEBUG("Get the AudioFrameProvider");
    return m_externalOutput->getAudioFrameProvider();
}

void WebRTCGateway::onTimeout()
{
    // Firstly, send the I-Frame requests to the publisher.
    // TODO: Tune the counter value to control the frequency of I-Frame requests.
    if (m_pendingIFrameRequests > 0) {
        m_publisher->sendFirPacket();
        m_pendingIFrameRequests = 0;
    }

    // Secondly, send the feedback from Gateway to the WebRTC client.
    FeedbackSink* feedback = m_publisher->getFeedbackSink();
    char buf[MAX_DATA_PACKET_SIZE];
    // Deliver the video feedback.
    uint32_t len = m_videoReceiver->generateRTCPFeedback(buf, MAX_DATA_PACKET_SIZE);
    if (len > 0 && feedback)
        feedback->deliverFeedback(buf, len);
    // Deliver the audio feedback.
    len = m_audioReceiver->generateRTCPFeedback(buf, MAX_DATA_PACKET_SIZE);
    if (len > 0 && feedback)
        feedback->deliverFeedback(buf, len);
}

void WebRTCGateway::closeAll()
{
    ELOG_DEBUG("closeAll");

    std::vector<SubscriberInfo> removedSubscribers;
    boost::unique_lock<boost::shared_mutex> subscriberLock(m_subscriberMutex);
    std::map<std::string, SubscriberInfo>::iterator subscriberItor = m_subscribers.begin();
    while (subscriberItor != m_subscribers.end()) {
        SubscriberInfo& subscriber = subscriberItor->second;
        if (subscriber.feedback) {
            subscriber.feedback->resetVideoFeedbackReactor();
            subscriber.feedback->resetAudioFeedbackReactor();
        }
        removedSubscribers.push_back(subscriber);
        m_subscribers.erase(subscriberItor++);
    }
    m_subscribers.clear();
    subscriberLock.unlock();

    removePublisher(m_participantId);
}

} /* namespace mcu */
