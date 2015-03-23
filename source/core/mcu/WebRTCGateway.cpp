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

#include <ProtectedRTPSender.h>

using namespace erizo;

namespace mcu {

DEFINE_LOGGER(WebRTCGateway, "mcu.WebRTCGateway");

WebRTCGateway::WebRTCGateway()
    : m_mixer(nullptr)
    , m_pendingIFrameRequests(0)
{
}

WebRTCGateway::~WebRTCGateway()
{
    closeAll();
}

int WebRTCGateway::deliverAudioData(char* buf, int len)
{
    if (len <= 0)
        return 0;

    m_audioReceiver->deliverAudioData(buf, len);

    if (m_mixer && m_mixer->mediaSink())
        m_mixer->mediaSink()->deliverAudioData(buf, len);

    return len;
}

int WebRTCGateway::deliverVideoData(char* buf, int len)
{
    if (len <= 0)
        return 0;

    m_videoReceiver->deliverVideoData(buf, len);

    if (m_mixer && m_mixer->mediaSink())
        m_mixer->mediaSink()->deliverVideoData(buf, len);

    return len;
}

void WebRTCGateway::receiveRtpData(char* rtpdata, int len, DataType type, uint32_t streamId)
{
    if (m_subscribers.empty() || len <= 0)
        return;

    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it;
    boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
    switch (type) {
    case erizo::VIDEO:
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if ((*it).second)
                (*it).second->deliverVideoData(rtpdata, len);
        }
        break;
    case erizo::AUDIO:
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if ((*it).second)
                (*it).second->deliverAudioData(rtpdata, len);
        }
        break;
    default:
        break;
    }
}

void WebRTCGateway::handleIntraFrameRequest()
{
    ++m_pendingIFrameRequests;
}

bool WebRTCGateway::setPublisher(MediaSource* publisher, const std::string& id)
{
    if (m_publisher) {
        ELOG_WARN("Publisher already exists: %p, id %s, ignoring the new set request (%p, %s)",
            m_publisher.get(), m_participantId.c_str(), publisher, id.c_str());
        return false;
    }

    m_postProcessedMediaReceiver.reset(new RTPDataReceiveBridge(this));
    m_audioReceiver.reset(new woogeen_base::ProtectedRTPReceiver(m_postProcessedMediaReceiver));
    m_videoReceiver.reset(new woogeen_base::ProtectedRTPReceiver(m_postProcessedMediaReceiver));

    m_participantId = id;
    m_publisher.reset(publisher);

    // Set the NACK status of the RTPReceiver(s).
    erizo::FeedbackSink* feedbackSink = m_publisher->getFeedbackSink();
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

    publisher->setAudioSink(this);
    publisher->setVideoSink(this);

    m_feedbackTimer.reset(new woogeen_base::JobTimer(1, this));

    return true;
}

void WebRTCGateway::unsetPublisher()
{
    if (!m_publisher) {
        ELOG_WARN("Publisher doesn't exist; can't unset the publisher");
        return;
    }

    if (!m_subscribers.empty()) {
        ELOG_WARN("There're still %zu subscribers to current publisher; can't unset the publisher", m_subscribers.size());
        return;
    }

    if (m_mixer) {
        m_mixer->removeSource(m_publisher->getAudioSourceSSRC(), true);
        m_mixer->removeSource(m_publisher->getVideoSourceSSRC(), false);
    }

    m_feedbackTimer->stop();

    m_publisher.reset();
    // The mixer reference and feedback processor need to be resetted because
    // they rely on the valid publisher information like the SSRCs.
    m_mixer = nullptr;
    m_feedback.reset();
}

void WebRTCGateway::addSubscriber(MediaSink* subscriber, const std::string& id)
{
    ELOG_DEBUG("Adding subscriber to %u(a), %u(v)", m_publisher->getAudioSourceSSRC(), m_publisher->getVideoSourceSSRC());

    uint32_t audioSSRC = m_publisher->getAudioSourceSSRC();
    uint32_t videoSSRC = m_publisher->getVideoSourceSSRC();
    subscriber->setAudioSinkSSRC(audioSSRC);
    subscriber->setVideoSinkSSRC(videoSSRC);

    if (!m_feedback) {
        m_feedback.reset(new woogeen_base::WebRTCFeedbackProcessor(0));
        m_iFrameRequestBridge.reset(new IFrameRequestBridge(this));
        boost::shared_ptr<woogeen_base::ProtectedRTPSender> noSender;
        m_feedback->initVideoFeedbackReactor(0, videoSSRC, noSender, m_iFrameRequestBridge);
        m_feedback->initAudioFeedbackReactor(0, audioSSRC, noSender);
    }

    FeedbackSource* fbsource = subscriber->getFeedbackSource();
    if (fbsource) {
        ELOG_DEBUG("adding fbsource");
        fbsource->setFeedbackSink(m_feedback.get());
    }

    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    m_subscribers[id] = boost::shared_ptr<MediaSink>(subscriber);
}

void WebRTCGateway::removeSubscriber(const std::string& id)
{
    ELOG_DEBUG("Removing subscriber: id is %s", id.c_str());

    std::vector<boost::shared_ptr<MediaSink>> removedSubscribers;
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        removedSubscribers.push_back(it->second);
        m_subscribers.erase(it);
    }
    lock.unlock();
}

void WebRTCGateway::setAdditionalSourceConsumer(woogeen_base::MediaSourceConsumer* mixer)
{
    m_mixer = mixer;

    uint32_t audioSSRC = m_publisher->getAudioSourceSSRC();
    uint32_t videoSSRC = m_publisher->getVideoSourceSSRC();
    FeedbackSink* feedback = m_publisher->getFeedbackSink();

    if (audioSSRC)
        mixer->addSource(audioSSRC, true, feedback, m_participantId);
    if (videoSSRC)
        mixer->addSource(videoSSRC, false, feedback, m_participantId);

    if (audioSSRC && videoSSRC)
        mixer->bindAV(audioSSRC, videoSSRC);

    if (m_mixer->mediaSink()) {
        // Disable NACK of the RTPReceiver(s).
        m_audioReceiver->setNACKStatus(false);
        m_videoReceiver->setNACKStatus(false);
    }
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

    // If the stream will be handed over to another source consumer (usually a mixer),
    // we skip the feedback from the subscribers because the source consumer is responsible
    // for generating the feedback to the publisher.
    if (m_mixer && m_mixer->mediaSink())
        return;

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

    std::vector<boost::shared_ptr<MediaSink>> removedSubscribers;
    boost::unique_lock<boost::shared_mutex> subscriberLock(m_subscriberMutex);
    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator subscriberItor = m_subscribers.begin();
    while (subscriberItor != m_subscribers.end()) {
        boost::shared_ptr<MediaSink>& subscriber = subscriberItor->second;
        if (subscriber) {
            FeedbackSource* fbsource = subscriber->getFeedbackSource();
            if (fbsource)
                fbsource->setFeedbackSink(nullptr);
            removedSubscribers.push_back(subscriber);
        }
        m_subscribers.erase(subscriberItor++);
    }
    m_subscribers.clear();
    subscriberLock.unlock();

    m_feedback.reset();

    unsetPublisher();
}

}/* namespace mcu */
