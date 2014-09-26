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

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>
#include <vector>

namespace woogeen_base {
class ProtectedRTPReceiver;
class ProtectedRTPSender;
class WoogeenVideoTransport;
class WoogeenAudioTransport;
}

namespace mcu {

class ACMOutputProcessor;
class BufferManager;
class VCMOutputProcessor;

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
     * @param puber The MediaSource as the Publisher
     */
    DLL_PUBLIC void addPublisher(erizo::MediaSource* puber);
    /**
     * Sets the subscriber
     * @param suber The MediaSink as the subscriber
     * @param peerId An unique Id for the subscriber
     */
    DLL_PUBLIC void addSubscriber(erizo::MediaSink* suber, const std::string& peerId);
    /**
     * Eliminates the subscriber
     * @param puber
     */
    DLL_PUBLIC void removeSubscriber(const std::string& peerId);

    DLL_PUBLIC void removePublisher(erizo::MediaSource* puber);

    /**
     * called by WebRtcConnections' onTransportData. This MCUMixer
     * will be set as the MediaSink of all the WebRtcConnections in the
     * same room
     */
    virtual int deliverAudioData(char* buf, int len, erizo::MediaSource* from);
    /**
     * called by WebRtcConnections' onTransportData. This MCUMixer
     * will be set as the MediaSink of all the WebRtcConnections in the
     * same room
     */
    virtual int deliverVideoData(char* buf, int len, erizo::MediaSource* from);

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

    int assignSlot(erizo::MediaSource* pub) {
        for (uint32_t i = 0; i < puberSlotMap_.size(); i++) {
            if (puberSlotMap_[i] == NULL) {
                puberSlotMap_[i] = pub;
                return i;
            }
        }
        puberSlotMap_.push_back(pub);
        return puberSlotMap_.size() - 1;
    }

    int maxSlot() {
        return puberSlotMap_.size();
    }

    // find the slot number for the corresponding puber
    // return -1 if not found
    int getSlot(erizo::MediaSource* pub) {
        for (uint32_t i = 0; i < puberSlotMap_.size(); i++) {
            if (puberSlotMap_[i] == pub)
                return i;
        }
        return -1;
    }

    boost::mutex myMonitor_;
    std::map<std::string, boost::shared_ptr<MediaSink>> subscribers_;
    std::map<erizo::MediaSource*, woogeen_base::ProtectedRTPReceiver*> publishers_;
    std::vector<erizo::MediaSource*> puberSlotMap_;    // each publisher will be allocated one index
    boost::shared_ptr<VCMOutputProcessor> vop_;
    boost::shared_ptr<ACMOutputProcessor> aop_;
    boost::shared_ptr<woogeen_base::WoogeenVideoTransport> videoTransport_;
    boost::shared_ptr<woogeen_base::WoogeenAudioTransport> audioTransport_;
    boost::shared_ptr<BufferManager> bufferManager_;
    boost::shared_ptr<erizo::FeedbackSink> feedback_;
};

} /* namespace mcu */
#endif /* MCUMixer_h */
