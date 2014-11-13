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

#include "InProcessMixer.h"

#include "AudioMixer.h"
#include "Config.h"
#include "VideoMixer.h"

#include <ProtectedRTPSender.h>
#include <WebRTCFeedbackProcessor.h>

using namespace woogeen_base;
using namespace erizo;

namespace mcu {

class InProcessMixerIntraFrameCallback : public woogeen_base::IntraFrameCallback {
public:
    InProcessMixerIntraFrameCallback(boost::shared_ptr<VideoMixer> videoMixer)
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

DEFINE_LOGGER(InProcessMixer, "mcu.InProcessMixer");

InProcessMixer::InProcessMixer()
{
    init();
}

InProcessMixer::~InProcessMixer()
{
    closeAll();
}

/**
 * init could be used for reset the state of this InProcessMixer
 */
bool InProcessMixer::init()
{
    m_videoMixer.reset(new VideoMixer(this));
    m_audioMixer.reset(new AudioMixer(this));

    return true;
}

int InProcessMixer::deliverAudioData(char* buf, int len, MediaSource* from) 
{
    return m_audioMixer ? m_audioMixer->deliverAudioData(buf, len, from) : 0;
}

int InProcessMixer::deliverVideoData(char* buf, int len, MediaSource* from)
{
    return m_videoMixer ? m_videoMixer->deliverVideoData(buf, len, from) : 0;
}

int InProcessMixer::deliverFeedback(char* buf, int len)
{
    if (m_videoMixer->deliverFeedback(buf, len) > 0 &&
        m_audioMixer->deliverFeedback(buf, len) > 0)
        return len;

    return 0;
}

void InProcessMixer::receiveRtpData(char* buf, int len, erizo::DataType type, uint32_t streamId)
{
    if (m_subscribers.empty() || len <= 0)
        return;

    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it;
    boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
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

int32_t InProcessMixer::addSource(MediaSource* source)
{
    int32_t voiceChannelId = m_audioMixer->addSource(source);
    if (voiceChannelId != -1)
        m_videoMixer->addSource(source, voiceChannelId, m_audioMixer->avSyncInterface());

    return 0;
}

void InProcessMixer::addSubscriber(MediaSink* subscriber, const std::string& peerId)
{
    ELOG_DEBUG("Adding subscriber to %u(a), %u(v)", m_audioMixer->sendSSRC(), m_videoMixer->sendSSRC());

    subscriber->setVideoSinkSSRC(m_videoMixer->sendSSRC());
    subscriber->setAudioSinkSSRC(m_audioMixer->sendSSRC());

    // TODO: We now just pass the feedback from _all_ of the subscribers to the video mixer without pre-processing,
    // but maybe it's needed in a InProcessMixer scenario where one mixed stream is sent to multiple subscribers.
    // The WebRTCFeedbackProcessor can be enhanced to provide another option to handle the feedback from the subscribers.
    // Lazily create the feedback sink only if there're subscribers added, because only with
    // subscribers are there chances for us to receive feedback.
    // Currently all of the subscribers shared one feedback sink because the feedback will
    // be sent to the VCMOutputProcessor which is a single instance shared by all the subscribers.
    if (0 && !m_feedback) {
        WebRTCFeedbackProcessor* feedback = new woogeen_base::WebRTCFeedbackProcessor(0);
        boost::shared_ptr<woogeen_base::IntraFrameCallback> intraFrameCallback(new InProcessMixerIntraFrameCallback(m_videoMixer));
        feedback->initVideoFeedbackReactor(MIXED_VIDEO_STREAM_ID, subscriber->getVideoSinkSSRC(), boost::shared_ptr<woogeen_base::ProtectedRTPSender>(), intraFrameCallback);
        m_feedback.reset(feedback);
    }

    FeedbackSource* fbsource = subscriber->getFeedbackSource();
    if (fbsource) {
        ELOG_DEBUG("adding fbsource");
        fbsource->setFeedbackSink(this);
    }

    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    m_subscribers[peerId] = boost::shared_ptr<MediaSink>(subscriber);
}

void InProcessMixer::removeSubscriber(const std::string& peerId)
{
    ELOG_DEBUG("Removing subscriber: id is %s", peerId.c_str());
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it = m_subscribers.find(peerId);
    if (it != m_subscribers.end())
        m_subscribers.erase(it);
}

int32_t InProcessMixer::removeSource(MediaSource* source)
{
    m_audioMixer->removeSource(source);
    m_videoMixer->removeSource(source);

    return 0;
}

void InProcessMixer::configLayout(const std::string& layout)
{
    ELOG_DEBUG("configLayout");
    Config::get()->setVideoLayout(layout);
}

void InProcessMixer::closeAll()
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
