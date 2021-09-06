// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoFrameConstructor.h"

#include <future>
#include <random>
#include <rtputils.h>

using namespace rtc_adapter;

namespace owt_base {

DEFINE_LOGGER(VideoFrameConstructor, "owt.VideoFrameConstructor");

VideoFrameConstructor::VideoFrameConstructor(VideoInfoListener* vil, uint32_t transportccExtId)
    : m_enabled(true)
    , m_ssrc(0)
    , m_transport(nullptr)
    , m_pendingKeyFrameRequests(0)
    , m_videoInfoListener(vil)
    , m_rtcAdapter(RtcAdapterFactory::CreateRtcAdapter())
    , m_videoReceive(nullptr)
{
    m_config.transport_cc = transportccExtId;
    m_feedbackTimer = SharedJobTimer::GetSharedFrequencyTimer(1);
    m_feedbackTimer->addListener(this);
}

VideoFrameConstructor::VideoFrameConstructor(
    std::shared_ptr<rtc_adapter::RtcAdapter> rtcAdapter,
    VideoInfoListener* vil, uint32_t transportccExtId)
    : m_enabled(true)
    , m_ssrc(0)
    , m_transport(nullptr)
    , m_pendingKeyFrameRequests(0)
    , m_videoInfoListener(vil)
    , m_videoReceive(nullptr)
{
    m_config.transport_cc = transportccExtId;
    assert(rtcAdapter.get());
    m_feedbackTimer = SharedJobTimer::GetSharedFrequencyTimer(1);
    m_feedbackTimer->addListener(this);
    m_rtcAdapter = rtcAdapter;
}

VideoFrameConstructor::VideoFrameConstructor(KeyFrameRequester* requester)
    : m_enabled(true)
    , m_ssrc(0)
    , m_transport(nullptr)
    , m_pendingKeyFrameRequests(0)
    , m_videoInfoListener(nullptr)
    , m_rtcAdapter(RtcAdapterFactory::CreateRtcAdapter())
    , m_videoReceive(nullptr)
    , m_requester(requester)
{
    m_config.transport_cc = -1;
    m_feedbackTimer = SharedJobTimer::GetSharedFrequencyTimer(1);
    m_feedbackTimer->addListener(this);
}

VideoFrameConstructor::~VideoFrameConstructor()
{
    m_feedbackTimer->removeListener(this);
    unbindTransport();
    if (m_videoReceive) {
        m_rtcAdapter->destoryVideoReceiver(m_videoReceive);
        m_rtcAdapter.reset();
        m_videoReceive = nullptr;
    }
}

void VideoFrameConstructor::maybeCreateReceiveVideo(uint32_t ssrc)
{
    if (!m_videoReceive) {
        ELOG_DEBUG("createVideoReceiver %p", this);
        // Create Receive Video Stream
        rtc_adapter::RtcAdapter::Config recvConfig;
        recvConfig.ssrc = ssrc;
        recvConfig.transport_cc = m_config.transport_cc;
        recvConfig.rtp_listener = this;
        recvConfig.stats_listener = this;
        recvConfig.frame_listener = this;

        m_videoReceive = m_rtcAdapter->createVideoReceiver(recvConfig);
        if (m_currentSpatialLayer >= 0 || m_currentSpatialLayer >= 0) {
            m_videoReceive->setPreferredLayers(
                m_currentSpatialLayer, m_currentTemporalLayer);
        }
    }
}

void VideoFrameConstructor::bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_transportMutex);
    m_transport = source;
    m_transport->setVideoSink(this);
    m_transport->setEventSink(this);
    setFeedbackSink(fbSink);
}

void VideoFrameConstructor::unbindTransport()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transportMutex);
    if (m_transport) {
        setFeedbackSink(nullptr);
        m_transport = nullptr;
    }
}

void VideoFrameConstructor::enable(bool enabled)
{
    m_enabled = enabled;
    if(!m_enabled) {
        m_ssrc = 0;
        m_rtcAdapter->destoryVideoReceiver(m_videoReceive);
        m_videoReceive = nullptr;
    } else {
        RequestKeyFrame();
    }
}

int32_t VideoFrameConstructor::RequestKeyFrame()
{
    if (!m_enabled) {
        return 0;
    }
    if (m_videoReceive) {
        ELOG_DEBUG("videoReceive requestKeyFrame")
        m_videoReceive->requestKeyFrame();
    }
    return 0;
}

