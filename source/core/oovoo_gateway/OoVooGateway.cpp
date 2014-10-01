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

#include "OoVooGateway.h"

#include "OoVooConnection.h"
#include <media/ExternalOutput.h>
#include <rtputils.h>

using namespace erizo;
using namespace woogeen_base;

Gateway* Gateway::createGatewayInstance(const std::string& customParams)
{
    return new oovoo_gateway::OoVooGateway(customParams);
}

namespace oovoo_gateway {

DEFINE_LOGGER(OoVooGateway, "ooVoo.OoVooGateway");

static const uint32_t FEEDBACK_TIMER_INTERVAL = 1; // seconds
static const uint32_t PERIODIC_FIR_MULTIPLIER = 3; // FEEDBACK_TIMER_INTERVALs
static const uint32_t MINIMUM_IFRAME_INTERVAL = 30; // frames

OoVooGateway::OoVooGateway(const std::string& ooVooCustomParam)
    : m_feedbackSink(nullptr)
    , m_isClientLeaving(false)
    , m_webRTCClientId(0)
    , m_webRTCVideoStreamId(NOT_A_OOVOO_STREAM_ID)
    , m_webRTCAudioStreamId(NOT_A_OOVOO_STREAM_ID)
    , m_webRTCVideoResolution(VideoResolutionType::unknown)
    , m_videoCodec(VideoCoderType::unknown)
    , m_audioCodec(AudioCoderType::unknown)
    , m_webRTCPFramesDelivered(0)
    , m_timesRemainingForFIR(PERIODIC_FIR_MULTIPLIER)
    , m_averageRTT(0)
{
    m_outboundStreamProcessor.reset(new OoVooOutboundStreamProcessor());
    m_outboundStreamProcessor->init(this);
    m_inboundStreamProcessor.reset(new OoVooInboundStreamProcessor());
    m_inboundStreamProcessor->init();
    m_ooVoo.reset(OoVooProtocolStack::CreateOoVooProtocolStack(this, ooVooCustomParam));
    m_ooVooTimer.reset(new OoVooJobTimer(m_ooVoo.get()));
    m_ooVoo->JobTimerCreated(m_ooVooTimer.get());
}

OoVooGateway::~OoVooGateway()
{
    ELOG_DEBUG("OoVooGateway destructor");
    closeWebRTCClient();
}

// Firefox does not send SSRC in SDP, but we need to map its SSRC
// to ooVoo streamId before we can forward its stream to ooVoo.
// So for Firefox we'll notify ooVoo outboundStreamCreate lazily when
// the first RTP packet which contains SSRC from Firefox is received.

void OoVooGateway::audioReady()
{
    ELOG_DEBUG("ooVoo -> outboundStreamCreate: user %u, AUDIO", m_webRTCClientId);
    if (!m_isClientLeaving)
        m_ooVoo->outboundStreamCreate(m_webRTCClientId, true, unknown);
}

void OoVooGateway::videoReady()
{
    ELOG_DEBUG("ooVoo -> outboundStreamCreate: user %u, VIDEO", m_webRTCClientId);
    if (!m_isClientLeaving)
        m_ooVoo->outboundStreamCreate(m_webRTCClientId, false, m_webRTCVideoResolution);
}

// The WebRTC connection audio receive thread
int OoVooGateway::deliverAudioData(char* buf, int len, MediaSource* from)
{
    if (len <= 0)
        return 0;

    return m_audioReceiver->deliverAudioData(buf, len, from);
}

// The WebRTC connection video receive thread
int OoVooGateway::deliverVideoData(char* buf, int len, MediaSource* from)
{
    if (len <= 0)
        return 0;

    return m_videoReceiver->deliverVideoData(buf, len, from);
}

bool OoVooGateway::acceptEncapsulatedRTPData()
{
    return m_audioReceiver && m_audioReceiver->acceptEncapsulatedRTPData()
        && m_videoReceiver && m_videoReceiver->acceptEncapsulatedRTPData();
}

bool OoVooGateway::acceptFEC()
{
    return m_audioReceiver && m_audioReceiver->acceptFEC()
        && m_videoReceiver && m_videoReceiver->acceptFEC();
}

bool OoVooGateway::acceptResentData()
{
    return m_audioReceiver && m_audioReceiver->acceptResentData()
        && m_videoReceiver && m_videoReceiver->acceptResentData();
}

// The WebRTC connection receive threads (audio and video)
void OoVooGateway::receiveOoVooData(const mediaPacket_t& packet)
{
    ELOG_TRACE("Sending packet of %s stream %u to ooVoo",
        packet.isAudio ? "AUDIO" : "VIDEO", packet.streamId);

    if (!m_isClientLeaving)
        m_ooVoo->dataDeliver(const_cast<mediaPacket_t*>(&packet));

    if (!packet.isAudio && packet.fragmentId == 0) {
        if (packet.isKey) {
            // Wrap the counter once a key frame is received.
            m_webRTCPFramesDelivered = 0;
        } else
            ++m_webRTCPFramesDelivered;
    }
}

// The WebRTC connection receive threads (audio and video)
void OoVooGateway::receiveOoVooSyncMsg(uint64_t ntp, uint64_t rtp, bool isAudio, uint32_t streamId)
{
    ELOG_TRACE("Sending Sync message of %s stream %u to ooVoo",
        isAudio ? "AUDIO" : "VIDEO", streamId);

    if (!m_isClientLeaving)
        m_ooVoo->syncMsg(ntp, rtp, isAudio);
}

// The ooVoo connection work thread
void OoVooGateway::receiveRtpData(char* rtpdata, int len, DataType type, uint32_t streamId)
{
    // The ooVoo stream data is supposed to be received by the Gateway
    // only after the WebRTC client tells ooVoo it's ready.

    // TODO: Add a new interface (for RTPDataReceiver) to receive RTCP data.
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(rtpdata);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);

    std::map<uint32_t, std::vector<boost::shared_ptr<MediaSink>>>::iterator it;

