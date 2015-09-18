/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#include "ExternalInputGateway.h"
#include "MediaMuxerFactory.h"

#include <boost/property_tree/json_parser.hpp>
#include <EncodedVideoFrameSender.h>
#include <WebRTCFeedbackProcessor.h>

using namespace erizo;

namespace mcu {

DEFINE_LOGGER(ExternalInputGateway, "mcu.ExternalInputGateway");

ExternalInputGateway::ExternalInputGateway()
    : m_incomingVideoFormat(woogeen_base::FRAME_FORMAT_UNKNOWN)
    , m_videoOutputKbps(0)
{
    m_taskRunner.reset(new woogeen_base::WebRTCTaskRunner());
    m_audioTranscoder.reset(new AudioMixer(this, nullptr));
    m_taskRunner->Start();
}

ExternalInputGateway::~ExternalInputGateway()
{
    m_taskRunner->Stop();
    closeAll();
}

int ExternalInputGateway::deliverAudioData(char* buf, int len)
{
    if (len <= 0 || !m_publisherStatus.hasAudio())
        return 0;

    {
        boost::shared_lock<boost::shared_mutex> lock(m_sinkMutex);
        if (audioSink_)
            audioSink_->deliverAudioData(buf, len);
    }

    if (!m_subscribers.empty())
        return m_audioTranscoder->deliverAudioData(buf, len);

    return 0;
}

int ExternalInputGateway::deliverVideoData(char* buf, int len)
{
    if (len <= 0 || !m_publisherStatus.hasVideo())
        return 0;

    {
        boost::shared_lock<boost::shared_mutex> lock(m_sinkMutex);
        if (videoSink_)
            videoSink_->deliverVideoData(buf, len);
    }

    boost::shared_lock<boost::shared_mutex> transcoderLock(m_videoTranscoderMutex);
    std::vector<boost::shared_ptr<woogeen_base::VideoFrameTranscoder>>::iterator it = m_videoTranscoders.begin();
    for (; it != m_videoTranscoders.end(); ++it) {
        woogeen_base::Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = m_incomingVideoFormat;
        frame.payload = reinterpret_cast<uint8_t*>(buf);
        frame.length = len;
        (*it)->onFrame(frame);
    }

    return len;
}

int ExternalInputGateway::deliverFeedback(char* buf, int len)
{
    // TODO: For now we just send the feedback to all of the output processors.
    // The output processor will filter out the feedback which does not belong
    // to it. In the future we may do the filtering at a higher level?
    boost::shared_lock<boost::shared_mutex> lock(m_videoOutputMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameSender>>::iterator it = m_videoOutputs.begin();
    for (; it != m_videoOutputs.end(); ++it) {
        FeedbackSink* feedbackSink = it->second->feedbackSink();
        if (feedbackSink)
            feedbackSink->deliverFeedback(buf, len);
    }

    m_audioTranscoder->deliverFeedback(buf, len);

    return len;
}

void ExternalInputGateway::receiveRtpData(char* buf, int len, DataType type, uint32_t streamId)
{
    if (m_subscribers.empty() || len <= 0)
        return;

    uint32_t ssrc = 0;
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT)
        ssrc = chead->getSSRC();
    else {
        RTPHeader* head = reinterpret_cast<RTPHeader*>(buf);
        ssrc = head->getSSRC();
    }

    std::map<std::string, std::pair<boost::shared_ptr<MediaSink>, woogeen_base::MediaEnabling>>::iterator it;
    boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
    switch (type) {
    case AUDIO: {
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if (it->second.second.hasAudio()) {
                MediaSink* sink = it->second.first.get();
                if (sink && sink->getAudioSinkSSRC() == ssrc)
                    sink->deliverAudioData(buf, len);
            }
        }
        break;
    }
    case VIDEO: {
        for (it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
            if (it->second.second.hasVideo()) {
                MediaSink* sink = it->second.first.get();
                if (sink && sink->getVideoSinkSSRC() == ssrc)
                    sink->deliverVideoData(buf, len);
            }
        }
        break;
    }
    default:
        break;
    }
}

