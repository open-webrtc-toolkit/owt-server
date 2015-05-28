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

#ifndef OutOfProcessMixerProxy_h
#define OutOfProcessMixerProxy_h

#include "MixerInterface.h"

#include <boost/scoped_ptr.hpp>
#include <MediaDefinitions.h>
#include <MediaSourceConsumer.h>
#include <RawTransport.h>

namespace mcu {

class AudioDataWriter : public woogeen_base::RawTransportListener {
public:
    AudioDataWriter();
    ~AudioDataWriter();

    // Implements RawTransportListener.
    // TODO: onTransportData should be implemented to handle the feedback.
    void onTransportData(char*, int len, woogeen_base::Protocol) { }
    void onTransportError(woogeen_base::Protocol) { }
    void onTransportConnected(woogeen_base::Protocol);

    int32_t write(char*, int len);

private:
    boost::scoped_ptr<woogeen_base::RawTransport> m_transport;
};

class VideoDataWriter : public woogeen_base::RawTransportListener {
public:
    VideoDataWriter();
    ~VideoDataWriter();

    // Implements RawTransportListener.
    // TODO: onTransportData should be implemented to handle the feedback.
    void onTransportData(char*, int len, woogeen_base::Protocol) { }
    void onTransportError(woogeen_base::Protocol) { }
    void onTransportConnected(woogeen_base::Protocol);

    int32_t write(char*, int len);

private:
    boost::scoped_ptr<woogeen_base::RawTransport> m_transport;
};

class MessageWriter : public woogeen_base::RawTransportListener {
public:
    MessageWriter();
    ~MessageWriter();

    // Implements RawTransportListener.
    // TODO: onTransportData should be implemented to handle the feedback.
    void onTransportData(char*, int len, woogeen_base::Protocol) { }
    void onTransportError(woogeen_base::Protocol) { }
    void onTransportConnected(woogeen_base::Protocol);

    int32_t write(char*, int len);

private:
    boost::scoped_ptr<woogeen_base::RawTransport> m_transport;
};

class OutOfProcessMixerProxy : public MixerInterface, public erizo::MediaSink {
public:
    OutOfProcessMixerProxy();
    ~OutOfProcessMixerProxy();

    // Implements MixerInterface.
    bool addPublisher(erizo::MediaSource*, const std::string& id);
    bool addPublisher(erizo::MediaSource* source, const std::string& id, const std::string& videoResolution) { return addPublisher(source, id); }
    void removePublisher(const std::string& id);

    void addSubscriber(erizo::MediaSink*, const std::string& id) { }
    void removeSubscriber(const std::string& id) { }

    // TODO: reconsider the role and usage of this class.
    void setupAsyncEvent(const std::string& event, woogeen_base::EventRegistry*) { }
    void destroyAsyncEvents() { }

    void customMessage(const std::string& message) { }

    std::string retrieveStatistics() { return ""; }

    void subscribeStream(const std::string& id, bool isAudio) { }
    void unsubscribeStream(const std::string& id, bool isAudio) { }
    void publishStream(const std::string& id, bool isAudio) { }
    void unpublishStream(const std::string& id, bool isAudio) { }

    bool addExternalOutput(const std::string& configParam) { return false; }
    bool removeExternalOutput(const std::string& outputId) { return false; }

    int sendFirPacket() { return -1; }
    int setVideoCodec(const std::string& codecName, unsigned int clockRate) { return -1; }
    int setAudioCodec(const std::string& codecName, unsigned int clockRate) { return -1; }

    std::string getRegion(const std::string& participantId) { return ""; }
    bool setRegion(const std::string& participantId, const std::string& regionId) { return false; }

    // Implement MediaSink.
    int deliverAudioData(char* buf, int len);
    int deliverVideoData(char* buf, int len);

private:
    boost::scoped_ptr<AudioDataWriter> m_audioOutput;
    boost::scoped_ptr<VideoDataWriter> m_videoOutput;
    boost::scoped_ptr<MessageWriter> m_messageOutput;
    std::map<std::string, erizo::MediaSource*> m_publishers;
};

} /* namespace mcu */
#endif /* OutOfProcessMixerProxy_h */
