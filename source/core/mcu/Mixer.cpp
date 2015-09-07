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

#include "MediaMuxerFactory.h"
#include "Mixer.h"

using namespace woogeen_base;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(Mixer, "mcu.Mixer");

Mixer::Mixer(boost::property_tree::ptree& videoConfig)
{
    init(videoConfig);
}

Mixer::~Mixer()
{
    closeAll();
}

int Mixer::deliverFeedback(char* buf, int len)
{
    if (m_videoMixer->deliverFeedback(buf, len) > 0
        && m_audioMixer->deliverFeedback(buf, len) > 0)
        return len;

    return 0;
}

void Mixer::receiveRtpData(char* buf, int len, erizo::DataType type, uint32_t streamId)
{
    if (m_subscribers.empty() || len <= 0)
        return;

    uint32_t ssrc = 0;
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT)
        ssrc = chead->getSSRC();
    else {
        RTPHeader* head = reinterpret_cast<RTPHeader*>(buf);
        ssrc = head->getSSRC();
    }

    std::map<std::string, std::pair<boost::shared_ptr<erizo::MediaSink>, woogeen_base::MediaEnabling>>::iterator it;
    boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
    switch (type) {
    case erizo::AUDIO: {
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if (it->second.second.hasAudio()) {
                MediaSink* sink = it->second.first.get();
                if (sink && sink->getAudioSinkSSRC() == ssrc)
                    sink->deliverAudioData(buf, len);
            }
        }
        break;
    }
    case erizo::VIDEO: {
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if (it->second.second.hasVideo()) {
                MediaSink* sink = it->second.first.get();
                if (sink && sink->getVideoSinkSSRC() == ssrc)
                    sink->deliverVideoData(buf, len);
            }
        }
        break;
    }
    default:
        break;
    }
}

void Mixer::onPositiveAudioSources(std::vector<uint32_t>& audioSources)
{
    std::vector<uint32_t> videoSources;
    for (std::vector<uint32_t>::iterator it = audioSources.begin(); it != audioSources.end(); ++it) {
        boost::shared_lock<boost::shared_mutex> lock(m_avBindingsMutex);
        std::map<uint32_t, uint32_t>::iterator found = m_avBindings.find(*it);
        if (found != m_avBindings.end())
            videoSources.push_back(found->second);
    }
    m_videoMixer->promoteSources(videoSources);
}

bool Mixer::addExternalOutput(const std::string& configParam, woogeen_base::EventRegistry* callback)
{
    // Create an ExternalOutput here
    if (configParam != "" && configParam != "undefined") {
        boost::property_tree::ptree pt;
        std::istringstream is(configParam);
        boost::property_tree::read_json(is, pt);
        const std::string outputId = pt.get<std::string>("id", "");

        woogeen_base::MediaMuxer* muxer = MediaMuxerFactory::createMediaMuxer(outputId, configParam, callback);
        if (muxer) {
            muxer->setMediaSource(m_videoMixer.get(), m_audioMixer.get());
            return true;
        } else {
            ELOG_ERROR("no media muxer is available.");
        }
    } else {
        ELOG_DEBUG("add external output error: invalid config");
    }

    return false;
}

bool Mixer::removeExternalOutput(const std::string& outputId, bool close)
{
    woogeen_base::MediaMuxer* muxer = MediaMuxerFactory::findMediaMuxer(outputId);
    if (muxer)
        muxer->unsetMediaSource();

    if (close)
        return MediaMuxerFactory::recycleMediaMuxer(outputId); // Remove the media muxer

    return true;
}

bool Mixer::addPublisher(erizo::MediaSource* publisher, const std::string& id)
{
    if (m_publishers.find(id) != m_publishers.end())
        return false;

    uint32_t audioSSRC = publisher->getAudioSourceSSRC();
    uint32_t videoSSRC = publisher->getVideoSourceSSRC();
    FeedbackSink* feedback = publisher->getFeedbackSink();
    MediaSink* audioSink = nullptr;
    MediaSink* videoSink = nullptr;

    if (audioSSRC)
        audioSink = m_audioMixer->addSource(audioSSRC, true, publisher->getAudioDataType(), publisher->getAudioPayloadType(), feedback, id);
    if (videoSSRC)
        videoSink = m_videoMixer->addSource(videoSSRC, false, publisher->getVideoDataType(), publisher->getVideoPayloadType(), feedback, id);

    if (audioSSRC && videoSSRC &&
            publisher->getAudioDataType() == DataContentType::RTP &&
            publisher->getVideoDataType() == DataContentType::RTP) {
        {
            boost::unique_lock<boost::shared_mutex> lock(m_avBindingsMutex);
            m_avBindings[audioSSRC] = videoSSRC;
        }

        m_videoMixer->bindAudio(videoSSRC, m_audioMixer->getChannelId(audioSSRC), m_audioMixer->avSyncInterface());
    }

    publisher->setAudioSink(audioSink);
    publisher->setVideoSink(videoSink);

    m_publishers[id] = publisher;
    return true;
}