bool ExternalInputGateway::addPublisher(MediaSource* publisher, const std::string& id)
{
    if (m_publisher) {
        ELOG_WARN("Publisher already exists: %p, id %s, ignoring the new set request (%p, %s)",
            m_publisher.get(), m_participantId.c_str(), publisher, id.c_str());
        return false;
    }

    if (publisher->getVideoDataType() != DataContentType::ENCODED_FRAME) {
        ELOG_ERROR("Wrong kind of publisher: %p, id %s, ignoring)",
             publisher, id.c_str());
        assert(false);
        return false;
    }

    switch (publisher->getVideoPayloadType()) {
    case VP8_90000_PT:
        m_incomingVideoFormat = woogeen_base::FRAME_FORMAT_VP8;
        break;
    case H264_90000_PT:
        m_incomingVideoFormat = woogeen_base::FRAME_FORMAT_H264;
        break;
    default:
        break;
    }

    FeedbackSink* feedbackSink = publisher->getFeedbackSink();
    uint32_t audioSSRC = publisher->getAudioSourceSSRC();

    if (audioSSRC)
        m_audioTranscoder->addSource(audioSSRC, true, publisher->getAudioDataType(), publisher->getAudioPayloadType(), feedbackSink, id);

    videoSourceSSRC_ = publisher->getVideoSourceSSRC();
    audioSourceSSRC_ = audioSSRC;
    sourcefbSink_ = feedbackSink;
    videoDataType_ = publisher->getVideoDataType();
    audioDataType_ = publisher->getAudioDataType();
    videoPayloadType_ = publisher->getVideoPayloadType();
    audioPayloadType_ = publisher->getAudioPayloadType();

    publisher->setAudioSink(this);
    publisher->setVideoSink(this);

    m_participantId = id;
    m_publisher.reset(publisher);

    return true;
}

void ExternalInputGateway::removePublisher(const std::string& id)
{
    if (!m_publisher || id != m_participantId) {
        ELOG_WARN("Publisher doesn't exist; can't unset the publisher");
        return;
    }

    if (!m_subscribers.empty()) {
        ELOG_WARN("There're still %zu subscribers to current publisher; can't unset the publisher", m_subscribers.size());
        return;
    }

    {
        boost::shared_lock<boost::shared_mutex> transcoderLock(m_videoTranscoderMutex);
        std::vector<boost::shared_ptr<woogeen_base::VideoFrameTranscoder>>::iterator it = m_videoTranscoders.begin();
        for (; it != m_videoTranscoders.end(); ++it)
            (*it)->unsetInput();
    }

    m_audioTranscoder->removeSource(m_publisher->getAudioSourceSSRC(), true);

    m_publisher.reset();
}

void ExternalInputGateway::addSubscriber(MediaSink* subscriber, const std::string& id, const std::string& options)
{
    int videoPayloadType = subscriber->preferredVideoPayloadType();
    // FIXME: Now we hard code the output to be NACK enabled and FEC disabled,
    // because the video mixer now is not able to output different formatted
    // RTP packets for a single encoded stream elegantly.
    bool enableNACK = true || subscriber->acceptResentData();
    bool enableFEC = false && subscriber->acceptFEC();
    uint32_t videoSSRC = addVideoOutput(videoPayloadType, enableNACK, enableFEC);
    subscriber->setVideoSinkSSRC(videoSSRC);

    int audioPayloadType = subscriber->preferredAudioPayloadType();
    int32_t channelId = m_audioTranscoder->addOutput(id, audioPayloadType);
    subscriber->setAudioSinkSSRC(m_audioTranscoder->getSendSSRC(channelId));

    ELOG_DEBUG("Adding subscriber to %u(a), %u(v)", m_audioTranscoder->getSendSSRC(channelId), videoSSRC);

    FeedbackSource* fbsource = subscriber->getFeedbackSource();
    if (fbsource) {
        ELOG_DEBUG("adding fbsource");
        fbsource->setFeedbackSink(this);
    }

    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    m_subscribers[id] = std::pair<boost::shared_ptr<MediaSink>,
                            woogeen_base::MediaEnabling>(boost::shared_ptr<MediaSink>(subscriber), woogeen_base::MediaEnabling());
}