    if (streamId == static_cast<uint32_t>(MIXED_OOVOO_AUDIO_STREAM_ID)) {
        assert(type == AUDIO);
        boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
        it = m_subscribers.begin();
        // for (; it != m_subscribers.end(); ++it) {
        if (it != m_subscribers.end()) {
            if (packetType == RTCP_Sender_PT) { // Sender Report
                ELOG_TRACE("Sending audio sender report to the WebRTC subscriber to %u", (*it).first);
            } else {
                ELOG_TRACE("Sending mixed ooVoo audio stream to the WebRTC subscriber to %u", (*it).first);
            }
            std::vector<boost::shared_ptr<MediaSink>>& sub = (*it).second;
            for (uint32_t i = 0; i < sub.size(); ++i)
                sub[i]->deliverAudioData(rtpdata, len);
            ++it;
        }

        for (; it != m_subscribers.end(); ++it) {
            std::vector<boost::shared_ptr<MediaSink>>& sub = (*it).second;

            if (packetType == RTCP_Sender_PT) { // Sender Report
                ELOG_TRACE("Sending audio sender report to the WebRTC subscriber to %u", (*it).first);
                for (uint32_t i = 0; i < sub.size(); ++i)
                    sub[i]->deliverAudioData(rtpdata, len);
            } else {
                RTPHeader* head = reinterpret_cast<RTPHeader*>(rtpdata);
                uint32_t headerLength = head->getHeaderLength();
                char buf[MAX_DATA_PACKET_SIZE];
                memcpy(buf, rtpdata, headerLength);
                uint32_t fakedPacketLength = headerLength;
                // Faked "SILENT" audio for G711 and OPUS.
                switch (m_audioCodec) {
                case AudioCoderType::opus:
                    buf[fakedPacketLength++] = 0xD8;
                    buf[fakedPacketLength++] = 0xFF;
                    buf[fakedPacketLength++] = 0xFE;
                    break;
                case AudioCoderType::g711:
                    memset(buf + headerLength, 0xFF, len - headerLength);
                    fakedPacketLength = len;
                    break;
                case AudioCoderType::unknown:
                default:
                    break;
                }
                ELOG_TRACE("Sending FAKED ooVoo audio stream to the WebRTC subscriber to %u", (*it).first);
                for (uint32_t i = 0; i < sub.size(); ++i)
                    sub[i]->deliverAudioData(buf, fakedPacketLength);
            }
        }

        return;
    }

    boost::shared_lock<boost::shared_mutex> streamLock(m_ooVooStreamMutex);
    std::map<uint32_t, OoVooStreamInfo>::iterator ooVooStreamInfoMapIt = m_ooVooStreamInfoMap.find(streamId);
    if (ooVooStreamInfoMapIt == m_ooVooStreamInfoMap.end()) {
        ELOG_WARN("No corresponding user for the ooVoo %s stream %u",
            type == AUDIO ? "AUDIO" : "VIDEO", streamId);
        return;
    }

    uint32_t userId = (*ooVooStreamInfoMapIt).second.userId;
    streamLock.unlock();

    boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
    it = m_subscribers.find(userId);
    if (it != m_subscribers.end()) {
        if (packetType == RTCP_Sender_PT) { // Sender Report
            ELOG_TRACE("Sending %s sender report %u to the WebRTC subscriber to %u",
                type == AUDIO ? "AUDIO" : "VIDEO", streamId, userId);
        } else {
            ELOG_TRACE("Sending %s stream %u to the WebRTC subscriber to %u",
                type == AUDIO ? "AUDIO" : "VIDEO", streamId, userId);
        }
        std::vector<boost::shared_ptr<MediaSink>>& sub = (*it).second;
        for (uint32_t i = 0; i < sub.size(); ++i) {
            if (type == AUDIO)
                sub[i]->deliverAudioData(rtpdata, len);
            else
                sub[i]->deliverVideoData(rtpdata, len);
        }
    } else {
        ELOG_WARN("No subscriber to %u is available", userId);
    }
}