bool VideoFrameConstructor::setBitrate(uint32_t kbps)
{
    // At present we do not react on this request
    return true;
}

void VideoFrameConstructor::setPreferredLayers(int spatialId, int temporalId)
{
    m_currentSpatialLayer = spatialId;
    m_currentTemporalLayer = temporalId;
    if (m_videoReceive) {
        m_videoReceive->setPreferredLayers(
            m_currentSpatialLayer, m_currentTemporalLayer);
    }
}

bool VideoFrameConstructor::addChildProcessor(std::string id, erizo::MediaSink* sink)
{
    if (m_childProcessors.count(id) == 0 && sink) {
        ELOG_DEBUG("Add child processor %s %p", id.c_str(), this);
        m_childProcessors[id] = sink;
        return true;
    }
    return false;
}

bool VideoFrameConstructor::removeChildProcessor(std::string id)
{
    if (m_childProcessors.count(id) > 0) {
        ELOG_DEBUG("Remove child processor %s %p", id.c_str(), this);
        m_childProcessors.erase(id);
        return true;
    }
    return false;
}

void VideoFrameConstructor::onAdapterFrame(const Frame& frame)
{
    if (m_enabled) {
        deliverFrame(frame);
    }
}

void VideoFrameConstructor::onAdapterStats(const AdapterStats& stats)
{
    if (m_videoInfoListener) {
        std::ostringstream json_str;
        json_str.str("");
        json_str << "{\"video\": {\"parameters\": {\"resolution\": {"
                 << "\"width\":" << stats.width << ", "
                 << "\"height\":" << stats.height
                 << "}}}}";
        m_videoInfoListener->onVideoInfo(json_str.str().c_str());
    }
}

void VideoFrameConstructor::onAdapterData(char* data, int len)
{
    // Data come from video receive stream is RTCP
    if (fb_sink_) {
        fb_sink_->deliverFeedback(
            std::make_shared<erizo::DataPacket>(0, data, len, erizo::VIDEO_PACKET));
    } else if (m_requester && len > 0) {
        // Check PLI/FIR
        RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(data);
        uint8_t packetType = chead->getPacketType();
        uint8_t rcOrFmt = chead->getRCOrFMT();
        if (packetType == RTCP_PS_Feedback_PT &&
            (rcOrFmt == RTCP_PLI_FMT || rcOrFmt == RTCP_FIR_FMT)) {
            ELOG_DEBUG("External RequestKeyFrame");
            m_requester->RequestKeyFrame();
        }
    }
}

int VideoFrameConstructor::deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet)
{
    if (!m_enabled) {
        return 0;
    }
    // Dispatch packets to child processors
    for (auto it = m_childProcessors.begin(); it != m_childProcessors.end(); it++) {
        it->second->deliverVideoData(video_packet);
    }

    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(video_packet->data);
    uint8_t packetType = chead->getPacketType();

    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT)
        return 0;

    const uint8_t rtcpMinPt = 194, rtcpMaxPt = 223;
    if (packetType >= rtcpMinPt && packetType <= rtcpMaxPt) {
        return 0;
    }

    RTPHeader* head = reinterpret_cast<RTPHeader*>(video_packet->data);
    if (!m_ssrc && head->getSSRC()) {
        m_ssrc = head->getSSRC();
        maybeCreateReceiveVideo(m_ssrc);
    }
    if (m_videoReceive) {
        m_videoReceive->onRtpData(video_packet->data, video_packet->length);
    }

    return video_packet->length;
}

int VideoFrameConstructor::deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet)
{
    assert(false);
    return 0;
}

void VideoFrameConstructor::onTimeout()
{
    if (m_pendingKeyFrameRequests > 1) {
        RequestKeyFrame();
    }
    m_pendingKeyFrameRequests = 0;
}

void VideoFrameConstructor::onFeedback(const FeedbackMsg& msg)
{
    if (msg.type == owt_base::VIDEO_FEEDBACK) {
        if (msg.cmd == REQUEST_KEY_FRAME) {
            if (!m_pendingKeyFrameRequests) {
                RequestKeyFrame();
            }
            ++m_pendingKeyFrameRequests;
        } else if (msg.cmd == SET_BITRATE) {
            this->setBitrate(msg.data.kbps);
        }
    }
}

int VideoFrameConstructor::deliverEvent_(erizo::MediaEventPtr event)
{
    return 0;
}

void VideoFrameConstructor::close()
{
    unbindTransport();
}
}
