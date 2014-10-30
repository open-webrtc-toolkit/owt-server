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

#include "MCU.h"

#include "AudioMixer.h"
#include "VideoMixer.h"

#include <ProtectedRTPSender.h>
#include <WebRTCFeedbackProcessor.h>

using namespace woogeen_base;
using namespace erizo;

namespace mcu {

class MCUIntraFrameCallback : public woogeen_base::IntraFrameCallback {
public:
    MCUIntraFrameCallback(boost::shared_ptr<VideoMixer> videoMixer)
        : m_videoMixer(videoMixer)
    {
    }

    virtual void handleIntraFrameRequest()
    {
        m_videoMixer->onRequestIFrame();
    }

private:
    boost::shared_ptr<VideoMixer> m_videoMixer;
};

DEFINE_LOGGER(MCU, "mcu.MCU");

MCU::MCU()
{
    init();
}

MCU::~MCU()
{
    closeAll();
}

/**
 * init could be used for reset the state of this MCU
 */
bool MCU::init()
{
    m_videoMixer.reset(new VideoMixer(this));
    m_audioMixer.reset(new AudioMixer(this));

    return true;
}

int MCU::deliverAudioData(char* buf, int len, MediaSource* from) 
{
    return m_audioMixer ? m_audioMixer->deliverAudioData(buf, len, from) : 0;
}

int MCU::deliverVideoData(char* buf, int len, MediaSource* from)
{
    return m_videoMixer ? m_videoMixer->deliverVideoData(buf, len, from) : 0;
}

void MCU::receiveRtpData(char* buf, int len, erizo::DataType type, uint32_t streamId)
{
    if (m_subscribers.empty() || len <= 0)
        return;

    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it;
    boost::unique_lock<boost::mutex> lock(m_subscriberMutex);
    switch (type) {
    case erizo::AUDIO: {
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if ((*it).second)
                (*it).second->deliverAudioData(buf, len);
        }
        break;
    }
    case erizo::VIDEO: {
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if ((*it).second)
                (*it).second->deliverVideoData(buf, len);
        }
        break;
    }
    default:
        break;
    }
}

void MCU::addPublisher(MediaSource* publisher)
{
    int32_t voiceChannelId = m_audioMixer->addSource(publisher);
    // TODO
    if (voiceChannelId != -1)
        m_videoMixer->addSource(publisher, voiceChannelId, m_audioMixer->avSyncInterface());
}

void MCU::addSubscriber(MediaSink* subscriber, const std::string& peerId)
{
    // Lazily create the feedback sink only if there're subscribers added, because only with
    // subscribers are there chances for us to receive feedback.
    // Currently all of the subscribers shared one feedback sink because the feedback will
    // be sent to the VCMOutputProcessor which is a single instance shared by all the subscribers.
    if (!m_feedback) {
        WebRTCFeedbackProcessor* feedback = new woogeen_base::WebRTCFeedbackProcessor(0);
        boost::shared_ptr<woogeen_base::IntraFrameCallback> intraFrameCallback(new MCUIntraFrameCallback(m_videoMixer));
        feedback->initVideoFeedbackReactor(MIXED_VIDEO_STREAM_ID, subscriber->getVideoSinkSSRC(), boost::shared_ptr<woogeen_base::ProtectedRTPSender>(), intraFrameCallback);
        m_feedback.reset(feedback);
    }

    FeedbackSource* fbsource = subscriber->getFeedbackSource();
    if (fbsource) {
        ELOG_DEBUG("adding fbsource");
        fbsource->setFeedbackSink(m_feedback.get());
    }

    boost::mutex::scoped_lock lock(m_subscriberMutex);
    m_subscribers[peerId] = boost::shared_ptr<MediaSink>(subscriber);
}

void MCU::removeSubscriber(const std::string& peerId)
{
    ELOG_DEBUG("removing subscriber: peerId is %s",   peerId.c_str());
    boost::mutex::scoped_lock lock(m_subscriberMutex);
    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it = m_subscribers.find(peerId);
    if (it != m_subscribers.end())
        m_subscribers.erase(it);
}

void MCU::removePublisher(MediaSource* publisher)
{
    m_videoMixer->removeSource(publisher);
    m_audioMixer->removeSource(publisher);
}

void MCU::closeAll()
{
    ELOG_DEBUG ("MCU closeAll");

    boost::unique_lock<boost::mutex> subscriberLock(m_subscriberMutex);
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