void OoVooGateway::onProcessFeedback()
{
    // Firstly, send the feedback from the WebRTC client to ooVoo.

    // ooVoo doesn't support separated packet loss rate feedback for different
    // streams, instead it needs the average aggregated loss rate of ALL the
    // inbound streams. So we need to re-calculate it.
    //
    // Note that the feedback sink is for one inbound client only (for now),
    // and we don't want to invoke the related ooVoo API each time a RTCP feedback
    // is received for one inbound client, instead we want to invoke the ooVoo API
    // with the combined loss rate information for all inbound clients, which can
    // be performed at a fixed interval and means that we need to cumulate the
    // loss rate from all of the feedback sinks.
    uint32_t totalNumberOfPackets = 0;
    uint32_t aggregatedFracLost = 0;
    uint32_t aggregatedRTT = 0;
    uint32_t numberRTTCounted = 0;
    std::map<uint32_t, boost::shared_ptr<WebRTCFeedbackProcessor>>::iterator it;
    boost::shared_lock<boost::shared_mutex> lock(m_feedbackMutex);
    for (it = m_feedbackProcessors.begin(); it != m_feedbackProcessors.end(); ++it) {
        WebRTCFeedbackProcessor* feedback = it->second.get();
        PacketLossInfo packetLoss = feedback->packetLoss();
        totalNumberOfPackets += packetLoss.totalNumberOfPackets;
        aggregatedFracLost += packetLoss.aggregatedFracLost;
        if (packetLoss.totalNumberOfPackets > 0) {
            aggregatedRTT += feedback->lastRTT();
            ++numberRTTCounted;
        }
        feedback->resetPacketLoss();

        // We also process the bitrate of the RTP senders in this timer.
        ProtectedRTPSender* rtpSender = feedback->videoRTPSender();
        if (rtpSender)
            rtpSender->processBitrate();
        rtpSender = feedback->audioRTPSender();
        if (rtpSender)
            rtpSender->processBitrate();
    }
    lock.unlock();

    if (totalNumberOfPackets > 0) {
        double lossRate = ((aggregatedFracLost + totalNumberOfPackets / 2) / totalNumberOfPackets) / 256.0;
        ELOG_TRACE("Send WebRTC client %u packet loss to ooVoo: total #packets %u, aggregated loss rate %f", 
            m_webRTCClientId, totalNumberOfPackets, lossRate);
        if (!m_isClientLeaving)
            m_ooVoo->inboundPacketLoss(lossRate);
    }

    if (numberRTTCounted > 0) {
        m_averageRTT = aggregatedRTT / numberRTTCounted;
        ELOG_TRACE("The Average RTT for all of the inbound streams is %u ms", m_averageRTT.load());
        // FIXME: This is a temporary workaround to avoid a Chrome issue/bug where
        // Chrome will stop decoding the remote video if there're too lately arrived
        // re-transmitted packets; on the other hand Chrome as a sender will re-transmit
        // the packets per request regardless of the network latency/RTT. Based on the
        // RTP/RTCP spec only the sender side knows the RTT. In order to workaround the
        // above issues before they're fixed in the browser, we currently use the RTT
        // got from the Gateway sender side to control the Gateway receiver side behavior,
        // i.e., to disable or enable receiver side NACK based on the sender side RTT.
        // Yes this is awkward and should NOT be kept in the code for a long time.
        if (m_averageRTT > 180) {
            m_audioReceiver->setNACKStatus(false);
            m_videoReceiver->setNACKStatus(false);
        } else {
            bool enableNack = m_feedbackSink && m_feedbackSink->acceptNACK();
            m_audioReceiver->setNACKStatus(enableNack);
            m_videoReceiver->setNACKStatus(enableNack);
        }
    }

    // Secondly, send the feedback from Gateway to the WebRTC client.
    char buf[MAX_DATA_PACKET_SIZE];
    // Deliver the video feedback.
    uint32_t len = m_videoReceiver->generateRTCPFeedback(buf, MAX_DATA_PACKET_SIZE);
    if (len > 0 && m_feedbackSink)
        m_feedbackSink->deliverFeedback(buf, len);
    // Deliver the audio feedback.
    len = m_audioReceiver->generateRTCPFeedback(buf, MAX_DATA_PACKET_SIZE);
    if (len > 0 && m_feedbackSink)
        m_feedbackSink->deliverFeedback(buf, len);

    // Lastly, send FIR if necessary.
    if (--m_timesRemainingForFIR == 0) {
        if (m_webRTCPFramesDelivered > MINIMUM_IFRAME_INTERVAL) {
            ELOG_TRACE("Requesting IFrame to %d proactively", m_webRTCClientId);
            boost::shared_lock<boost::shared_mutex> lock(m_publisherMutex);
            if (publisher)
                publisher->sendFirPacket();
        }

        m_timesRemainingForFIR = PERIODIC_FIR_MULTIPLIER;
    }
}

// The main thread
bool OoVooGateway::setPublisher(erizo::MediaSource* source, const std::string& videoResolution)
{
    ELOG_DEBUG("SET PUBLISHER");
    m_audioReceiver.reset(new ProtectedRTPReceiver(m_outboundStreamProcessor));
    m_videoReceiver.reset(new ProtectedRTPReceiver(m_outboundStreamProcessor));
    boost::unique_lock<boost::shared_mutex> lock(m_publisherMutex);
    publisher.reset(source);
    // Set the NACK status of the RTPReceiver(s).
    m_feedbackSink = publisher->getFeedbackSink();
    bool enableNack = m_feedbackSink && m_feedbackSink->acceptNACK();
    m_audioReceiver->setNACKStatus(enableNack);
    m_videoReceiver->setNACKStatus(enableNack);
    // Set the feedback sink of the RTPReceiver (usually used for it to send immediate NACK messages).
    FeedbackSource* fbSource = m_audioReceiver->getFeedbackSource();
    if (fbSource)
        fbSource->setFeedbackSink(m_feedbackSink);
    fbSource = m_videoReceiver->getFeedbackSource();
    if (fbSource)
        fbSource->setFeedbackSink(m_feedbackSink);

    if (videoResolution == "sif")
        m_webRTCVideoResolution = sif;
    else if (videoResolution == "vga")
        m_webRTCVideoResolution = vga;
    else if (videoResolution == "hd_720p" || videoResolution == "hd720p")
        m_webRTCVideoResolution = hd_720p;
    // Other resolution strings result in unknown resolution type.

    // FIXME: We need to add a map between the Video/AudioCoderType and the string in SDP.
    switch (m_videoCodec) {
    case VideoCoderType::vp8:
        ELOG_DEBUG("Reset WebRTC publisher video codec to be vp8 according to the feedback from ooVoo");
        publisher->setVideoCodec("VP8", 90000);
        break;
    case VideoCoderType::h264:
    case VideoCoderType::unknown:
    default:
        ELOG_WARN("Ignore the WebRTC publisher video codec type settings by ooVoo; only VP8 is supported now");
        break;
    }

    switch (m_audioCodec) {
    case AudioCoderType::opus:
        ELOG_DEBUG("Reset WebRTC publisher audio codec to be opus according to the feedback from ooVoo");
        publisher->setAudioCodec("opus", 48000);
        break;
    case AudioCoderType::g711:
        ELOG_DEBUG("Reset WebRTC publisher audio codec to be g711 according to the feedback from ooVoo");
        publisher->setAudioCodec("PCMU", 8000);
        break;
    case AudioCoderType::unknown:
    default:
        ELOG_WARN("Ignore the WebRTC publisher audio codec type settings by ooVoo; only opus and g711 are supported now");
        break;
    }

    publisher->setAudioSink(this);
    publisher->setVideoSink(this);
    publisher->setInitialized(true);
    lock.unlock();

    m_ooVooTimer->registerFeedbackTimerListener(this);
    m_ooVooTimer->setFeedbackTimer(FEEDBACK_TIMER_INTERVAL);
    m_ooVooTimer->setRunning();

    return true;
}

// The main thread
bool OoVooGateway::setPublisher(MediaSource* source)
{
    return setPublisher(source, "unknown");
}

