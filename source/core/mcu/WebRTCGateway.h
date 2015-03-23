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

#ifndef WebRTCGateway_h
#define WebRTCGateway_h

#include <boost/thread/shared_mutex.hpp>
#include <Gateway.h>
#include <JobTimer.h>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>
#include <ProtectedRTPReceiver.h>
#include <string>
#include <WebRTCFeedbackProcessor.h>

namespace mcu {

class RTPDataReceiveBridge : public erizo::RTPDataReceiver {
public:
    RTPDataReceiveBridge(RTPDataReceiver* destination)
        : m_destination(destination)
    {
    }

    void receiveRtpData(char* buf, int len, erizo::DataType type, uint32_t streamId)
    {
        m_destination->receiveRtpData(buf, len, type, streamId);
    }

private:
    RTPDataReceiver* m_destination;
};

class IFrameRequestBridge : public woogeen_base::IntraFrameCallback {
public:
    IFrameRequestBridge(IntraFrameCallback* destination)
        : m_destination(destination)
    {
    }

    void handleIntraFrameRequest()
    {
        m_destination->handleIntraFrameRequest();
    }

private:
    IntraFrameCallback* m_destination;
};

/**
 * Represents a gateway between different WebRTC clients.
 * Receives media from one WebRTC client and dispatches it to others,
 */
class WebRTCGateway : public woogeen_base::Gateway, public woogeen_base::JobTimerListener, public woogeen_base::IntraFrameCallback, public erizo::MediaSink, public erizo::RTPDataReceiver {
    DECLARE_LOGGER();

public:
    WebRTCGateway();
    virtual ~WebRTCGateway();

    // Implements Gateway.
    bool setPublisher(erizo::MediaSource*, const std::string& participantId);
    bool setPublisher(erizo::MediaSource* source, const std::string& participantId, const std::string& videoResolution) { return setPublisher(source, participantId); }
    void unsetPublisher();

    void addSubscriber(erizo::MediaSink*, const std::string& id);
    void removeSubscriber(const std::string& id);

    void setupAsyncEvent(const std::string& event, woogeen_base::EventRegistry* handler)
    {
        m_asyncHandler.reset(handler);
        m_asyncHandler->notify("");
    }
    void destroyAsyncEvents() { m_asyncHandler.reset(); }

    bool clientJoin(const std::string& clientJoinUri) { return true; }
    void customMessage(const std::string& message) { }

    std::string retrieveGatewayStatistics() { return ""; }

    void subscribeStream(const std::string& id, bool isAudio) { }
    void unsubscribeStream(const std::string& id, bool isAudio) { }
    void publishStream(bool isAudio) { }
    void unpublishStream(bool isAudio) { }

    void setAdditionalSourceConsumer(woogeen_base::MediaSourceConsumer*);

    // Implements JobTimerListener.
    void onTimeout();

    // Implements IntraFrameCallback.
    void handleIntraFrameRequest();

    // Implements MediaSink.
    int deliverAudioData(char*, int len);
    int deliverVideoData(char*, int len);

    // Implements RTPDataReceiver.
    void receiveRtpData(char*, int len, erizo::DataType, uint32_t streamId);

private:
    void closeAll();

    std::string m_participantId;
    boost::shared_ptr<erizo::MediaSource> m_publisher;
    boost::shared_mutex m_subscriberMutex;
    std::map<std::string, boost::shared_ptr<erizo::MediaSink>> m_subscribers;

    woogeen_base::MediaSourceConsumer* m_mixer;

    // TODO: Use it for async event notification from the worker thread to the main node thread.
    boost::shared_ptr<woogeen_base::EventRegistry> m_asyncHandler;

    boost::shared_ptr<IFrameRequestBridge> m_iFrameRequestBridge;
    boost::shared_ptr<woogeen_base::WebRTCFeedbackProcessor> m_feedback;
    boost::shared_ptr<RTPDataReceiveBridge> m_postProcessedMediaReceiver;
    boost::scoped_ptr<woogeen_base::ProtectedRTPReceiver> m_videoReceiver;
    boost::scoped_ptr<woogeen_base::ProtectedRTPReceiver> m_audioReceiver;
    boost::scoped_ptr<woogeen_base::JobTimer> m_feedbackTimer;
    uint32_t m_pendingIFrameRequests;
};

} /* namespace mcu */
#endif /* WebRTCGateway_h */