void Mixer::removePublisher(const std::string& id)
{
    std::map<std::string, MediaSource*>::iterator it = m_publishers.find(id);
    if (it == m_publishers.end())
        return;

    it->second->setAudioSink(nullptr);
    it->second->setVideoSink(nullptr);

    int audioSSRC = it->second->getAudioSourceSSRC();
    int videoSSRC = it->second->getVideoSourceSSRC();

    if (audioSSRC && videoSSRC &&
            it->second->getAudioDataType() == DataContentType::RTP &&
            it->second->getVideoDataType() == DataContentType::RTP) {
        boost::unique_lock<boost::shared_mutex> lock(m_avBindingsMutex);
        m_avBindings.erase(audioSSRC);
    }
    if (audioSSRC)
        m_audioMixer->removeSource(audioSSRC, true);

    if (videoSSRC)
        m_videoMixer->removeSource(videoSSRC, false);

    m_publishers.erase(it);
}

void Mixer::addSubscriber(MediaSink* subscriber, const std::string& peerId, const std::string& options)
{
    int videoPayloadType = subscriber->preferredVideoPayloadType();
    // FIXME: Now we hard code the output to be NACK enabled and FEC disabled,
    // because the video mixer now is not able to output different formatted
    // RTP packets for a single encoded stream elegantly.
    bool enableNACK = true || subscriber->acceptResentData();
    bool enableFEC = false && subscriber->acceptFEC();

    unsigned int width {0};
    unsigned int height {0};
    try {
        boost::property_tree::ptree resolution;
        std::istringstream is(options);
        boost::property_tree::read_json(is, resolution);
        width = resolution.get("width", 0);
        height = resolution.get("height", 0);
    } catch (std::exception const& e) {
        ELOG_DEBUG("ignore error in parsing addSubscriber options: %s", e.what());
        width = 0;
        height = 0;
    }
    VideoSize size {width, height};

    m_videoMixer->addOutput(videoPayloadType, enableNACK, enableFEC, size);
    subscriber->setVideoSinkSSRC(m_videoMixer->getSendSSRC(videoPayloadType, enableNACK, enableFEC, size));

    int audioPayloadType = subscriber->preferredAudioPayloadType();
    int32_t channelId = m_audioMixer->addOutput(peerId, audioPayloadType);
    subscriber->setAudioSinkSSRC(m_audioMixer->getSendSSRC(channelId));

    ELOG_DEBUG("Adding subscriber to %u(a), %u(v)", m_audioMixer->getSendSSRC(channelId), m_videoMixer->getSendSSRC(videoPayloadType, enableNACK, enableFEC, size));

    // TODO: We now just pass the feedback from _all_ of the subscribers to the video mixer without pre-processing,
    // but maybe it's needed in a Mixer scenario where one mixed stream is sent to multiple subscribers.
    // The WebRTCFeedbackProcessor can be enhanced to provide another option to handle the feedback from the subscribers.

    FeedbackSource* fbsource = subscriber->getFeedbackSource();
    if (fbsource) {
        ELOG_DEBUG("adding fbsource");
        fbsource->setFeedbackSink(this);
    }

    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    m_subscribers[peerId] = std::pair<boost::shared_ptr<erizo::MediaSink>,
                                woogeen_base::MediaEnabling>(boost::shared_ptr<MediaSink>(subscriber), woogeen_base::MediaEnabling());
}

void Mixer::removeSubscriber(const std::string& peerId)
{
    ELOG_DEBUG("Removing subscriber: id is %s", peerId.c_str());
    m_audioMixer->removeOutput(peerId);

    std::vector<boost::shared_ptr<MediaSink>> removedSubscribers;
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    auto it = m_subscribers.find(peerId);
    if (it != m_subscribers.end()) {
        removedSubscribers.push_back(it->second.first);
        m_subscribers.erase(it);
    }
    lock.unlock();
}