// The main thread
void OoVooGateway::unsetPublisher()
{
    if (!publisher)
        return;

    // Publisher set/unset is only performed by the main thread, but other
    // threads may need to know whether there's an active publisher or not.
    boost::unique_lock<boost::shared_mutex> lock(m_publisherMutex);

    // We don't wait for the onOutboundStreamClose response from ooVoo
    // to destruct the publisher.

    boost::shared_ptr<erizo::MediaSource> dyingPublisher = publisher;
    // We need to reset the publisher earlier to let the other working thread
    // know that we're in the unpublishing process.
    publisher.reset();
    m_ooVooTimer->cancelFeedbackTimer();

    if (m_outboundStreamProcessor->unmapSsrc(dyingPublisher->getVideoSourceSSRC()) != m_webRTCVideoStreamId) {
        ELOG_WARN("Fail to remove the outbound video stream %u", m_webRTCVideoStreamId);
    }
    if (m_outboundStreamProcessor->unmapSsrc(dyingPublisher->getAudioSourceSSRC()) != m_webRTCAudioStreamId) {
        ELOG_WARN("Fail to remove the outbound audio stream %u", m_webRTCAudioStreamId);
    }

    if (m_webRTCVideoStreamId != static_cast<uint32_t>(NOT_A_OOVOO_STREAM_ID)) {
        ELOG_DEBUG("ooVoo -> outboundStreamClose: user %u, VIDEO stream %u", m_webRTCClientId, m_webRTCVideoStreamId);
        m_ooVoo->outboundStreamClose(m_webRTCVideoStreamId);
        m_webRTCVideoStreamId = NOT_A_OOVOO_STREAM_ID;
    }
    if (m_webRTCAudioStreamId != static_cast<uint32_t>(NOT_A_OOVOO_STREAM_ID)) {
        ELOG_DEBUG("ooVoo -> outboundStreamClose: user %u, AUDIO stream %u", m_webRTCClientId, m_webRTCAudioStreamId);
        m_ooVoo->outboundStreamClose(m_webRTCAudioStreamId);
        m_webRTCAudioStreamId = NOT_A_OOVOO_STREAM_ID;
    }

    lock.unlock();

    m_feedbackSink = nullptr;
    dyingPublisher.reset();
    m_videoReceiver.reset();
    m_audioReceiver.reset();
    m_gatewayStats.packetsReceived = 0;
    m_gatewayStats.packetsLost = 0;
}

// The main thread
void OoVooGateway::addSubscriber(MediaSink* sink, uint32_t id)
{
    ELOG_DEBUG("Adding subscriber");
    FeedbackSource* fbsource = sink->getFeedbackSource();

    if (fbsource) {
        ELOG_DEBUG("Adding fbsource");
        WebRTCFeedbackProcessor* fbSink = nullptr;
        boost::upgrade_lock<boost::shared_mutex> lock(m_feedbackMutex);
        std::map<uint32_t, boost::shared_ptr<WebRTCFeedbackProcessor>>::iterator it = m_feedbackProcessors.find(id);
        if (it != m_feedbackProcessors.end()) {
            assert((*it).second->mediaSourceId() == id);
            ELOG_DEBUG("Existing feedback sink to %u is available", id);
            fbSink = (*it).second.get();
        } else {
            fbSink = new WebRTCFeedbackProcessor(id);
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
            m_feedbackProcessors[id].reset(fbSink);
        }
        lock.unlock();
        fbsource->setFeedbackSink(fbSink);
    }
    boost::lock_guard<boost::shared_mutex> subscriberLock(m_subscriberMutex);
    m_subscribers[id].push_back(boost::shared_ptr<MediaSink>(sink));
}

// The main thread
void OoVooGateway::removeSubscriber(uint32_t id)
{
    std::vector<boost::shared_ptr<MediaSink>> removedSubscribers;

    boost::unique_lock<boost::shared_mutex> subscriberLock(m_subscriberMutex);
    // Firstly, copy the removed subscribers to a local vector before removing
    // them from the subscribers map, then we release the lock for the map.
    // This allows we destroying the subscribers after releasing the map lock
    // which avoids potential dead-locks between the main thread (destroying
    // the subscribers) and the WebRTC threads (receiving and sending streams).
    std::map<uint32_t, std::vector<boost::shared_ptr<MediaSink>>>::iterator it = m_subscribers.find(id);
    if (it != m_subscribers.end()) {
        removedSubscribers.insert(removedSubscribers.begin(), it->second.begin(), it->second.end());
        it->second.clear();
        m_subscribers.erase(it);
    }
    subscriberLock.unlock();

    boost::shared_lock<boost::shared_mutex> streamInfoLock(m_ooVooStreamMutex);
    std::map<uint32_t, OoVooStreamInfo>::iterator streamInfoIter;
    for (streamInfoIter = m_ooVooStreamInfoMap.begin(); streamInfoIter != m_ooVooStreamInfoMap.end(); ++streamInfoIter) {
        if (streamInfoIter->second.userId == id) {
            ELOG_DEBUG("ooVoo -> inboundStreamClose [%u], %s stream %u from user %u",
                m_webRTCClientId, streamInfoIter->second.isAudio ? "AUDIO" : "VIDEO", streamInfoIter->first, id);
            m_ooVoo->inboundStreamClose(streamInfoIter->first);
        }
    }
    streamInfoLock.unlock();

    removedSubscribers.clear();
    boost::upgrade_lock<boost::shared_mutex> feedbackLock(m_feedbackMutex);
    std::map<uint32_t, boost::shared_ptr<WebRTCFeedbackProcessor>>::iterator feedbackIter = m_feedbackProcessors.find(id);
    if (feedbackIter != m_feedbackProcessors.end()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(feedbackLock);
        m_feedbackProcessors.erase(feedbackIter);
    }
}

bool OoVooGateway::clientJoin(const std::string& clientJoinUri)
{
    ELOG_DEBUG("ooVoo -> clientJoin");
    m_ooVoo->clientJoin(clientJoinUri.c_str());
    return !m_isClientLeaving;
}

