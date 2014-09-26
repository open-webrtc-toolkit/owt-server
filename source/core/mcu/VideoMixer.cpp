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

#include "VideoMixer.h"

#include "BufferManager.h"
#include "VCMMediaProcessor.h"
#include "ACMMediaProcessor.h"
#include <ProtectedRTPReceiver.h>
#include <ProtectedRTPSender.h>
#include <WebRtcConnection.h>
#include <WoogeenTransport.h>

using namespace webrtc;
using namespace woogeen_base;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(VideoMixer, "media.mixers.VideoMixer");

VideoMixer::VideoMixer()
{
    this->init();
}

VideoMixer::~VideoMixer()
{
    closeAll();
    delete bufferManager_;
    delete videoTransport_;
    delete audioTransport_;
    delete aop_;
    delete vop_;
}

/**
 * init could be used for reset the state of this VideoMixer
 */
bool VideoMixer::init()
{
    feedback_.reset(new DummyFeedbackSink());
    bufferManager_ = new BufferManager();
    videoTransport_ = new WoogeenVideoTransport(this);
    vop_ = new VCMOutputProcessor();
    vop_->init(videoTransport_, bufferManager_);

    audioTransport_ = new WoogeenAudioTransport(this);
    aop_ = new ACMOutputProcessor(1, audioTransport_);

    return true;

  }

  int VideoMixer::deliverAudioData(char* buf, int len, MediaSource* from) 
{
    ProtectedRTPReceiver* receiver = publishers_[from];
    if (receiver == NULL)
        return 0;		// Is this possible when remove publisher is called while media stream still comes in?
    receiver->deliverAudioData(buf, len);
    return 0;
  }

/**
 * use vcm to decode/compose/encode the streams, and then deliver to all subscribers
 * multiple publishers may call to this method simultaneously from different threads.
 * the incoming buffer is a rtp packet
 */
int VideoMixer::deliverVideoData(char* buf, int len, MediaSource* from)
{
    ProtectedRTPReceiver* receiver = publishers_[from];
    if (receiver == NULL)
        return 0;        // Is this possible when remove publisher is called while media stream still comes in?

    receiver->deliverVideoData(buf, len);
    return 0;
}

void VideoMixer::receiveRtpData(char* buf, int len, erizo::DataType type, uint32_t streamId)
{
    if (subscribers_.empty() || len <= 0)
        return;

    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it;
    boost::unique_lock<boost::mutex> lock(myMonitor_);
    switch (type) {
    case erizo::AUDIO: {
        for (it = subscribers_.begin(); it != subscribers_.end(); ++it) {
            if ((*it).second != NULL)
                (*it).second->deliverAudioData(buf, len);
        }
        break;
    }
    case erizo::VIDEO: {
        for (it = subscribers_.begin(); it != subscribers_.end(); ++it) {
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
void VideoMixer::addPublisher(MediaSource* puber)
{
    int index = assignSlot(puber);
    ELOG_DEBUG("addPublisher - assigned slot is %d", index);
    if (publishers_[puber] == NULL) {
        vop_->updateMaxSlot(maxSlot());
        boost::shared_ptr<VCMInputProcessor> ip(new VCMInputProcessor(index, vop_));
        ip->init(bufferManager_);
	ACMInputProcessor* aip = new ACMInputProcessor(index);
	aip->Init(aop_);
	ip->setAudioInputProcessor(aip);
        ProtectedRTPReceiver* protectedRTPReceiver = new ProtectedRTPReceiver(ip);
        publishers_[puber] = protectedRTPReceiver;
        //add to audio mixer
         aop_->SetMixabilityStatus(*aip, true);
    } else {
        assert (!"new publisher added with InputProcessor still available");    // should not go there
    }
}

void VideoMixer::addSubscriber(MediaSink* suber, const std::string& peerId)
{
    ELOG_DEBUG("Adding subscriber: videoSinkSSRC is %d", suber->getVideoSinkSSRC());
    boost::mutex::scoped_lock lock(myMonitor_);
    FeedbackSource* fbsource = suber->getFeedbackSource();

    if (fbsource!=NULL){
      ELOG_DEBUG("adding fbsource");
      fbsource->setFeedbackSink(feedback_.get());
    }
    subscribers_[peerId] = boost::shared_ptr<MediaSink>(suber);
}

void VideoMixer::removeSubscriber(const std::string& peerId)
{
    ELOG_DEBUG("removing subscriber: peerId is %s",   peerId.c_str());
    boost::mutex::scoped_lock lock(myMonitor_);
    if (subscribers_.find(peerId) != subscribers_.end()) {
      subscribers_.erase(peerId);
    }
}

void VideoMixer::removePublisher(MediaSource* puber)
{
    if (publishers_[puber]) {
        int index = getSlot(puber);
        assert(index >= 0);
        puberSlotMap_[index] = NULL;
        delete publishers_[puber];
        publishers_.erase(puber);
    }
}

void VideoMixer::closeSink()
{ 
}

void VideoMixer::closeAll()
{
    boost::unique_lock<boost::mutex> lock(myMonitor_);
    ELOG_DEBUG ("Mixer closeAll");
    std::map<std::string, boost::shared_ptr<MediaSink>>::iterator it = subscribers_.begin();
    while (it != subscribers_.end()) {
      if ((*it).second != NULL) {
        FeedbackSource* fbsource = (*it).second->getFeedbackSource();
        if (fbsource!=NULL){
          fbsource->setFeedbackSink(NULL);
        }
      }
      subscribers_.erase(it++);
    }
    lock.unlock();
    lock.lock();
    subscribers_.clear();
    // TODO: publishers
    ELOG_DEBUG ("ClosedAll media in this Mixer");

}

}/* namespace mcu */

