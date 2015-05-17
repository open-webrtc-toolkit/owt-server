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

#ifndef Gateway_h
#define Gateway_h

#include "EventRegistry.h"
#include "MediaSourceConsumer.h"

#include <Compiler.h>
#include <MediaDefinitions.h>

namespace woogeen_base {

/**
 * Represents connection between different WebRTC clients, or between a WebRTC client and other third-party clients.
 * Receives media from the WebRTC client and retransmits it to others,
 * or receives media from other sources and retransmits it to the WebRTC clients.
 */
class Gateway : public MediaSourceConsumer {

public:
    DLL_PUBLIC static Gateway* createGatewayInstance(const std::string& customParam);

    DLL_PUBLIC virtual ~Gateway() { }

    /**
     * Sets the Publisher
     * @param source the MediaSource as the Publisher
     * @param videoResolution the video resolution
     * @return true if the publisher is added successfully
     */
    DLL_PUBLIC virtual bool setPublisher(erizo::MediaSource*, const std::string& participantId, const std::string& videoResolution) = 0;
    /**
     * Sets the Publisher
     * @param source the MediaSource as the Publisher
     * @return true if the publisher is added successfully
     */
    DLL_PUBLIC virtual bool setPublisher(erizo::MediaSource*, const std::string& participantId) = 0;
    /**
     * Unsets the Publisher
     */
    DLL_PUBLIC virtual void unsetPublisher() = 0;
    /**
     * Sets the subscriber
     * @param sink the MediaSink of the subscriber
     * @param id the id for the subscriber
     */
    DLL_PUBLIC virtual void addSubscriber(erizo::MediaSink*, const std::string& id) = 0;
    /**
     * Eliminates the subscriber given its peer id
     * @param id the id for the subscriber
     */
    DLL_PUBLIC virtual void removeSubscriber(const std::string& id) = 0;
    /**
     * Set async event handler
     * @param event name, handle
     */
    DLL_PUBLIC virtual void setupAsyncEvent(const std::string& event, woogeen_base::EventRegistry*) = 0;
    /**
     * Destroy async event handlers
     */
    DLL_PUBLIC virtual void destroyAsyncEvents() = 0;
    /**
     * Client joins a session with the specified uri
     * @param clientJoinUri
     * @return true if client joins successfully
     */
    DLL_PUBLIC virtual bool clientJoin(const std::string& clientJoinUri) = 0;
    /**
     * Pass 'custom' messages such as text messages
     * @param message
     */
    DLL_PUBLIC virtual void customMessage(const std::string& message) = 0;

    /**
     * Retrieve the Gateway statistics including the number of packets received
     * and lost, the average RTT between Gateway and browser, etc.
     * @return the JSON formatted string of the statistics.
     */
    DLL_PUBLIC virtual std::string retrieveGatewayStatistics() = 0;

    /**
     * Start subscribing audio or video
     * @param id the subscriber id
     * @param isAudio subscribe audio or video
     */
    DLL_PUBLIC virtual void subscribeStream(const std::string& id, bool isAudio) = 0;

    /**
     * Stop subscribing audio or video
     * @param id the subscriber id
     * @param isAudio unsubscrib audio or video
     */
    DLL_PUBLIC virtual void unsubscribeStream(const std::string& id, bool isAudio) = 0;

    /**
     * Start publishing audio or video
     * @param isAudio publish audio or video
     */
    DLL_PUBLIC virtual void publishStream(bool isAudio) = 0;

    /**
     * Stop publishing audio or video
     * @param isAudio unpublish audio or video
     */
    DLL_PUBLIC virtual void unpublishStream(bool isAudio) = 0;

    /**
     * Set the additional media source consumer which consumes the publisher of this Gateway
     * @param mediaSourceConsumer
     */
    DLL_PUBLIC virtual void setAdditionalSourceConsumer(MediaSourceConsumer*) = 0;

    /**
     * Sets the media recorder
     * @param file path for media recording
     * @return true if the media recorder is set successfully
     */
    DLL_PUBLIC virtual bool setRecorder(const std::string& recordPath, int snapshotInterval) { return false; }
    /**
     * Unsets the media recorder
     */
    DLL_PUBLIC virtual void unsetRecorder() {}

    /**
     * Add the media source to compose the virtual Publisher
     * @param source the MediaSource for publishing
     */
    DLL_PUBLIC virtual void addSource(erizo::MediaSource* source) {}

    virtual int32_t addSource(uint32_t id, bool isAudio, erizo::FeedbackSink*, const std::string& participantId) { return -1; }
    virtual int32_t removeSource(uint32_t id, bool isAudio) { return -1; }
};

} /* namespace woogeen_base */
#endif /* Gateway_h */