void ExternalInputGateway::removeSubscriber(const std::string& id)
{
    ELOG_DEBUG("Removing subscriber: id is %s", id.c_str());

    std::vector<boost::shared_ptr<MediaSink>> removedSubscribers;
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<std::string, std::pair<boost::shared_ptr<MediaSink>,
        woogeen_base::MediaEnabling>>::iterator it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        boost::shared_ptr<MediaSink>& subscriber = it->second.first;
        if (subscriber) {
            FeedbackSource* fbsource = subscriber->getFeedbackSource();
            if (fbsource)
                fbsource->setFeedbackSink(nullptr);
            removedSubscribers.push_back(subscriber);
        }
        m_subscribers.erase(it);
    }
    lock.unlock();
}

void ExternalInputGateway::subscribeStream(const std::string& id, bool isAudio)
{
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<std::string, std::pair<boost::shared_ptr<MediaSink>,
        woogeen_base::MediaEnabling>>::iterator it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        ELOG_DEBUG("subscriber %s - %s", id.c_str(), isAudio ? "playAudio" : "playVideo");
        isAudio ? it->second.second.enableAudio() : it->second.second.enableVideo();
    }
    lock.unlock();
}

void ExternalInputGateway::unsubscribeStream(const std::string& id, bool isAudio)
{
    boost::unique_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<std::string, std::pair<boost::shared_ptr<MediaSink>,
        woogeen_base::MediaEnabling>>::iterator it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        ELOG_DEBUG("subscriber %s - %s", id.c_str(), isAudio ? "pauseAudio" : "pauseVideo");
        isAudio ? it->second.second.disableAudio() : it->second.second.disableVideo();
    }
    lock.unlock();
}

void ExternalInputGateway::publishStream(const std::string& id, bool isAudio)
{
    isAudio ? m_publisherStatus.enableAudio() : m_publisherStatus.enableVideo();
}

void ExternalInputGateway::unpublishStream(const std::string& id, bool isAudio)
{
    isAudio ? m_publisherStatus.disableAudio() : m_publisherStatus.disableVideo();
}

void ExternalInputGateway::setAudioSink(MediaSink* sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_sinkMutex);
    audioSink_ = sink;
}

void ExternalInputGateway::setVideoSink(MediaSink* sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_sinkMutex);
    videoSink_ = sink;
}

int ExternalInputGateway::sendFirPacket()
{
    return m_publisher ? m_publisher->sendFirPacket() : -1;
}

int ExternalInputGateway::setVideoCodec(const std::string& codecName, unsigned int clockRate)
{
    return m_publisher ? m_publisher->setVideoCodec(codecName, clockRate) : -1;
}

int ExternalInputGateway::setAudioCodec(const std::string& codecName, unsigned int clockRate)
{
    return m_publisher ? m_publisher->setAudioCodec(codecName, clockRate) : -1;
}

woogeen_base::FrameProvider* ExternalInputGateway::getVideoFrameProvider()
{
    return this;
}

woogeen_base::FrameProvider* ExternalInputGateway::getAudioFrameProvider()
{
    return m_audioTranscoder.get();
}

void ExternalInputGateway::closeAll()
{
    ELOG_DEBUG("closeAll");

    std::vector<boost::shared_ptr<MediaSink>> removedSubscribers;
    boost::unique_lock<boost::shared_mutex> subscriberLock(m_subscriberMutex);
    std::map<std::string, std::pair<boost::shared_ptr<MediaSink>,
        woogeen_base::MediaEnabling>>::iterator subscriberItor = m_subscribers.begin();
    while (subscriberItor != m_subscribers.end()) {
        boost::shared_ptr<MediaSink>& subscriber = subscriberItor->second.first;
        if (subscriber) {
            FeedbackSource* fbsource = subscriber->getFeedbackSource();
            if (fbsource)
                fbsource->setFeedbackSink(nullptr);
            removedSubscribers.push_back(subscriber);
        }
        m_subscribers.erase(subscriberItor++);
    }
    m_subscribers.clear();
    subscriberLock.unlock();

    removePublisher(m_participantId);
}

