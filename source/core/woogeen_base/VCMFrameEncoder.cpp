/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#include "VCMFrameEncoder.h"

#include "MediaUtilities.h"
#include <webrtc/modules/video_coding/codecs/vp8/vp8_factory.h>

#ifdef ENABLE_YAMI
#include "YamiVideoFrame.h"
#endif

#ifdef ENABLE_MSDK
#include "MsdkFrame.h"
#endif

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(VCMFrameEncoder, "woogeen.VCMFrameEncoder");

VCMFrameEncoder::VCMFrameEncoder(FrameFormat format, boost::shared_ptr<WebRTCTaskRunner> taskRunner, bool useSimulcast)
    : m_streamId(0)
    , m_vcm(VideoCodingModule::Create())
    , m_encodeFormat(format)
    , m_taskRunner(taskRunner)
    , m_running(false)
    , m_incomingFrameCount(0)
{
    webrtc::VP8EncoderFactoryConfig::set_use_simulcast_adapter(useSimulcast);
    m_vcm->InitializeSender();
    m_vcm->RegisterTransportCallback(this);
    m_vcm->EnableFrameDropper(false);
    if (m_taskRunner)
        m_taskRunner->RegisterModule(m_vcm);

    //m_jobTimer.reset(new JobTimer(30, this));
    //m_jobTimer->start();

    m_running = true;
    m_thread = boost::thread(&VCMFrameEncoder::encodeLoop, this);
}

VCMFrameEncoder::~VCMFrameEncoder()
{
    //m_jobTimer->stop();

    m_running = false;
    m_thread.join();

    if (m_taskRunner)
        m_taskRunner->DeRegisterModule(m_vcm);

    VideoCodingModule::Destroy(m_vcm);
    m_vcm = nullptr;
    m_streamId = 0;
}

bool VCMFrameEncoder::canSimulcast(FrameFormat format, uint32_t width, uint32_t height)
{
    VideoCodec videoCodec;
    videoCodec = m_vcm->GetSendCodec();
    return false
           &&webrtc::VP8EncoderFactoryConfig::use_simulcast_adapter()
           && m_encodeFormat == format
           && videoCodec.width * height == videoCodec.height * width;
}

bool VCMFrameEncoder::isIdle()
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    return m_streams.size() == 0;
}