void OoVooGateway::customMessage(const std::string& message)
{
    ELOG_DEBUG("ooVoo -> customMessage - length: %lu", message.size());
    m_ooVoo->customMessage(const_cast<char*>(message.c_str()), message.size());
}

std::string OoVooGateway::retrieveGatewayStatistics()
{
    std::ostringstream output;

    if (m_audioReceiver && m_videoReceiver) {
        uint32_t accumulatedReceived = m_audioReceiver->packetsReceived() + m_videoReceiver->packetsReceived();
        uint32_t accumulatedLost = m_audioReceiver->packetsLost() + m_videoReceiver->packetsLost();
        uint32_t receivedSinceLastTime = accumulatedReceived - m_gatewayStats.packetsReceived;
        uint32_t lostSinceLastTime = accumulatedLost - m_gatewayStats.packetsLost;
        m_gatewayStats.packetsReceived = accumulatedReceived;
        m_gatewayStats.packetsLost = accumulatedLost;

        output << "{\"packetsReceived\":" << receivedSinceLastTime
            << ",\"packetsLost\":" << lostSinceLastTime
            << ",\"averageRTT\":" << m_averageRTT << "}";
    }

    return output.str();
}

void OoVooGateway::subscribeStream(uint32_t id, bool isAudio) {
    ELOG_DEBUG("ooVoo -> inboundStreamCreate: user %u, %s", id, isAudio ? "AUDIO" : "VIDEO");
    m_ooVoo->inboundStreamCreate(id, isAudio);
}

void OoVooGateway::unsubscribeStream(uint32_t id, bool isAudio) {
    bool found = false;

    boost::shared_lock<boost::shared_mutex> streamInfoLock(m_ooVooStreamMutex);
    std::map<uint32_t, OoVooStreamInfo>::iterator streamInfoIter;
    for (streamInfoIter = m_ooVooStreamInfoMap.begin(); streamInfoIter != m_ooVooStreamInfoMap.end(); ++streamInfoIter) {
        if (streamInfoIter->second.userId == id && streamInfoIter->second.isAudio == isAudio) {
            found = true;
            ELOG_DEBUG("ooVoo -> inboundStreamClose [%u], %s stream %u from user %u",
                m_webRTCClientId, streamInfoIter->second.isAudio ? "AUDIO" : "VIDEO", streamInfoIter->first, id);
            m_ooVoo->inboundStreamClose(streamInfoIter->first);
        }
    }
    streamInfoLock.unlock();

    if (!found && isAudio) {
        // If we cannot find a valid stream id from the user id and it's an audio stream,
        // this must be the mixed audio stream with the special user/stream id.
        ELOG_DEBUG("ooVoo -> inboundStreamClose [%u], MIXED AUDIO stream", m_webRTCClientId);
        m_ooVoo->inboundStreamClose(MIXED_OOVOO_AUDIO_STREAM_ID);
    }
}

void OoVooGateway::publishStream(bool isAudio) {
    if (!publisher)
        return;

    if (isAudio && publisher->getAudioSourceSSRC())
        audioReady();
    else if (publisher->getVideoSourceSSRC())
        videoReady();
}

void OoVooGateway::unpublishStream(bool isAudio) {
    if (!publisher)
        return;

    if (isAudio) {
        if (m_webRTCAudioStreamId != static_cast<uint32_t>(NOT_A_OOVOO_STREAM_ID)) {
            ELOG_DEBUG("ooVoo -> outboundStreamClose: user %u, AUDIO stream %u", m_webRTCClientId, m_webRTCAudioStreamId);
            m_ooVoo->outboundStreamClose(m_webRTCAudioStreamId);
            m_webRTCAudioStreamId = NOT_A_OOVOO_STREAM_ID;
        }
    } else {
        if (m_webRTCVideoStreamId != static_cast<uint32_t>(NOT_A_OOVOO_STREAM_ID)) {
            ELOG_DEBUG("ooVoo -> outboundStreamClose: user %u, VIDEO stream %u", m_webRTCClientId, m_webRTCVideoStreamId);
            m_ooVoo->outboundStreamClose(m_webRTCVideoStreamId);
            m_webRTCVideoStreamId = NOT_A_OOVOO_STREAM_ID;
        }
    }
}

// The main thread
void OoVooGateway::closeAllSubscribers()
{
    ELOG_DEBUG("closeAllSubscribers");
    std::vector<boost::shared_ptr<MediaSink>> removedSubscribers;

    boost::unique_lock<boost::shared_mutex> subscriberLock(m_subscriberMutex);
    // Firstly, copy the removed subscribers to a local vector before removing
    // them from the subscribers map, then we release the lock for the map.
    // This allows we destroying the subscribers after releasing the map lock
    // which avoids potential dead-locks between the main thread (destroying
    // the subscribers) and the WebRTC threads (receiving and sending streams).
    std::map<uint32_t, std::vector<boost::shared_ptr<MediaSink>>>::iterator it = m_subscribers.begin();
    for (; it != m_subscribers.end(); ++it) {
        // Always insert elements after the last element in the vector to avoid
        // moving existing elements;
        removedSubscribers.insert(removedSubscribers.begin() + removedSubscribers.size(), it->second.begin(), it->second.end());
        it->second.clear();
    }
    m_subscribers.clear();
    subscriberLock.unlock();
}

// The main thread
void OoVooGateway::closeWebRTCClient()
{
    ELOG_DEBUG("closeWebRTCClient - %u, proactive? %s", m_webRTCClientId, m_isClientLeaving ? "No" : "Yes");
    if (!m_isClientLeaving) {
        ELOG_DEBUG("ooVoo -> clientLeave")
        m_ooVoo->clientLeave();
    }

    m_ooVooTimer->setStopped();
    closeAllSubscribers();
    if (publisher) {
        // We need to release the lock before the MediaSource object is destructed,
        // and also we need to reset publisher to let the other threads know about that,
        // so we use a temporary local variable to hold the pointer, which will
        // increase the reference count temporarily before we exit this scope.
        boost::unique_lock<boost::shared_mutex> lock(m_publisherMutex);
        boost::shared_ptr<erizo::MediaSource> dyingPublisher = publisher;
        publisher.reset();
        lock.unlock();
        dyingPublisher.reset();
    }
}

