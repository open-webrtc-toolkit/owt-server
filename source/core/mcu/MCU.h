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

#ifndef MCU_h
#define MCU_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>

namespace mcu {

class AudioMixer;
class VideoMixer;

/**
 * Represents a Many to Many connection.
 * Receives media from several publishers, mixed into one stream and retransmits it to every subscriber.
 */
class MCU : public erizo::MediaSink, public erizo::RTPDataReceiver {
    DECLARE_LOGGER();

public:
    DLL_PUBLIC MCU();
    DLL_PUBLIC virtual ~MCU();
    /**
     * Add a Publisher.
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
     * called by WebRtcConnections' onTransportData. This MCU
     * will be set as the MediaSink of all the WebRtcConnections in the
     * same room
     */
    virtual int deliverAudioData(char* buf, int len, erizo::MediaSource*);
    /**
     * called by WebRtcConnections' onTransportData. This MCU
     * will be set as the MediaSink of all the WebRtcConnections in the
     * same room
     */
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
#endif /* MCU_h */
