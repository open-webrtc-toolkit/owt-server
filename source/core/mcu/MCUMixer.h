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

#ifndef MCUMixer_h
#define MCUMixer_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>
#include <vector>

namespace woogeen_base {
class WoogeenVideoTransport;
class WoogeenAudioTransport;
}

namespace mcu {

class ACMOutputProcessor;
class BufferManager;
class VCMOutputProcessor;
class AVSyncTaskRunner;

class DummyFeedbackSink : public erizo::FeedbackSink {
public:
    DummyFeedbackSink() { }
    virtual ~DummyFeedbackSink() { }
    virtual int deliverFeedback(char* buf, int len) { return 0; }
};

/**
 * Represents a Many to Many connection.
 * Receives media from several publishers, mixed into one stream and retransmits it to every subscriber.
 */
class MCUMixer : public erizo::MediaSink, public erizo::RTPDataReceiver {
    DECLARE_LOGGER();

public:
    DLL_PUBLIC MCUMixer();
    DLL_PUBLIC virtual ~MCUMixer();
    /**
     * Add a Publisher.
     * Each publisher will be served by a InputProcessor, which is responsible for
     * decoding the incoming streams into I420Frames
     * @param publisher The MediaSource as the Publisher
     */
    DLL_PUBLIC void addPublisher(erizo::MediaSource*);
    /**
     * Sets the subscriber
     * @param subscriber The MediaSink as the subscriber
     * @param peerId An unique Id for the subscriber
     */
    DLL_PUBLIC void addSubscriber(erizo::MediaSink*, const std::string& peerId);
    /**
     * Eliminates the subscriber
     * @param peerId An unique Id for the subscriber
     */
    DLL_PUBLIC void removeSubscriber(const std::string& peerId);

    DLL_PUBLIC void removePublisher(erizo::MediaSource*);

    /**
     * called by WebRtcConnections' onTransportData. This MCUMixer
     * will be set as the MediaSink of all the WebRtcConnections in the
     * same room
     */
    virtual int deliverAudioData(char* buf, int len, erizo::MediaSource*);
    /**
     * called by WebRtcConnections' onTransportData. This MCUMixer
     * will be set as the MediaSink of all the WebRtcConnections in the
     * same room
     */
    virtual int deliverVideoData(char* buf, int len, erizo::MediaSource*);

    /**
     * Implements RTPDataReceiver interfaces
     */
    void receiveRtpData(char*, int len, erizo::DataType, uint32_t streamId);

    AVSyncTaskRunner* getTaskRunner() {
    	return m_taskRunner.get();
    }

private:
    struct PublishDataSink {
        boost::shared_ptr<erizo::MediaSink> audioSink;
        boost::shared_ptr<erizo::MediaSink> videoSink;
    };

    bool init();
    /**
     * Closes all the subscribers and the publisher, the object is useless after this
     */
    void closeAll();

    int assignSlot(erizo::MediaSource*);
    int maxSlot();
    // find the slot number for the corresponding publisher
    // return -1 if not found
    int getSlot(erizo::MediaSource*);

    boost::mutex m_subscriberMutex;
    std::map<std::string, boost::shared_ptr<erizo::MediaSink>> m_subscribers;
    std::map<erizo::MediaSource*, PublishDataSink> m_publishDataSinks;
    std::vector<erizo::MediaSource*> m_publisherSlotMap;    // each publisher will be allocated one index
    boost::shared_ptr<VCMOutputProcessor> m_vcmOutputProcessor;
    boost::shared_ptr<ACMOutputProcessor> m_acmOutputProcessor;
    boost::shared_ptr<woogeen_base::WoogeenVideoTransport> m_videoTransport;
    boost::shared_ptr<woogeen_base::WoogeenAudioTransport> m_audioTransport;
    boost::shared_ptr<BufferManager> m_bufferManager;
    boost::shared_ptr<erizo::FeedbackSink> m_feedback;
    boost::shared_ptr<AVSyncTaskRunner> m_taskRunner;
};

inline int MCUMixer::assignSlot(erizo::MediaSource* publisher)
{
    for (uint32_t i = 0; i < m_publisherSlotMap.size(); i++) {
        if (!m_publisherSlotMap[i]) {
            m_publisherSlotMap[i] = publisher;
            return i;
        }
    }
    m_publisherSlotMap.push_back(publisher);
    return m_publisherSlotMap.size() - 1;
}

inline int MCUMixer::maxSlot()
{
    return m_publisherSlotMap.size();
}

inline int MCUMixer::getSlot(erizo::MediaSource* publisher)
{
    for (uint32_t i = 0; i < m_publisherSlotMap.size(); i++) {
        if (m_publisherSlotMap[i] == publisher)
            return i;
    }
    return -1;
}

} /* namespace mcu */
#endif /* MCUMixer_h */