// OoVooProtocolEventsListener implementation

// All of the events are triggered in the ooVoo connection work thread,
// as according to ooVoo it needs to communicate with the AVS server before
// any event is triggered.

// The main thread (if in the Dummy ooVoo implementation)
void OoVooGateway::onClientJoin(uint32_t userId, VideoCoderType vType, AudioCoderType aType, int errorCode)
{
    ELOG_DEBUG("onClientJoin - user: %u, errorCode %d", userId, errorCode);
    m_webRTCClientId = userId;
    m_videoCodec = vType;
    m_audioCodec = aType;

    // FIXME: We need to add a map between the Video/AudioCoderType and the payload type defined in RTP.
    switch (m_videoCodec) {
    case VideoCoderType::vp8:
        ELOG_DEBUG("Set inbound stream video codec to be vp8 according to the feedback from ooVoo");
        m_inboundStreamProcessor->setVideoCodecPayloadType(VP8_90000_PT);
        break;
    case VideoCoderType::h264:
    case VideoCoderType::unknown:
    default:
        ELOG_WARN("Ignore the inbound stream video codec type settings by ooVoo; only VP8 is supported now");
        break;
    }

    switch (m_audioCodec) {
    case AudioCoderType::opus:
        ELOG_DEBUG("Set inbound stream audio codec to be opus according to the feedback from ooVoo");
        m_inboundStreamProcessor->setAudioCodecPayloadType(OPUS_48000_PT);
        break;
    case AudioCoderType::g711:
        ELOG_DEBUG("Set inbound stream audio codec to be g711 according to the feedback from ooVoo");
        m_inboundStreamProcessor->setAudioCodecPayloadType(PCMU_8000_PT);
        break;
    case AudioCoderType::unknown:
    default:
        ELOG_WARN("Ignore the inbound stream audio codec type settings by ooVoo; only opus and g711 are supported now");
        break;
    }

    notifyAsyncEvent("ClientJoin", userId);
}

void OoVooGateway::onClientLeave(int errorCode)
{
    if (m_isClientLeaving)
        return;

    m_isClientLeaving = true;

    ELOG_DEBUG("onClientLeave - user: %u, errorCode %d", m_webRTCClientId, errorCode);
    // We should not make the optimistic assumption that the ooVoo library will
    // never call this if the ooVoo connection has not been established for now,
    // though it violates the convention that every ooVoo event is triggered
    // after the ooVoo library communicates with the AVS server.
    // In practice, current ooVoo library does invoke this without communicating
    // with the AVS server in some cases, so it's possible that the connection
    // has not been established yet. We may be optimistic again if the ooVoo
    // library strictly follows the convention described above.
    if (m_ooVooConnection)
        m_ooVooConnection->close();

    notifyAsyncEvent("ClientLeave", m_webRTCClientId);
}

// The WebRTC connection receive threads (audio and video, if in the Dummy ooVoo implementation)
void OoVooGateway::onOutboundStreamCreate(uint32_t streamId, bool isAudio, int errorCode)
{
    ELOG_DEBUG("onOutboundStreamCreate [%u] - stream: %u, %s", m_webRTCClientId, streamId, isAudio ? "AUDIO" : "VIDEO");
    boost::shared_lock<boost::shared_mutex> lock(m_publisherMutex);

    if (isAudio) {
        notifyAsyncEvent("OutboundAudioCreate", m_webRTCClientId);
        m_webRTCAudioStreamId = streamId;
        if (!publisher || !m_outboundStreamProcessor->mapSsrcToStreamId(publisher->getAudioSourceSSRC(), streamId)) {
            ELOG_WARN("Fail to add the outbound audio stream %u", streamId);
        }
    } else {
        notifyAsyncEvent("OutboundVideoCreate", m_webRTCClientId);
        m_webRTCVideoStreamId = streamId;
        m_outboundStreamProcessor->setVideoStreamResolution(streamId, m_webRTCVideoResolution);
        if (!publisher || !m_outboundStreamProcessor->mapSsrcToStreamId(publisher->getVideoSourceSSRC(), streamId)) {
            ELOG_WARN("Fail to add the outbound video stream %u", streamId);
        }
    }
}

void OoVooGateway::onOutboundStreamClose(uint32_t streamId, int errorCode)
{
    ELOG_DEBUG("onOutboundStreamClose [%u] - stream: %u", m_webRTCClientId, streamId);

    if (streamId == m_webRTCAudioStreamId)
        notifyAsyncEvent("OutboundAudioClose", m_webRTCClientId);
    else if (streamId == m_webRTCVideoStreamId)
        notifyAsyncEvent("OutboundVideoClose", m_webRTCClientId);
    else {
        ELOG_WARN("Invalid outbound stream id: %u", streamId);
    }
}

// The request from the ooVoo lib to the WebRTC client for a full intra frame
void OoVooGateway::onRequestIFrame()
{
    ELOG_DEBUG("onRequestIFrame - to WebRTC client %d", m_webRTCClientId);
    boost::shared_lock<boost::shared_mutex> lock(m_publisherMutex);
    if (publisher)
        publisher->sendFirPacket();
}

void OoVooGateway::onCreateInboundClient(std::string userName, uint32_t userId, std::string participantInfo)
{
    ELOG_DEBUG("onCreateInboundClient [%u] - user: %u, name: %s", m_webRTCClientId, userId, userName.c_str());

    std::ostringstream message;
    message << "{\"userId\":" << userId
            << ",\"name\":" << "\"" << userName << "\""
            << ",\"info\":" << "\"" << participantInfo << "\""
            << "}";
    notifyAsyncEvent("CreateInboundClient", message.str());

    ELOG_DEBUG("ooVoo -> inboundClientCreated");
    m_ooVoo->inboundClientCreated(0);
}

void OoVooGateway::onCloseInboundClient(uint32_t userId)
{
    ELOG_DEBUG("onCloseInboundClient [%u] - user: %u", m_webRTCClientId, userId);
    notifyAsyncEvent("CloseInboundClient", userId);
}

