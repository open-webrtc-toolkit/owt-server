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

#include "MCUMixer.h"

#include "AudioProcessor.h"
#include "BufferManager.h"
#include "TaskRunner.h"
#include "VCMInputProcessor.h"
#include "VCMOutputProcessor.h"
#include <ProtectedRTPSender.h>
#include <WebRTCFeedbackProcessor.h>
#include <WoogeenTransport.h>
#include <webrtc/system_wrappers/interface/trace.h>

using namespace webrtc;
using namespace woogeen_base;
using namespace erizo;

namespace mcu {

class MCUIntraFrameCallback : public woogeen_base::IntraFrameCallback {
public:
    MCUIntraFrameCallback(boost::shared_ptr<VCMOutputProcessor> vcmOutputProcessor)
        : m_vcmOutputProcessor(vcmOutputProcessor)
    {
    }

    virtual void handleIntraFrameRequest()
    {
        m_vcmOutputProcessor->onRequestIFrame();
    }

private:
    boost::shared_ptr<VCMOutputProcessor> m_vcmOutputProcessor;
};

static const int MIXED_VIDEO_STREAM_ID = 2;

DEFINE_LOGGER(MCUMixer, "mcu.MCUMixer");

MCUMixer::MCUMixer()
{
    init();
}

MCUMixer::~MCUMixer()
{
    closeAll();
}

/**
 * init could be used for reset the state of this MCUMixer
 */
bool MCUMixer::init()
{
    Trace::CreateTrace();
    Trace::SetTraceFile("webrtc.trace.txt");
    Trace::set_level_filter(webrtc::kTraceAll);

    m_bufferManager.reset(new BufferManager());
    m_taskRunner.reset(new TaskRunner());

    m_vcmOutputProcessor.reset(new VCMOutputProcessor(MIXED_VIDEO_STREAM_ID));
    m_vcmOutputProcessor->init(new WoogeenTransport<erizo::VIDEO>(this, nullptr/*not enabled yet*/), m_bufferManager, m_taskRunner);

    m_audioProcessor.reset(new AudioProcessor());
    m_audioProcessor->setOutTransport(new WoogeenTransport<erizo::AUDIO>(this, nullptr));

    m_taskRunner->Start();

    return true;
}

int MCUMixer::deliverAudioData(char* buf, int len, MediaSource* from) 
{
    boost::shared_lock<boost::shared_mutex> lock(m_publisherMutex);
    std::map<erizo::MediaSource*, boost::shared_ptr<erizo::MediaSink>>::iterator it = m_sinksForPublishers.find(from);
    if (it != m_sinksForPublishers.end() && it->second)
        return it->second->deliverAudioData(buf, len, from);

    return 0;
}

/**
 * use vcm to decode/compose/encode the streams, and then deliver to all subscribers
 * multiple publishers may call to this method simultaneously from different threads.
 * the incoming buffer is a rtp packet
 */
int MCUMixer::deliverVideoData(char* buf, int len, MediaSource* from)
{
    boost::shared_lock<boost::shared_mutex> lock(m_publisherMutex);
    std::map<erizo::MediaSource*, boost::shared_ptr<erizo::MediaSink>>::iterator it = m_sinksForPublishers.find(from);
    if (it != m_sinksForPublishers.end() && it->second)
        return it->second->deliverVideoData(buf, len, from);

    return 0;
}

void MCUMixer::receiveRtpData(char* buf, int len, erizo::DataType type, uint32_t streamId)
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

/**
 * Attach a new InputStream to the Transcoder
 */
void MCUMixer::addPublisher(MediaSource* publisher)
{
    int index = assignSlot(publisher);
    ELOG_DEBUG("addPublisher - assigned slot is %d", index);
    boost::upgrade_lock<boost::shared_mutex> lock(m_publisherMutex);
    std::map<erizo::MediaSource*, boost::shared_ptr<erizo::MediaSink>>::iterator it = m_sinksForPublishers.find(publisher);
    if (it == m_sinksForPublishers.end() || !it->second) {
        m_vcmOutputProcessor->updateMaxSlot(maxSlot());

        boost::shared_ptr<VCMInputProcessor> videoInputProcessor(new VCMInputProcessor(index));
        videoInputProcessor->init(new WoogeenTransport<erizo::VIDEO>(this, publisher->getFeedbackSink()),
        							m_bufferManager,
        							m_vcmOutputProcessor,
        							m_taskRunner);
        int32_t voiceChannelId = m_audioProcessor->addSource(publisher, new WoogeenTransport<erizo::AUDIO>(this, publisher->getFeedbackSink()));
        videoInputProcessor->bindAudioForSync(voiceChannelId, m_audioProcessor->avSyncInterface());

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_sinksForPublishers[publisher].reset(new SeparateMediaSink(m_audioProcessor, videoInputProcessor));
    } else
        assert("new publisher added with InputProcessor still available");    // should not go there
}

void MCUMixer::addSubscriber(MediaSink* subscriber, const std::string& peerId)
{
    ELOG_DEBUG("Adding subscriber: videoSinkSSRC is %d", subscriber->getVideoSinkSSRC());

    // Lazily create the feedback sink only if there're subscribers added, because only with
    // subscribers are there chances for us to receive feedback.
    // Currently all of the subscribers shared one feedback sink because the feedback will
    // be sent to the VCMOutputProcessor which is a single instance shared by all the subscribers.
    if (!m_feedback) {
        WebRTCFeedbackProcessor* feedback = new woogeen_base::WebRTCFeedbackProcessor(0);
        boost::shared_ptr<woogeen_base::IntraFrameCallback> intraFrameCallback(new MCUIntraFrameCallback(m_vcmOutputProcessor));
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

void MCUMixer::removeSubscriber(const std::string& peerId)
{
    ELOG_DEBUG("removing subscriber: peerId is %s",   peerId.c_str());
    boost::mutex::scoped_lock lock(m_subscriberMutex);
    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it = m_subscribers.find(peerId);
    if (it != m_subscribers.end())
        m_subscribers.erase(it);
}

void MCUMixer::removePublisher(MediaSource* publisher)
{
    boost::unique_lock<boost::shared_mutex> lock(m_publisherMutex);
    std::map<erizo::MediaSource*, boost::shared_ptr<erizo::MediaSink>>::iterator it = m_sinksForPublishers.find(publisher);
    if (it != m_sinksForPublishers.end()) {
        int index = getSlot(publisher);
        assert(index >= 0);
        m_publisherSlotMap[index] = nullptr;
        m_sinksForPublishers.erase(it);
    }
    lock.unlock();
    m_audioProcessor->removeSource(publisher);
    delete publisher;
}

void MCUMixer::closeAll()
{
    ELOG_DEBUG ("Mixer closeAll");
    m_taskRunner->Stop();

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
    subscriberLock.unlock();

    boost::unique_lock<boost::shared_mutex> publisherLock(m_publisherMutex);
    std::map<erizo::MediaSource*, boost::shared_ptr<erizo::MediaSink>>::iterator publisherItor = m_sinksForPublishers.begin();
    while (publisherItor != m_sinksForPublishers.end()) {
        MediaSource* publisher = publisherItor->first;
        int index = getSlot(publisher);
        assert(index >= 0);
        m_publisherSlotMap[index] = nullptr;
        m_sinksForPublishers.erase(publisherItor++);
        // Delete the publisher as a MediaSource.
        // We need to release the lock before deleting it because the destructor of the publisher
        // will need to wait for its working thread to finish the work which may need the lock.
        publisherLock.unlock();
        delete publisher;
        publisherLock.lock();
    }
    m_sinksForPublishers.clear();
    ELOG_DEBUG ("ClosedAll media in this Mixer");

    Trace::ReturnTrace();
}

}/* namespace mcu */

