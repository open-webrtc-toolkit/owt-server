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

#ifndef Mixer_h
#define Mixer_h

#include "media/AudioMixer.h"
#include "media/MediaRecorder.h"
#include "media/RTSPMuxer.h"
#include "media/VideoMixer.h"
#include "MixerInterface.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <Gateway.h>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>
#include <MediaEnabling.h>
#include <utility> // std::pair

namespace mcu {

/**
 * A Mixer refers to a media mixer acting as a woogeen_base::Gateway.
 * It receives media from several sources through the WebRTCGateways, mixed them into one stream and retransmits
 * it to every subscriber.
 */
class Mixer : public MixerInterface, public erizo::FeedbackSink, public erizo::RTPDataReceiver, AudioMixerVADCallback {
    DECLARE_LOGGER();

public:
    Mixer(boost::property_tree::ptree& videoConfig);
    virtual ~Mixer();

    // Implements MixerInterface.
    bool addPublisher(erizo::MediaSource*, const std::string& id);
    bool addPublisher(erizo::MediaSource* source, const std::string& id, const std::string& videoResolution) { return addPublisher(source, id); }
    void removePublisher(const std::string& id);

    void addSubscriber(erizo::MediaSink*, const std::string& id);
    void removeSubscriber(const std::string& id);

    // TODO: implement the below interfaces to support async event notification
    // from the native layer to the JS (controller) layer.
    void setupAsyncEvent(const std::string& event, woogeen_base::EventRegistry*) { }
    void destroyAsyncEvents() { }

    void customMessage(const std::string& message) { }

    // TODO: implement the below interface to support Mixer statistics retrieval,
    // which can be used to monitor the mixer.
    std::string retrieveStatistics() { return ""; }

    void subscribeStream(const std::string& id, bool isAudio);
    void unsubscribeStream(const std::string& id, bool isAudio);
    // TODO: implement the below interfaces to support media play/pause.
    void publishStream(const std::string& id, bool isAudio) { }
    void unpublishStream(const std::string& id, bool isAudio) { }

    bool addExternalOutput(const std::string& configParam, woogeen_base::EventRegistry* callback = nullptr);
    bool removeExternalOutput(const std::string& outputId, bool close);

    bool setVideoBitrate(const std::string& id, uint32_t kbps);

    // TODO: implement the below interfaces to support video layout related
    // information retrieval and setting.
    std::string getRegion(const std::string& participantId);
    bool setRegion(const std::string& participantId, const std::string& regionId);

    // TODO: we currently don't accept the below requests to the mixer as a MediaSource.
    // We may reconsider it later to see if it's valuable to support them.
    int sendFirPacket() { return -1; }
    int setVideoCodec(const std::string& codecName, unsigned int clockRate) { return -1; }
    int setAudioCodec(const std::string& codecName, unsigned int clockRate) { return -1; }

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements RTPDataReceiver.
    void receiveRtpData(char*, int len, erizo::DataType, uint32_t channelId);

    // Implements AudioMixerVADCallback
    void onPositiveAudioSources(std::vector<uint32_t>& sources);

protected:
    boost::shared_ptr<VideoMixer> m_videoMixer;
    boost::shared_ptr<AudioMixer> m_audioMixer;
    boost::shared_mutex m_avBindingsMutex;
    std::map<uint32_t/*audio source*/, uint32_t/*video source*/> m_avBindings;

private:
    bool init(boost::property_tree::ptree& videoConfig);
    /**
     * Closes all the subscribers and the publisher, the object is useless after this
     */
    void closeAll();

    boost::shared_mutex m_subscriberMutex;
    std::map<std::string, std::pair<boost::shared_ptr<erizo::MediaSink>, woogeen_base::MediaEnabling>> m_subscribers;
    std::map<std::string, erizo::MediaSource*> m_publishers;
};

} /* namespace mcu */
#endif /* Mixer_h */