void OoVooGateway::onInboundStreamCreate(uint32_t userId, uint32_t streamId, bool isAudio, uint16_t resolution)
{
    ELOG_DEBUG("onInboundStreamCreate [%u] - user: %u, stream: %u, %s", m_webRTCClientId, userId, streamId, isAudio ? "AUDIO" : "VIDEO");
    if (streamId != static_cast<uint32_t>(MIXED_OOVOO_AUDIO_STREAM_ID)) {
        boost::unique_lock<boost::shared_mutex> lock(m_ooVooStreamMutex);
        m_ooVooStreamInfoMap[streamId] = {userId, isAudio};
    }

    boost::shared_ptr<ProtectedRTPSender> rtpSender = m_inboundStreamProcessor->createRTPSender(streamId, this);

    boost::shared_lock<boost::shared_mutex> lock(m_subscriberMutex);
    std::map<uint32_t, std::vector<boost::shared_ptr<MediaSink>>>::iterator subIter = m_subscribers.find(userId);
    if (subIter != m_subscribers.end()) {
        std::vector<boost::shared_ptr<MediaSink>>& sub = subIter->second;
        // We must have a valid subscriber for this stream till now.
        assert(sub.size() == 1);
        rtpSender->enableEncapsulatedRTPData(sub[0]->acceptEncapsulatedRTPData());
        boost::shared_lock<boost::shared_mutex> lock(m_feedbackMutex);
        std::map<uint32_t, boost::shared_ptr<WebRTCFeedbackProcessor>>::iterator it = m_feedbackProcessors.find(userId);
        if (it != m_feedbackProcessors.end()) {
            if (!isAudio) {
                rtpSender->setFecStatus(sub[0]->acceptFEC());
                rtpSender->setNACKStatus(sub[0]->acceptResentData());
                // The video feedback reactor needs to know the video sender's SSRC
                // to filter out the video specific browser receiver reports.
                it->second->initVideoFeedbackReactor(streamId, sub[0]->getVideoSinkSSRC(), rtpSender, boost::shared_ptr<IntraFrameCallback>());
            } else
                it->second->initAudioFeedbackReactor(streamId, sub[0]->getAudioSinkSSRC(), rtpSender);
        }
    }

    if (streamId != static_cast<uint32_t>(MIXED_OOVOO_AUDIO_STREAM_ID))
        notifyAsyncEvent(isAudio ? "InboundAudioCreate" : "InboundVideoCreate", userId);
    else
        notifyAsyncEvent("AllInboundAudioCreate", "");
}

void OoVooGateway::onInboundStreamClose(uint32_t streamId)
{
    ELOG_DEBUG("onInboundStreamClose [%u] - stream: %u", m_webRTCClientId, streamId);

    if (streamId == static_cast<uint32_t>(MIXED_OOVOO_AUDIO_STREAM_ID)) {
        m_inboundStreamProcessor->closeStream(streamId);
        notifyAsyncEvent("AllInboundAudioClose", "");
        return;
    }

    boost::unique_lock<boost::shared_mutex> lock(m_ooVooStreamMutex);
    std::map<uint32_t, OoVooStreamInfo>::iterator streamInfoIter = m_ooVooStreamInfoMap.find(streamId);
    if (streamInfoIter != m_ooVooStreamInfoMap.end()) {
        uint32_t userId = streamInfoIter->second.userId;
        bool isAudio = streamInfoIter->second.isAudio;
        m_ooVooStreamInfoMap.erase(streamInfoIter);
        lock.unlock();

        boost::shared_lock<boost::shared_mutex> feedbackLock(m_feedbackMutex);
        std::map<uint32_t, boost::shared_ptr<WebRTCFeedbackProcessor>>::iterator it = m_feedbackProcessors.find(userId);
        if (it != m_feedbackProcessors.end()) {
            if (!isAudio)
                it->second->resetVideoFeedbackReactor();
            else
                it->second->resetAudioFeedbackReactor();
        }
        feedbackLock.unlock();

        m_inboundStreamProcessor->closeStream(streamId);

        notifyAsyncEvent(isAudio ? "InboundAudioClose" : "InboundVideoClose", userId);
    } else {
        ELOG_WARN("Invalid stream id: %u", streamId);
    }
}

// The ooVoo connection work thread
void OoVooGateway::onDataReceive(mediaPacket_t* packet)
{
    ELOG_TRACE("onDataReceive - ooVoo %s length: %u", packet->isAudio ? "AUDIO" : "VIDEO", packet->length);
    // Format the packet to RTP packet.
    m_inboundStreamProcessor->receiveOoVooData(*packet);
}

void OoVooGateway::onCustomMessage(void* pData, uint32_t length)
{
    std::string data((const char*)pData, (size_t)length);
    ELOG_TRACE("onCustomMessage - length: %u", length);
    notifyAsyncEvent("CustomMessage", data);
}

// The main thread
void OoVooGateway::onEstablishAVSConnection(std::string ip, uint32_t port, bool isUdp)
{
    ELOG_DEBUG("onEstablishAVSConnection - ip: %s, port: %u, isUdp? %d", ip.c_str(), port, isUdp);
    if (!m_ooVooConnection)
        m_ooVooConnection.reset(new OoVooConnection(m_ooVoo));

    m_ooVooConnection->connect(ip, port, isUdp);
}

// (In the Dummy ooVoo implementation) The main thread, the WebRTC connection receive threads and (?) the ooVoo connection work thread
void OoVooGateway::onSentMessage(void* pMessage, uint32_t length, bool isUdp)
{
    ELOG_TRACE("onSentMessage - message length: %u, isUdp? %d", length, isUdp);
    assert(m_ooVooConnection);
    m_ooVooConnection->sendData(static_cast<const char*>(pMessage), length, isUdp); // ooVoo connection doesn't have separate transports for audio and video.
}

