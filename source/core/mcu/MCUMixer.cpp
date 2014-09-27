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

#include "BufferManager.h"
#include "VCMMediaProcessor.h"
#include "ACMMediaProcessor.h"
#include <ProtectedRTPReceiver.h>
#include <ProtectedRTPSender.h>
#include <WoogeenTransport.h>

using namespace webrtc;
using namespace woogeen_base;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(MCUMixer, "mcu.MCUMixer");

MCUMixer::MCUMixer()
{
    this->init();
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
    m_feedback.reset(new DummyFeedbackSink());
    m_bufferManager.reset(new BufferManager());
    m_videoTransport.reset(new WoogeenVideoTransport(this));
    m_vcmOutputProcessor.reset(new VCMOutputProcessor());
    m_vcmOutputProcessor->init(m_videoTransport.get(), m_bufferManager.get());

    m_audioTransport.reset(new WoogeenAudioTransport(this));
    m_acmOutputProcessor.reset(new ACMOutputProcessor(1, m_audioTransport.get()));

    return true;

  }

  int MCUMixer::deliverAudioData(char* buf, int len, MediaSource* from) 
{
    std::map<erizo::MediaSource*, boost::shared_ptr<woogeen_base::ProtectedRTPReceiver>>::iterator it = m_publishers.find(from);
    if (it != m_publishers.end() && it->second)
        return it->second->deliverAudioData(buf, len);

    return 0;
  }

/**
 * use vcm to decode/compose/encode the streams, and then deliver to all subscribers
 * multiple publishers may call to this method simultaneously from different threads.
 * the incoming buffer is a rtp packet
 */
int MCUMixer::deliverVideoData(char* buf, int len, MediaSource* from)
{
    std::map<erizo::MediaSource*, boost::shared_ptr<woogeen_base::ProtectedRTPReceiver>>::iterator it = m_publishers.find(from);
    if (it != m_publishers.end() && it->second)
        return it->second->deliverVideoData(buf, len);

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
            if ((*it).second != NULL)
                (*it).second->deliverAudioData(buf, len);
        }
        break;
    }
    case erizo::VIDEO: {
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if ((*it).second != NULL)
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
    std::map<erizo::MediaSource*, boost::shared_ptr<woogeen_base::ProtectedRTPReceiver>>::iterator it = m_publishers.find(publisher);
    if (it == m_publishers.end() || !it->second) {
        m_vcmOutputProcessor->updateMaxSlot(maxSlot());
        boost::shared_ptr<VCMInputProcessor> ip(new VCMInputProcessor(index, m_vcmOutputProcessor.get()));
        ip->init(m_bufferManager.get());
	ACMInputProcessor* aip = new ACMInputProcessor(index);
	aip->Init(m_acmOutputProcessor.get());
	ip->setAudioInputProcessor(aip);
        m_publishers[publisher].reset(new ProtectedRTPReceiver(ip));
        //add to audio mixer
        m_acmOutputProcessor->SetMixabilityStatus(*aip, true);
    } else {
        assert("new publisher added with InputProcessor still available");    // should not go there
    }
}

void MCUMixer::addSubscriber(MediaSink* subscriber, const std::string& peerId)
{
    ELOG_DEBUG("Adding subscriber: videoSinkSSRC is %d", subscriber->getVideoSinkSSRC());
    boost::mutex::scoped_lock lock(m_subscriberMutex);
    FeedbackSource* fbsource = subscriber->getFeedbackSource();

    if (fbsource!=NULL){
      ELOG_DEBUG("adding fbsource");
      fbsource->setFeedbackSink(m_feedback.get());
    }
    m_subscribers[peerId] = boost::shared_ptr<MediaSink>(subscriber);
}

void MCUMixer::removeSubscriber(const std::string& peerId)
{
    ELOG_DEBUG("removing subscriber: peerId is %s",   peerId.c_str());
    boost::mutex::scoped_lock lock(m_subscriberMutex);
    if (m_subscribers.find(peerId) != m_subscribers.end()) {
      m_subscribers.erase(peerId);
    }
}

void MCUMixer::removePublisher(MediaSource* publisher)
{
    std::map<erizo::MediaSource*, boost::shared_ptr<woogeen_base::ProtectedRTPReceiver>>::iterator it = m_publishers.find(publisher);
    if (it != m_publishers.end()) {
        int index = getSlot(publisher);
        assert(index >= 0);
        m_publisherSlotMap[index] = NULL;
        m_publishers.erase(it);
    }
}

void MCUMixer::closeAll()
{
    boost::unique_lock<boost::mutex> lock(m_subscriberMutex);
    ELOG_DEBUG ("Mixer closeAll");
    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it = m_subscribers.begin();
    while (it != m_subscribers.end()) {
      if ((*it).second != NULL) {
        FeedbackSource* fbsource = (*it).second->getFeedbackSource();
        if (fbsource!=NULL){
          fbsource->setFeedbackSink(NULL);
        }
      }
      m_subscribers.erase(it++);
    }
    lock.unlock();
    lock.lock();
    m_subscribers.clear();
    // TODO: publishers
    ELOG_DEBUG ("ClosedAll media in this Mixer");

}

}/* namespace mcu */

