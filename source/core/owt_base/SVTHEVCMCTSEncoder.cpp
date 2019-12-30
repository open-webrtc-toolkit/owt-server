// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "SVTHEVCMCTSEncoder.h"

#include <webrtc/api/video/video_frame.h>
#include <webrtc/api/video/video_frame_buffer.h>

#include <libyuv/convert.h>
#include <libyuv/planar_functions.h>
#include <libyuv/scale.h>

#include "MediaUtilities.h"

namespace owt_base {

DEFINE_LOGGER(SVTHEVCMCTSEncoder, "owt.SVTHEVCMCTSEncoder");

SVTHEVCMCTSEncoder::SVTHEVCMCTSEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast)
    : m_encoderReady(false)
    , m_dest(NULL)
    , m_width_hi(0)
    , m_height_hi(0)
    , m_width_low(0)
    , m_height_low(0)
    , m_frameRate(0)
    , m_bitrateKbps(0)
    , m_keyFrameIntervalSeconds(0)
    , m_encodedFrameCount(0)
    , m_hi_res_pending_call(0)
    , m_low_res_pending_call(0)
    , m_payload_buffer(NULL)
    , m_payload_buffer_length(0)
    , m_sendThreadExited(true)
{
    m_hi_res_encoder = boost::make_shared<SVTHEVCEncoderBase>();
    m_low_res_encoder = boost::make_shared<SVTHEVCEncoderBase>();

    m_srv       = boost::make_shared<boost::asio::io_service>();
    m_srvWork   = boost::make_shared<boost::asio::io_service::work>(*m_srv);
    m_thread    = boost::make_shared<boost::thread>(boost::bind(&boost::asio::io_service::run, m_srv));
}

SVTHEVCMCTSEncoder::~SVTHEVCMCTSEncoder()
{
    m_srvWork.reset();
    m_srv->stop();
    m_srv.reset();
    m_thread.reset();

    if (!m_sendThreadExited) {
        m_sendThreadExited = true;
        m_queueCond.notify_all();
        m_sendThread.join();
    }

    if (m_payload_buffer) {
        free(m_payload_buffer);
    }

    m_dest = NULL;
}

bool SVTHEVCMCTSEncoder::canSimulcast(FrameFormat format, uint32_t width, uint32_t height)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    return false;
}

bool SVTHEVCMCTSEncoder::isIdle()
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    return (m_dest == NULL);
}

bool SVTHEVCMCTSEncoder::initEncoder(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds)
{
    uint32_t bitrate_hi;
    uint32_t tiles_w_hi;
    uint32_t tiles_h_hi;

    uint32_t bitrate_low;
    uint32_t tiles_w_low;
    uint32_t tiles_h_low;

    if (width == 7680 && height == 3840) {
        m_width_hi = 7680;
        m_height_hi = 3840;
        bitrate_hi = 100000 * 30 / frameRate;
        tiles_w_hi = 12;
        tiles_h_hi = 6;

        m_width_low = 512;
        m_height_low = 1280;
        bitrate_low = calcBitrate(m_width_low, m_height_low, frameRate);
        tiles_w_low = 2;
        tiles_h_low = 2;
    } else if (width == 3840 && height == 2048) {
        m_width_hi = 3840;
        m_height_hi = 2048;
        bitrate_hi = 25000 * 30 / frameRate;
        tiles_w_hi = 10;
        tiles_h_hi = 8;

        m_width_low = 1280;
        m_height_low = 768;
        bitrate_low = calcBitrate(m_width_low, m_height_low, frameRate);
        tiles_w_low = 5;
        tiles_h_low = 3;
    } else {
        ELOG_ERROR_T("Resolution is not supported, %dx%d", width, height);
        return false;
    }

    ELOG_INFO_T("hi-res %dx%d, tiles %dx%d, bitrate %d", m_width_hi, m_height_hi, tiles_w_hi, tiles_h_hi, bitrate_hi);
    ELOG_INFO_T("low-res %dx%d, tiles %dx%d, bitrate %d", m_width_low, m_height_low, tiles_w_low, tiles_h_low, bitrate_low);

    int ready = false;

    //m_hi_res_encoder->setDebugDump(true);
    ready = m_hi_res_encoder->init(m_width_hi, m_height_hi, frameRate, bitrate_hi, keyFrameIntervalSeconds, tiles_w_hi, tiles_h_hi);
    if (!ready)
        return false;
    m_hi_res_encoder->register_listener(this);

    //m_low_res_encoder->setDebugDump(true);
    ready = m_low_res_encoder->init(m_width_low, m_height_low, frameRate, bitrate_low, keyFrameIntervalSeconds, tiles_w_low, tiles_h_low);
    if (!ready)
        return false;
    m_low_res_encoder->register_listener(this);

    m_sendThread = boost::thread(&SVTHEVCMCTSEncoder::sendLoop, this);
    m_sendThreadExited = false;

    m_encoderReady = true;

    return true;
}