void OoVooGateway::onSyncMsg(uint64_t ntp, uint64_t rtp, bool isAudio, uint32_t streamId)
{
    ELOG_TRACE("onSyncMsg - ntp: %lu, rtp: %lu, %s, %d", ntp, rtp, isAudio ? "AUDIO" : "VIDEO", streamId);
    m_inboundStreamProcessor->receiveOoVooSyncMsg(ntp, rtp, isAudio, streamId);
}

// The main thread
void OoVooGateway::setupAsyncEvent(const std::string& event, EventRegistry* handle)
{
    // We should not acquire a lock here because of potential dead lock.
    // This method should be invoked by the main thread at the very beginning
    // when the WebRTC client (browser) connects to the Gateway so we're safe
    // to do the following tasks without acquiring a lock.
    std::map<std::string, EventRegistry*>::iterator it = m_asyncHandles.find(event);
    if (it != m_asyncHandles.end()) {
        delete it->second;
        it->second = handle;
        ELOG_DEBUG("setupAsyncEvent - replace old listener for %s", event.c_str());
    } else {
        m_asyncHandles[event] = handle;
        ELOG_DEBUG("setupAsyncEvent - add listener for %s##%p", event.c_str(), handle);
    }
}

// The main thread
void OoVooGateway::destroyAsyncEvents()
{
    // This method should be invoked by the main thread when the WebRTC client
    // (browser) disconnects to the Gateway and the main thread are closing the uv async handles.
    // Other threads should check the existence of the event before async send.
    boost::lock_guard<boost::mutex> lock(m_asyncMutex);
    std::map<std::string, EventRegistry*>::iterator it;
    for (it = m_asyncHandles.begin(); it != m_asyncHandles.end(); it++) {
        delete it->second;
        m_asyncHandles.erase(it);
    }
    ELOG_DEBUG("destroyAsyncEvents [%u]", m_webRTCClientId);
}

void OoVooGateway::notifyAsyncEvent(const std::string& event, const std::string& data)
{
    boost::lock_guard<boost::mutex> lock(m_asyncMutex);
    std::map<std::string, EventRegistry*>::iterator it = m_asyncHandles.find(event);
    if (it != m_asyncHandles.end()) {
        bool sent = it->second->notify(data);
        ELOG_DEBUG("notifyAsyncEvent [%u] - event: %s; sent? %d", m_webRTCClientId, event.c_str(), sent);
    } else {
        ELOG_DEBUG("notifyAsyncEvent [%u] - missing event handler: %s", m_webRTCClientId, event.c_str());
    }
}

void OoVooGateway::notifyAsyncEvent(const std::string& event, uint32_t data)
{
    std::ostringstream message;
    message << data;
    notifyAsyncEvent(event, message.str());
}

DEFINE_LOGGER(OoVooJobTimer, "ooVoo.OoVooJobTimer");

OoVooJobTimer::OoVooJobTimer(OoVooProtocolStack* ooVoo)
    : m_isRunning(false)
    , m_isStopping(false)
    , m_freq(0)
    , m_feedbackFreq(0)
    , m_fbTimerListener(nullptr)
    , m_ooVoo(ooVoo)
{
}

OoVooJobTimer::~OoVooJobTimer()
{
    setStopped();
}

void OoVooJobTimer::setStopped()
{
    if (m_timer)
        m_timer->cancel();
    if (m_feedbackTimer)
        m_feedbackTimer->cancel();

    if (m_isRunning) {
        m_isStopping = true;
        m_workThread.join();
        m_isRunning = false;
        m_isStopping = false;
    }
}

void OoVooJobTimer::setRunning()
{
    // if (m_workThread.get_id() == boost::thread::id()) // Not-A-Thread
    if (!m_isRunning) {
        m_workThread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_io_service));
        m_isRunning = true;
    }
}

bool OoVooJobTimer::isRunning()
{
    return m_isRunning;
}

void OoVooJobTimer::setTimer(uint64_t frequency)
{
    // Assume seconds for now.
    m_freq = frequency;
    m_timer.reset(new boost::asio::deadline_timer(m_io_service, boost::posix_time::seconds(m_freq)));
    m_timer->async_wait(boost::bind(&OoVooJobTimer::timerHandler, this, boost::asio::placeholders::error));
}

void OoVooJobTimer::timerHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        m_ooVoo->JobTimerPulse();
        if (!m_isStopping) {
            m_timer->expires_at(m_timer->expires_at() + boost::posix_time::seconds(m_freq));
            m_timer->async_wait(boost::bind(&OoVooJobTimer::timerHandler, this, boost::asio::placeholders::error));
        }
    } else {
        ELOG_INFO("ooVoo Job timer error: %s", ec.message().c_str());
    }
}

void OoVooJobTimer::cancelFeedbackTimer()
{
    if (m_feedbackTimer)
        m_feedbackTimer->cancel();

    m_feedbackTimer.reset();
    m_fbTimerListener = nullptr;
}

void OoVooJobTimer::registerFeedbackTimerListener(FeedbackTimerListener* fbTimerListener)
{
    m_fbTimerListener = fbTimerListener;
}

void OoVooJobTimer::setFeedbackTimer(uint32_t frequency)
{
    // Assume seconds for now.
    m_feedbackFreq = frequency;
    m_feedbackTimer.reset(new boost::asio::deadline_timer(m_io_service, boost::posix_time::seconds(m_feedbackFreq)));
    m_feedbackTimer->async_wait(boost::bind(&OoVooJobTimer::feedbackTimerHandler, this, boost::asio::placeholders::error));
}

void OoVooJobTimer::feedbackTimerHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        if (m_fbTimerListener)
            m_fbTimerListener->onProcessFeedback();
        if (!m_isStopping && m_feedbackTimer) {
            m_feedbackTimer->expires_at(m_feedbackTimer->expires_at() + boost::posix_time::seconds(m_feedbackFreq));
            m_feedbackTimer->async_wait(boost::bind(&OoVooJobTimer::feedbackTimerHandler, this, boost::asio::placeholders::error));
        }
    } else {
        ELOG_INFO("Feedback timer error: %s", ec.message().c_str());
    }
}

}/* namespace oovoo_gateway */