uint32_t ExternalInputGateway::addVideoOutput(int payloadType, bool nack, bool fec)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_videoOutputMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameSender>>::iterator it = m_videoOutputs.find(payloadType);
    if (it != m_videoOutputs.end()) {
        it->second->startSend(nack, fec);
        return it->second->sendSSRC(nack, fec);
    }

    woogeen_base::FrameFormat outputFormat = woogeen_base::FRAME_FORMAT_UNKNOWN;
    switch (payloadType) {
    case VP8_90000_PT:
        outputFormat = woogeen_base::FRAME_FORMAT_VP8;
        break;
    case H264_90000_PT:
        outputFormat = woogeen_base::FRAME_FORMAT_H264;
        break;
    default:
        return 0;
    }

    // TODO: Create the transcoders on-demand, i.e., when
    // there're subscribers which don't support the incoming codec.
    // In such case we should use the publisher as the frame provider of the VideoFrameSender,
    // but currently the publisher doesn't implement the VideoFrameProvider interface.
    boost::shared_ptr<woogeen_base::VideoFrameTranscoder> videoTranscoder(new woogeen_base::VideoFrameTranscoder(m_taskRunner));
    videoTranscoder->setInput(m_incomingVideoFormat, nullptr);

    woogeen_base::WebRTCTransport<VIDEO>* transport = new woogeen_base::WebRTCTransport<VIDEO>(this, nullptr);
    // Fetch video size.
    // TODO: The size should be identical to the published video size.
    woogeen_base::VideoFrameSender* output = new woogeen_base::EncodedVideoFrameSender(videoTranscoder, outputFormat, m_videoOutputKbps, transport, m_taskRunner, 1280, 720);

    {
        boost::unique_lock<boost::shared_mutex> transcoderLock(m_videoTranscoderMutex);
        m_videoTranscoders.push_back(videoTranscoder);
    }

    output->startSend(nack, fec);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_videoOutputs[payloadType].reset(output);

    return output->sendSSRC(nack, fec);
}

// Video ONLY here for the frame consumer
int32_t ExternalInputGateway::addFrameConsumer(const std::string&/*unused*/, woogeen_base::FrameFormat format, woogeen_base::FrameConsumer* frameConsumer, const woogeen_base::MediaSpecInfo&)
{
    int payloadType = INVALID_PT;
    switch (format) {
    case woogeen_base::FRAME_FORMAT_VP8:
        payloadType = VP8_90000_PT;
        break;
    case woogeen_base::FRAME_FORMAT_H264:
        payloadType = H264_90000_PT;
        break;
    default:
        break;
    }

    // May delay this to the VideoFrameSender???
    addVideoOutput(payloadType, false, false);

    boost::shared_lock<boost::shared_mutex> lock(m_videoOutputMutex);
    int outputId = -1;
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameSender>>::iterator it = m_videoOutputs.find(payloadType);
    if (it != m_videoOutputs.end()) {
        it->second->registerPreSendFrameCallback(frameConsumer);

        // Request an IFrame explicitly, because the recorder doesn't support active I-Frame requests.
        woogeen_base::IntraFrameCallback* iFrameCallback = it->second->iFrameCallback();
        if (iFrameCallback)
            iFrameCallback->handleIntraFrameRequest();

        outputId = it->second->streamId();
    }

    return outputId;
}

// Video ONLY here for the frame consumer
void ExternalInputGateway::removeFrameConsumer(int32_t id)
{
    // May delay this to the VideoFrameSender???
    boost::shared_lock<boost::shared_mutex> lock(m_videoOutputMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameSender>>::iterator it = m_videoOutputs.begin();
    for (; it != m_videoOutputs.end(); ++it) {
        if (id == it->second->streamId()) {
            it->second->deRegisterPreSendFrameCallback();
            break;
        }
    }
}

} /* namespace mcu */