bool SVTHEVCMCTSEncoder::initEncoderAsync(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds)
{
    m_srv->post(boost::bind(&SVTHEVCMCTSEncoder::InitEncoder, this, width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds));
    return true;
}

int32_t SVTHEVCMCTSEncoder::generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, owt_base::FrameDestination* dest)
{
    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

    ELOG_INFO_T("generateStream: {.width=%d, .height=%d, .frameRate=%d, .bitrateKbps=%d, .keyFrameIntervalSeconds=%d}"
            , width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);

    if (m_dest) {
        ELOG_ERROR_T("Only support one stream!");
        return -1;
    }

    m_frameRate = frameRate;
    m_bitrateKbps = bitrateKbps;
    m_keyFrameIntervalSeconds = keyFrameIntervalSeconds;

    if (width != 0 && height != 0) {
        if (!initEncoderAsync(width, height, m_frameRate, m_bitrateKbps, m_keyFrameIntervalSeconds))
            return -1;
    }

    m_dest = dest;

    return 0;
}

void SVTHEVCMCTSEncoder::degenerateStream(int32_t streamId)
{
    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

    ELOG_DEBUG_T("degenerateStream");

    m_dest = NULL;
}

void SVTHEVCMCTSEncoder::setBitrate(unsigned short kbps, int32_t streamId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_WARN_T("%s", __FUNCTION__);
}

void SVTHEVCMCTSEncoder::requestKeyFrame(int32_t streamId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG_T("%s", __FUNCTION__);
    //todo
}

void SVTHEVCMCTSEncoder::onFrame(const Frame& frame)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    if (frame.format != FRAME_FORMAT_I420) {
        ELOG_ERROR_T("Frame format not supported");
        return;
    }

    if (m_dest == NULL) {
        return;
    }

    if (!m_encoderReady) {
        ELOG_WARN_T("Encoder not ready!");
        return;
    }

    if (m_hi_res_pending_call > 1 || m_low_res_pending_call > 1) {
        ELOG_WARN_T("Too many pending jobs");
        return;
    }

    webrtc::VideoFrame *videoFrame = reinterpret_cast<webrtc::VideoFrame*>(frame.payload);
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> videoBuffer = videoFrame->video_frame_buffer();

    m_hi_res_encoder->sendVideoBuffer_async(videoBuffer, videoFrame->timestamp_us(), videoFrame->timestamp_us());
    m_low_res_encoder->sendVideoBuffer_async(videoBuffer, videoFrame->timestamp_us(), videoFrame->timestamp_us());

    m_hi_res_pending_call++;
    m_low_res_pending_call++;
}

void SVTHEVCMCTSEncoder::onEncodedPacket(SVTHEVCEncoderBase *encoder, boost::shared_ptr<SVTHEVCEncodedPacket> pkt)
{
    if (encoder == m_hi_res_encoder.get()) {
        m_hi_res_pending_call--;

        if (pkt) {
            boost::mutex::scoped_lock lock(m_queueMutex);
            m_hi_res_packet_queue.push(pkt);

            if (m_hi_res_packet_queue.size() == 1)
                m_queueCond.notify_all();
        }
    } else {
        m_low_res_pending_call--;

        if (pkt) {
            boost::mutex::scoped_lock lock(m_queueMutex);
            m_low_res_packet_queue.push(pkt);

            if (m_low_res_packet_queue.size() == 1)
                m_queueCond.notify_all();
        }
    }
}