int32_t VCMFrameEncoder::generateStream(uint32_t width, uint32_t height, uint32_t bitrateKbps, woogeen_base::FrameDestination* dest)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);

    VideoCodec videoCodec;
    uint8_t simulcastId {0};
    if (m_streams.size() > 0) {
        videoCodec = m_vcm->GetSendCodec();
        for (int i = 0; i < videoCodec.numberOfSimulcastStreams; ++i) {
            if (videoCodec.simulcastStream[i].width == width) {
                simulcastId = i;
                boost::shared_ptr<EncodeOut> encodeOut;
                encodeOut.reset(new EncodeOut(m_streamId, this, dest));
                OutStream stream = {.width = width, .height = height, .simulcastId = simulcastId, .encodeOut = encodeOut};
                boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
                m_streams[m_streamId] = stream;
                ELOG_DEBUG_T("generateStream: {.width=%d, .height=%d}, simulcastId=%d", width, height, simulcastId);
                return m_streamId++;
            }
        }
        if (videoCodec.numberOfSimulcastStreams >= kMaxSimulcastStreams)
            return -1;
    } else {
        ELOG_DEBUG_T("Create encoder(%s)", getFormatStr(m_encodeFormat));
        switch (m_encodeFormat) {
        case FRAME_FORMAT_VP8:
            if (VideoCodingModule::Codec(kVideoCodecVP8, &videoCodec) != VCM_OK) {
                ELOG_ERROR_T("Error create encoder(%s)", getFormatStr(m_encodeFormat));
                return -1;
            }
            break;
        case FRAME_FORMAT_VP9:
            if (VideoCodingModule::Codec(kVideoCodecVP9, &videoCodec) != VCM_OK) {
                ELOG_ERROR_T("Error create encoder(%s)", getFormatStr(m_encodeFormat));
                return -1;
            }
            break;
        case FRAME_FORMAT_H264:
            if (VideoCodingModule::Codec(kVideoCodecH264, &videoCodec) != VCM_OK) {
                ELOG_ERROR_T("Error create encoder(%s)", getFormatStr(m_encodeFormat));
                return -1;
            }
            break;
        case FRAME_FORMAT_I420:
        default:
            ELOG_ERROR_T("Invalid encoder(%s)", getFormatStr(m_encodeFormat));
            return -1;
        }
    }

    VideoCodec fbVideoCodec = videoCodec;
    uint32_t targetKbps = bitrateKbps * (m_encodeFormat == FRAME_FORMAT_VP8 ? 0.9 : 1);

    for (; simulcastId < videoCodec.numberOfSimulcastStreams; ++simulcastId) {
        if (videoCodec.simulcastStream[simulcastId].width > width)
            break;
    }
    if (simulcastId < videoCodec.numberOfSimulcastStreams) {
        uint8_t j = videoCodec.numberOfSimulcastStreams;
        while (j > simulcastId) {
            memcpy(&videoCodec.simulcastStream[j], &videoCodec.simulcastStream[j-1], sizeof(SimulcastStream));
            --j;
        }
    } else {
        videoCodec.width = width;
        videoCodec.height = height;
    }

    videoCodec.simulcastStream[simulcastId] = SimulcastStream{
        .width = static_cast<short unsigned>(width),
        .height = static_cast<short unsigned>(height),
        .numberOfTemporalLayers = 0,
        .maxBitrate = targetKbps,
        .targetBitrate = targetKbps,
        .minBitrate = targetKbps / 4,
        .qpMax = videoCodec.qpMax
    };
    ++videoCodec.numberOfSimulcastStreams;

    uint32_t newTargetBitrate = videoCodec.simulcastStream[videoCodec.numberOfSimulcastStreams-1].targetBitrate;
    uint32_t newMaxBitrate = videoCodec.simulcastStream[videoCodec.numberOfSimulcastStreams-1].maxBitrate;
    uint32_t newMinBitrate = videoCodec.simulcastStream[videoCodec.numberOfSimulcastStreams-1].minBitrate;
    for (uint8_t i = 0; i < videoCodec.numberOfSimulcastStreams-1; ++i) {
        newTargetBitrate += videoCodec.simulcastStream[i].targetBitrate;
        newMaxBitrate += videoCodec.simulcastStream[i].maxBitrate;
        newMinBitrate += videoCodec.simulcastStream[i].targetBitrate;
    }
    videoCodec.startBitrate = newMaxBitrate;
    videoCodec.targetBitrate = newTargetBitrate;
    videoCodec.maxBitrate = newMaxBitrate;
    videoCodec.minBitrate = newMinBitrate;

    int32_t ret = m_vcm->RegisterSendCodec(&videoCodec, 1, 1400);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    if (ret != VCM_OK) {
        ELOG_ERROR_T("RegisterSendCodec error: %d", ret);
        ELOG_DEBUG_T("Try fallback to previous video codec");
        m_vcm->RegisterSendCodec(&fbVideoCodec, 1, 1400);
        return -1;
    } else {
        if (simulcastId != videoCodec.numberOfSimulcastStreams-1) {
            for (auto it = m_streams.begin(); it != m_streams.end(); ++it) {
                if (it->second.simulcastId >= simulcastId) {
                    it->second.simulcastId++;
                }
            }
        }
    }

    boost::shared_ptr<EncodeOut> encodeOut;
    encodeOut.reset(new EncodeOut(m_streamId, this, dest));
    OutStream stream = {.width = width, .height = height, .simulcastId = simulcastId, .encodeOut = encodeOut};
    m_streams[m_streamId] = stream;
    ELOG_DEBUG_T("generateStream: {.width=%d, .height=%d, .bitrateKbps=%d}, simulcastId=%d", width, height, bitrateKbps, simulcastId);
    return m_streamId++;
}

void VCMFrameEncoder::degenerateStream(int32_t streamId)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG_T("degenerateStream(%d)", streamId);

    auto it = m_streams.find(streamId);
    if (it != m_streams.end()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_streams.erase(streamId);
    }
}

void VCMFrameEncoder::setBitrate(unsigned short kbps, int32_t streamId)
{
    int bps = kbps * 1000;
    // TODO: Notify VCM about the packet lost and rtt information.
    m_vcm->SetChannelParameters(bps, 0, 0);
}

void VCMFrameEncoder::requestKeyFrame(int32_t streamId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG_T("requestKeyFrame(%d)", streamId);

    auto it = m_streams.find(streamId);
    if (it != m_streams.end()) {
        m_vcm->IntraFrameRequest(it->second.simulcastId);
    }
}

