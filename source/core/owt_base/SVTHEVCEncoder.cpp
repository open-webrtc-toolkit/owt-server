// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "SVTHEVCEncoder.h"

#include <webrtc/api/video/video_frame.h>
#include <webrtc/api/video/video_frame_buffer.h>

#include <libyuv/convert.h>
#include <libyuv/planar_functions.h>
#include <libyuv/scale.h>

#include "MediaUtilities.h"

#include "ltrace.h"

namespace owt_base {

DEFINE_LOGGER(SVTHEVCEncoder, "owt.SVTHEVCEncoder");

SVTHEVCEncoder::SVTHEVCEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast)
    : m_encoderReady(false)
    , m_dest(NULL)
    , m_width(0)
    , m_height(0)
    , m_frameRate(0)
    , m_bitrateKbps(0)
    , m_keyFrameIntervalSeconds(0)
    , m_encoded_frame_count(0)
    , m_sendThreadExited(true)
{
    m_hevc_encoder = boost::make_shared<SVTHEVCEncoderBase>();

    m_srv       = boost::make_shared<boost::asio::io_service>();
    m_srvWork   = boost::make_shared<boost::asio::io_service::work>(*m_srv);
    m_thread    = boost::make_shared<boost::thread>(boost::bind(&boost::asio::io_service::run, m_srv));
}

SVTHEVCEncoder::~SVTHEVCEncoder()
{
    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

    m_srvWork.reset();
    m_srv->stop();
    m_thread.reset();
    m_srv.reset();

    if (!m_sendThreadExited) {
        m_sendThreadExited = true;
        m_queueCond.notify_all();
        m_sendThread.join();
    }

    m_dest = NULL;
}

bool SVTHEVCEncoder::canSimulcast(FrameFormat format, uint32_t width, uint32_t height)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    return false;
}

bool SVTHEVCEncoder::isIdle()
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    return (m_dest == NULL);
}

bool SVTHEVCEncoder::initEncoder(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds)
{
    //m_hevc_encoder->setDebugDump(true);

    m_encoderReady = m_hevc_encoder->init(width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds, 1, 8);
    if (m_encoderReady) {
        m_sendThread = boost::thread(&SVTHEVCEncoder::sendLoop, this);
        m_sendThreadExited = false;
    }

    return true;
}

bool SVTHEVCEncoder::initEncoderAsync(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds)
{
    m_srv->post(boost::bind(&SVTHEVCEncoder::InitEncoder, this, width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds));
    return true;
}

int32_t SVTHEVCEncoder::generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, owt_base::FrameDestination* dest)
{
    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

    ELOG_INFO_T("generateStream: {.width=%d, .height=%d, .frameRate=%d, .bitrateKbps=%d, .keyFrameIntervalSeconds=%d}"
            , width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);

    if (m_dest) {
        ELOG_ERROR_T("Only support one stream!");
        return -1;
    }

    m_width = width;
    m_height = height;
    m_frameRate = frameRate;
    m_bitrateKbps = bitrateKbps;
    m_keyFrameIntervalSeconds = keyFrameIntervalSeconds;

    if (m_width != 0 && m_height != 0) {
        if (!initEncoderAsync(m_width, m_height, m_frameRate, m_bitrateKbps, m_keyFrameIntervalSeconds))
            return -1;
    }

    m_dest = dest;

    return 0;
}

void SVTHEVCEncoder::degenerateStream(int32_t streamId)
{
    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

    ELOG_DEBUG_T("degenerateStream");

    m_dest = NULL;
}

void SVTHEVCEncoder::setBitrate(unsigned short kbps, int32_t streamId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_WARN_T("%s", __FUNCTION__);
}

void SVTHEVCEncoder::requestKeyFrame(int32_t streamId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG_T("%s", __FUNCTION__);

    m_hevc_encoder->requestKeyFrame();
}

void SVTHEVCEncoder::onFrame(const Frame& frame)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    int32_t ret;

    LTRACE_ASYNC_BEGIN("SVTHEVCEncoder", frame.timeStamp);

    if (m_dest == NULL) {
        return;
    }

    if (m_width == 0 || m_height == 0) {
        m_width = frame.additionalInfo.video.width;
        m_height = frame.additionalInfo.video.height;

        if (m_bitrateKbps == 0)
            m_bitrateKbps = calcBitrate(m_width, m_height, m_frameRate);

        if (!initEncoderAsync(m_width, m_height, m_frameRate, m_bitrateKbps, m_keyFrameIntervalSeconds)) {
            return;
        }
    }

    if (!m_encoderReady) {
        ELOG_WARN_T("Encoder not ready!");
        return;
    }

    ret = m_hevc_encoder->sendFrame(frame);
    if (!ret) {
        ELOG_ERROR_T("SendPicture failed");
        return;
    }

    while (true) {
        boost::shared_ptr<SVTHEVCEncodedPacket> encoded_pkt = m_hevc_encoder->getEncodedPacket();
        if(!encoded_pkt)
            break;

        {
            boost::mutex::scoped_lock lock(m_queueMutex);
            m_packet_queue.push(encoded_pkt);

            if (m_packet_queue.size() == 1)
                m_queueCond.notify_all();
        }
    }
}

void SVTHEVCEncoder::fillPacketDone(boost::shared_ptr<SVTHEVCEncodedPacket> encoded_pkt)
{
    Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));
    outFrame.format     = FRAME_FORMAT_H265;
    outFrame.payload    = encoded_pkt->data;
    outFrame.length     = encoded_pkt->length;
    outFrame.timeStamp = (m_encoded_frame_count++) * 1000 / m_frameRate * 90;
    outFrame.orig_timeStamp = encoded_pkt->pts;
    outFrame.additionalInfo.video.width         = m_width;
    outFrame.additionalInfo.video.height        = m_height;
    outFrame.additionalInfo.video.isKeyFrame    = encoded_pkt->isKey;

    ELOG_TRACE_T("deliverFrame, %s, %dx%d(%s), length(%d), timestamp_ms(%u), timestamp(%u)",
            getFormatStr(outFrame.format),
            outFrame.additionalInfo.video.width,
            outFrame.additionalInfo.video.height,
            outFrame.additionalInfo.video.isKeyFrame ? "key" : "delta",
            outFrame.length,
            outFrame.timeStamp / 90,
            outFrame.timeStamp);

    m_dest->onFrame(outFrame);

    LTRACE_ASYNC_END("SVTHEVCEncoder", outFrame.orig_timeStamp);
}

void SVTHEVCEncoder::sendLoop()
{
    ELOG_INFO_T("Start sendLoop");

    while(!m_sendThreadExited) {
        boost::mutex::scoped_lock lock(m_queueMutex);
        while (m_packet_queue.size() == 0) {
            if (m_sendThreadExited)
                break;

            m_queueCond.wait(lock);
        }

        if (m_sendThreadExited)
            break;

        boost::shared_ptr<SVTHEVCEncodedPacket> pkt = m_packet_queue.front();
        m_packet_queue.pop();

        fillPacketDone(pkt);
    }

    ELOG_INFO_T("Stop sendLoop");
}

} // namespace owt_base
