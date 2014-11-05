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

#ifndef InProcessMixer_h
#define InProcessMixer_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <Gateway.h>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>

namespace mcu {

class AudioMixer;
class VideoMixer;

/**
 * An InProcessMixer refers to a media mixer which is run in the same process as a woogeen_base::Gateway.
 * It receives media from several sources through the WebRTCGateways, mixed them into one stream and retransmits
 * it to every subscriber.
 */
class InProcessMixer : public woogeen_base::Gateway, public erizo::MediaSink, public erizo::RTPDataReceiver {
    DECLARE_LOGGER();

public:
    InProcessMixer();
    virtual ~InProcessMixer();

    /**
     * Implements Gateway interfaces
     */
    bool setPublisher(erizo::MediaSource*) { return false; }
    bool setPublisher(erizo::MediaSource* source, const std::string& videoResolution) { return setPublisher(source); }
    void unsetPublisher() { }

    void addSubscriber(erizo::MediaSink*, const std::string& id);
    void removeSubscriber(const std::string& id);

    void setupAsyncEvent(const std::string& event, woogeen_base::EventRegistry*) { }
    void destroyAsyncEvents() { }

    bool clientJoin(const std::string& clientJoinUri) { return true; }
    void customMessage(const std::string& message) { }

    std::string retrieveGatewayStatistics() { return ""; }

    void subscribeStream(const std::string& id, bool isAudio) { }
    void unsubscribeStream(const std::string& id, bool isAudio) { }
    void publishStream(bool isAudio) { }
    void unpublishStream(bool isAudio) { }

    void setAdditionalSourceConsumer(woogeen_base::MediaSourceConsumer*) { }

    int32_t addSource(erizo::MediaSource*);
    int32_t removeSource(erizo::MediaSource*);
    erizo::MediaSink* mediaSink() { return this; }
    void configLayout(const std::string&);

    /**
     * Implements MediaSink interfaces
     */
    virtual int deliverAudioData(char* buf, int len, erizo::MediaSource*);
    virtual int deliverVideoData(char* buf, int len, erizo::MediaSource*);

    /**
     * Implements RTPDataReceiver interfaces
     */
    void receiveRtpData(char*, int len, erizo::DataType, uint32_t streamId);

private:
    bool init();
    /**
     * Closes all the subscribers and the publisher, the object is useless after this
     */
    void closeAll();

    boost::shared_ptr<erizo::FeedbackSink> m_feedback;
    boost::mutex m_subscriberMutex;
    std::map<std::string, boost::shared_ptr<erizo::MediaSink>> m_subscribers;

    boost::shared_ptr<VideoMixer> m_videoMixer;
    boost::shared_ptr<AudioMixer> m_audioMixer;
};

} /* namespace mcu */
#endif /* InProcessMixer_h */
