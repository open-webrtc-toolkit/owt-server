/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#ifndef Gateway_h
#define Gateway_h

#include "EventRegistry.h"

#include <MediaDefinitions.h>

namespace woogeen_base {

class FrameProvider;

/**
 * Represents connection between different WebRTC clients, or between a WebRTC client and other third-party clients.
 * Receives media from the WebRTC client and retransmits it to others,
 * or receives media from other sources and retransmits it to the WebRTC clients.
 */
class Gateway : public erizo::MediaSource {

public:
    static Gateway* createGatewayInstance(const std::string& customParam);

    virtual ~Gateway() { }

    /**
     * Adds a Publisher
     * @param source the MediaSource as the Publisher
     * @param participantId the id for the publisher
     * @param videoResolution the video resolution
     * @return true if the publisher is added successfully
     */
    virtual bool addPublisher(erizo::MediaSource*, const std::string& participantId, const std::string& videoResolution) = 0;
    /**
     * Adds a Publisher
     * @param source the MediaSource as the Publisher
     * @param participantId the id for the publisher
     * @return true if the publisher is added successfully
     */
    virtual bool addPublisher(erizo::MediaSource*, const std::string& participantId) = 0;
    /**
     * Removes the Publisher
     * @param participantId the id for the publisher
     */
    virtual void removePublisher(const std::string& participantId) = 0;
    /**
     * Adds a subscriber
     * @param sink the MediaSink of the subscriber
     * @param id the peer id for the subscriber
     * @param options additional options for the subscriber
     */
    virtual void addSubscriber(erizo::MediaSink*, const std::string& id, const std::string& options) = 0;
    /**
     * Eliminates the subscriber given its peer id
     * @param id the peer id for the subscriber
     */
    virtual void removeSubscriber(const std::string& id) = 0;

    /**
     * Set async event handle
     * @param handle
     */
    virtual void setEventRegistry(EventRegistry*) = 0;

    /**
     * Pass 'custom' messages such as text messages
     * @param message
     */
    virtual void customMessage(const std::string& message) = 0;
    /**
     * Retrieve the Gateway statistics including the number of packets received
     * and lost, the average RTT between Gateway and browser, etc.
     * @return the JSON formatted string of the statistics.
     */
    virtual std::string retrieveStatistics() = 0;

    /**
     * Start subscribing audio or video
     * @param id the subscriber id
     * @param isAudio subscribe audio or video
     */
    virtual void subscribeStream(const std::string& id, bool isAudio) = 0;
    /**
     * Stop subscribing audio or video
     * @param id the subscriber id
     * @param isAudio unsubscribe audio or video
     */
    virtual void unsubscribeStream(const std::string& id, bool isAudio) = 0;
    /**
     * Start publishing audio or video
     * @param id the publisher id
     * @param isAudio publish audio or video
     */
    virtual void publishStream(const std::string& id, bool isAudio) = 0;
    /**
     * Stop publishing audio or video
     * @param id the publisher id
     * @param isAudio unpublish audio or video
     */
    virtual void unpublishStream(const std::string& id, bool isAudio) = 0;
    /**
     * Retrieve the video frame provider from the Gateway
     * @return the video frame provider.
     */
    virtual FrameProvider* getVideoFrameProvider() = 0;
    /**
     * Retrieve the audio frame provider from the Gateway
     * @return the audio frame provider.
     */
    virtual FrameProvider* getAudioFrameProvider() = 0;
    /**
     * Sets the bitrate of a video stream
     * @param id the publisher id
     * @param id the suggested new bitrate
     * @return true if the bitrate setting request is accepted
     */
    virtual bool setVideoBitrate(const std::string& id, uint32_t kbps) { return false; }
};

} /* namespace woogeen_base */
#endif /* Gateway_h */