void Mixer::subscribeStream(const std::string& id, bool isAudio)
{
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    auto it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        isAudio ? it->second.second.enableAudio() : it->second.second.enableVideo();
    }
    lock.unlock();
}

void Mixer::unsubscribeStream(const std::string& id, bool isAudio)
{
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    auto it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        isAudio ? it->second.second.disableAudio() : it->second.second.disableVideo();
    }
    lock.unlock();
}

void Mixer::publishStream(const std::string& id, bool isAudio)
{
}

void Mixer::unpublishStream(const std::string& id, bool isAudio)
{
}

bool Mixer::setVideoBitrate(const std::string& id, uint32_t kbps)
{
    std::map<std::string, MediaSource*>::iterator it = m_publishers.find(id);
    if (it == m_publishers.end() || it->second->getVideoDataType() == DataContentType::ENCODED_FRAME)
        return false;

    uint32_t videoSource = it->second->getVideoSourceSSRC();
    return m_videoMixer->setSourceBitrate(videoSource, kbps);
}

std::string Mixer::getRegion(const std::string& participantId)
{
    std::map<std::string, MediaSource*>::iterator it = m_publishers.find(participantId);
    if (it == m_publishers.end())
        return "";

    uint32_t videoSource = it->second->getVideoSourceSSRC();
    return m_videoMixer->getSourceRegion(videoSource);
}

bool Mixer::setRegion(const std::string& participantId, const std::string& regionId)
{
    std::map<std::string, MediaSource*>::iterator it = m_publishers.find(participantId);
    if (it == m_publishers.end())
        return false;

    uint32_t videoSource = it->second->getVideoSourceSSRC();
    return m_videoMixer->specifySourceRegion(videoSource, regionId);
}

bool Mixer::init(boost::property_tree::ptree& videoConfig)
{
    m_videoMixer.reset(new VideoMixer(this, videoConfig));
    bool avCoordinated = videoConfig.get<bool>("avcoordinate");
    m_audioMixer.reset(new AudioMixer(this, this, avCoordinated));
    m_videoMixer->setupNotification([this](const std::string& event, const std::string& data) {
        this->notifyAsyncEvent(event, data);
    });
    return true;
}

void Mixer::closeAll()
{
    ELOG_DEBUG("closeAll");

    std::vector<boost::shared_ptr<MediaSink>> removedSubscribers;
    boost::unique_lock<boost::shared_mutex> subscriberLock(m_subscriberMutex);
    auto subscriberItor = m_subscribers.begin();
    while (subscriberItor != m_subscribers.end()) {
        boost::shared_ptr<MediaSink>& subscriber = subscriberItor->second.first;
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
}

// The main thread
void Mixer::setupAsyncEvent(const std::string& event, EventRegistry* handle)
{
    std::map<std::string, EventRegistry*>::iterator it = m_asyncHandles.find(event);
    if (it != m_asyncHandles.end()) {
        delete it->second;
        it->second = handle;
        ELOG_DEBUG("setupAsyncEvent - replace old listener for %s", event.c_str());
    } else {
        m_asyncHandles[event] = handle;
        ELOG_DEBUG("setupAsyncEvent - add listener for %s##%p", event.c_str(), handle);
    }
}

// The main thread
void Mixer::destroyAsyncEvents()
{
    // This method should be invoked by the main thread when the WebRTC client
    // (browser) disconnects and the main thread are closing the uv async handles.
    // Other threads should check the existence of the event before async send.
    boost::lock_guard<boost::mutex> lock(m_asyncMutex);
    std::map<std::string, EventRegistry*>::iterator it;
    for (it = m_asyncHandles.begin(); it != m_asyncHandles.end(); it++) {
        delete it->second;
        m_asyncHandles.erase(it);
    }
}

void Mixer::notifyAsyncEvent(const std::string& event, const std::string& data)
{
    boost::lock_guard<boost::mutex> lock(m_asyncMutex);
    std::map<std::string, EventRegistry*>::iterator it = m_asyncHandles.find(event);
    if (it != m_asyncHandles.end()) {
        bool sent = it->second->notify(data);
        ELOG_DEBUG("notifyAsyncEvent - event: %s; sent? %d", event.c_str(), sent);
    } else {
        ELOG_DEBUG("notifyAsyncEvent - missing event handler: %s", event.c_str());
    }
}

void Mixer::notifyAsyncEvent(const std::string& event, uint32_t data)
{
    std::ostringstream message;
    message << data;
    notifyAsyncEvent(event, message.str());
}

} /* namespace mcu */