void SVTHEVCMCTSEncoder::fillPacketsDone(boost::shared_ptr<SVTHEVCEncodedPacket> hi_res_pkt, boost::shared_ptr<SVTHEVCEncodedPacket> low_res_pkt)
{
    uint32_t length = 12 + hi_res_pkt->length + 12 + low_res_pkt->length;

    while (m_payload_buffer_length < length) {
        if (m_payload_buffer) {
            m_payload_buffer_length = m_width_hi * m_height_hi * 2;
            m_payload_buffer = (uint8_t *)malloc(m_payload_buffer_length);
            continue;
        }

        m_payload_buffer_length *= 2;
        m_payload_buffer = (uint8_t *)realloc(m_payload_buffer, m_payload_buffer_length);
    }

    uint8_t *payload = m_payload_buffer;

    int offset = 0;

    // hi_res
    payload[offset + 0] = m_width_hi & 0xff;
    payload[offset + 1] = (m_width_hi >> 8) & 0xff;
    payload[offset + 2] = (m_width_hi >> 16) & 0xff;
    payload[offset + 3] = (m_width_hi >> 24) & 0xff;
    offset += 4;

    payload[offset + 0] = m_height_hi & 0xff;
    payload[offset + 1] = (m_height_hi >> 8) & 0xff;
    payload[offset + 2] = (m_height_hi >> 16) & 0xff;
    payload[offset + 3] = (m_height_hi >> 24) & 0xff;
    offset += 4;

    payload[offset + 0] = hi_res_pkt->length & 0xff;
    payload[offset + 1] = (hi_res_pkt->length >> 8) & 0xff;
    payload[offset + 2] = (hi_res_pkt->length >> 16) & 0xff;
    payload[offset + 3] = (hi_res_pkt->length >> 24) & 0xff;
    offset += 4;

    memcpy(payload + offset, hi_res_pkt->data, hi_res_pkt->length);
    offset += hi_res_pkt->length;

    // low_res
    payload[offset + 0] = m_width_low & 0xff;
    payload[offset + 1] = (m_width_low >> 8) & 0xff;
    payload[offset + 2] = (m_width_low >> 16) & 0xff;
    payload[offset + 3] = (m_width_low >> 24) & 0xff;
    offset += 4;

    payload[offset + 0] = m_height_low & 0xff;
    payload[offset + 1] = (m_height_low >> 8) & 0xff;
    payload[offset + 2] = (m_height_low >> 16) & 0xff;
    payload[offset + 3] = (m_height_low >> 24) & 0xff;
    offset += 4;

    payload[offset + 0] = low_res_pkt->length & 0xff;
    payload[offset + 1] = (low_res_pkt->length >> 8) & 0xff;
    payload[offset + 2] = (low_res_pkt->length >> 16) & 0xff;
    payload[offset + 3] = (low_res_pkt->length >> 24) & 0xff;
    offset += 4;

    memcpy(payload + offset, low_res_pkt->data, low_res_pkt->length);
    offset += low_res_pkt->length;

    // out
    Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));
    outFrame.format     = FRAME_FORMAT_HEVC_MCTS;
    outFrame.payload    = payload;
    outFrame.length     = length;
    outFrame.timeStamp = (m_encodedFrameCount++) * 1000 / m_frameRate * 90;
    outFrame.additionalInfo.video.width         = m_width_hi;
    outFrame.additionalInfo.video.height        = m_height_hi;
    outFrame.additionalInfo.video.isKeyFrame    = hi_res_pkt->isKey;

    ELOG_TRACE_T("deliverFrame %s, hi-res %dx%d(%s), length(%d), low-res (%s), length(%d) ",
            getFormatStr(outFrame.format),
            outFrame.additionalInfo.video.width,
            outFrame.additionalInfo.video.height,
            hi_res_pkt->isKey ? "key" : "delta",
            hi_res_pkt->length,
            low_res_pkt->isKey ? "key" : "delta",
            low_res_pkt->length
            );

    m_dest->onFrame(outFrame);
}

void SVTHEVCMCTSEncoder::sendLoop()
{
    ELOG_INFO_T("Start sendLoop");

    while(!m_sendThreadExited) {
        boost::mutex::scoped_lock lock(m_queueMutex);
        while (m_hi_res_packet_queue.size() == 0
                || m_low_res_packet_queue.size() == 0) {
            if (m_sendThreadExited)
                break;

            m_queueCond.wait(lock);
        }

        if (m_sendThreadExited)
            break;

        boost::shared_ptr<SVTHEVCEncodedPacket> hi_res = m_hi_res_packet_queue.front();
        m_hi_res_packet_queue.pop();

        boost::shared_ptr<SVTHEVCEncodedPacket> low_res = m_low_res_packet_queue.front();
        m_low_res_packet_queue.pop();

        fillPacketsDone(hi_res, low_res);
    }

    ELOG_INFO_T("Stop sendLoop");
}

} // namespace owt_base
