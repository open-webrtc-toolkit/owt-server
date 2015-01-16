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

int Mixer::deliverAudioData(char* buf, int len) 
{
    return m_audioMixer ? m_audioMixer->deliverAudioData(buf, len) : 0;
}

int Mixer::deliverVideoData(char* buf, int len)
{
    return m_videoMixer ? m_videoMixer->deliverVideoData(buf, len) : 0;
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

    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it;
    boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
    switch (type) {
    case erizo::AUDIO: {
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            MediaSink* sink = it->second.get();
            if (sink && sink->getAudioSinkSSRC() == ssrc)
                sink->deliverAudioData(buf, len);
        }
        break;
    }
    case erizo::VIDEO: {
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            MediaSink* sink = it->second.get();
            if (sink && sink->getVideoSinkSSRC() == ssrc)
                sink->deliverVideoData(buf, len);
        }
        break;
    }
    default:
        break;
    }
}

int32_t Mixer::addSource(uint32_t id, bool isAudio, FeedbackSink* feedback, const std::string& participantId)
{
    if (isAudio)
        return m_audioMixer->addSource(id, true, feedback, participantId);

    return m_videoMixer->addSource(id, false, feedback, participantId);
}

int32_t Mixer::bindAV(uint32_t audioSource, uint32_t videoSource)
{
    return m_videoMixer->bindAudio(videoSource, m_audioMixer->getChannelId(audioSource), m_audioMixer->avSyncInterface());
}

void Mixer::addSubscriber(MediaSink* subscriber, const std::string& peerId)
{
    int videoPayloadType = subscriber->preferredVideoPayloadType();
    m_videoMixer->addOutput(videoPayloadType);
    subscriber->setVideoSinkSSRC(m_videoMixer->getSendSSRC(videoPayloadType));

    int32_t channelId = m_audioMixer->addOutput(peerId);
    subscriber->setAudioSinkSSRC(m_audioMixer->getSendSSRC(channelId));

    ELOG_DEBUG("Adding subscriber to %u(a), %u(v)", m_audioMixer->getSendSSRC(channelId), m_videoMixer->getSendSSRC(videoPayloadType));

    // TODO: We now just pass the feedback from _all_ of the subscribers to the video mixer without pre-processing,
    // but maybe it's needed in a Mixer scenario where one mixed stream is sent to multiple subscribers.
    // The WebRTCFeedbackProcessor can be enhanced to provide another option to handle the feedback from the subscribers.

    FeedbackSource* fbsource = subscriber->getFeedbackSource();
    if (fbsource) {
        ELOG_DEBUG("adding fbsource");
        fbsource->setFeedbackSink(this);
    }

    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    m_subscribers[peerId] = boost::shared_ptr<MediaSink>(subscriber);
}

void Mixer::removeSubscriber(const std::string& peerId)
{
    ELOG_DEBUG("Removing subscriber: id is %s", peerId.c_str());
    m_audioMixer->removeOutput(peerId);

    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it = m_subscribers.find(peerId);
    if (it != m_subscribers.end())
        m_subscribers.erase(it);
}

int32_t Mixer::removeSource(uint32_t source, bool isAudio)
{
    return isAudio ? m_audioMixer->removeSource(source, true) : m_videoMixer->removeSource(source, false);
}

bool Mixer::init(boost::property_tree::ptree& videoConfig)
{
    m_videoMixer.reset(new VideoMixer(this, videoConfig));
    m_audioMixer.reset(new AudioMixer(this));

    return true;
}

void Mixer::closeAll()
{
    ELOG_DEBUG("closeAll");

    boost::unique_lock<boost::shared_mutex> subscriberLock(m_subscriberMutex);
    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator subscriberItor = m_subscribers.begin();
    while (subscriberItor != m_subscribers.end()) {
        boost::shared_ptr<MediaSink>& subscriber = subscriberItor->second;
        if (subscriber) {
            FeedbackSource* fbsource = subscriber->getFeedbackSource();
            if (fbsource)
                fbsource->setFeedbackSink(nullptr);
        }
        m_subscribers.erase(subscriberItor++);
    }
    m_subscribers.clear();
}

}/* namespace mcu */