void VCMFrameEncoder::onFrame(const Frame& frame)
{
    if (!m_bufferManager && isVideoFrame(frame)) {
        m_bufferManager.reset(new BufferManager(1, frame.additionalInfo.video.width, frame.additionalInfo.video.height));
        m_bufferManager->setActive(0, true);
    }

    I420VideoFrame* freeFrame = m_bufferManager ? m_bufferManager->getFreeBuffer() : nullptr;
    if (!freeFrame)
        return;

    I420VideoFrame* busyFrame = nullptr;

    switch (frame.format) {
    case FRAME_FORMAT_I420: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return;
        // Currently we should only receive I420 format frame.
        I420VideoFrame* rawFrame = reinterpret_cast<I420VideoFrame*>(frame.payload);
        const int kMsToRtpTimestamp = 90;
        const uint32_t time_stamp =
            kMsToRtpTimestamp *
            static_cast<uint32_t>(rawFrame->render_time_ms());
        rawFrame->set_timestamp(time_stamp);
        freeFrame->CopyFrame(*rawFrame);
        break;
    }
#ifdef ENABLE_YAMI
    case FRAME_FORMAT_YAMI: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return;

        I420VideoFrame rawFrame;
        YamiVideoFrame yamiFrame = *(reinterpret_cast<YamiVideoFrame*>(frame.payload));
        if (!yamiFrame.convertToI420VideoFrame(rawFrame))
            return;

        freeFrame->CopyFrame(rawFrame);
        break;
    }
#endif
#ifdef ENABLE_MSDK
    case FRAME_FORMAT_MSDK: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return;

        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
        if (!holder->frame->convertTo(*freeFrame))
            return;

        freeFrame->set_timestamp(frame.timeStamp);
        break;
    }
#endif
    case FRAME_FORMAT_VP8:
    case FRAME_FORMAT_VP9:
    case FRAME_FORMAT_H264:
        assert(false);
    default:
        m_bufferManager->releaseBuffer(freeFrame);
        return;
    }

    busyFrame = m_bufferManager->postFreeBuffer(freeFrame, 0);
    if (busyFrame)
        m_bufferManager->releaseBuffer(busyFrame);

    encodeOneFrame();
}

void VCMFrameEncoder::doEncoding()
{
    if (!m_bufferManager)
        return;

    webrtc::I420VideoFrame* rawFrame = m_bufferManager->getBusyBuffer(0);
    if (!rawFrame)
        return;

    m_vcm->AddVideoFrame(*rawFrame);

    m_bufferManager->releaseBuffer(rawFrame);
}

void VCMFrameEncoder::encodeLoop()
{
    while (m_running) {
        boost::mutex::scoped_lock lock(m_encMutex);
        while (m_incomingFrameCount == 0) {
            m_encCond.wait(lock);
        }
        m_incomingFrameCount--;
        m_encMutex.unlock();

        if (m_running)
            doEncoding();
    }

    ELOG_TRACE_T("Thread exited!");
}

void VCMFrameEncoder::encodeOneFrame()
{
    boost::mutex::scoped_lock lock(m_encMutex);

    m_incomingFrameCount++;
    m_encCond.notify_one();

    if (m_incomingFrameCount > 100)
        ELOG_WARN_T("Too many pending frames(%d)", m_incomingFrameCount);
}

void VCMFrameEncoder::onTimeout()
{
    encodeOneFrame();
}

int32_t VCMFrameEncoder::SendData(
    uint8_t payloadType,
    const webrtc::EncodedImage& encoded_image,
    const RTPFragmentationHeader& fragmentationHeader,
    const RTPVideoHeader* rtpVideoHdr)
{
    // New encoded data, hand over to the frame consumer.
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (!m_streams.empty()) {
        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = m_encodeFormat;
        frame.payload = encoded_image._buffer;
        frame.length = encoded_image._length;
        frame.timeStamp = encoded_image._timeStamp;
        frame.additionalInfo.video.width = encoded_image._encodedWidth;
        frame.additionalInfo.video.height = encoded_image._encodedHeight;

        ELOG_TRACE_T("SendData(%d), %s, %dx%d, length(%d)",
                rtpVideoHdr->simulcastIdx,
                getFormatStr(frame.format),
                frame.additionalInfo.video.width,
                frame.additionalInfo.video.height ,
                frame.length);

        auto it = m_streams.begin();
        for (; it != m_streams.end(); ++it) {
            if (it->second.encodeOut.get() && it->second.simulcastId == rtpVideoHdr->simulcastIdx)
                it->second.encodeOut->onEncoded(frame);
        }
        return 0;
    }

    return -1;
}

} // namespace woogeen_base
