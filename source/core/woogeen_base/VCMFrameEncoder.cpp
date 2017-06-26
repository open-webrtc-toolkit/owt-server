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

#include <libyuv/convert.h>
#include <libyuv/planar_functions.h>
#include <libyuv/scale.h>

#include <webrtc/system_wrappers/include/cpu_info.h>
#include <webrtc/modules/video_coding/codec_database.h>

#include "VCMFrameEncoder.h"

#include "MediaUtilities.h"

#ifdef ENABLE_YAMI
#include "YamiVideoFrame.h"
#endif

#ifdef ENABLE_MSDK
#include "MsdkFrame.h"
#endif

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(VCMFrameEncoder, "woogeen.VCMFrameEncoder");

VCMFrameEncoder::VCMFrameEncoder(FrameFormat format, bool useSimulcast)
    : m_streamId(0)
    , m_encodeFormat(format)
    , m_running(false)
    , m_incomingFrameCount(0)
    , m_requestKeyFrame(false)
    , m_width(0)
    , m_height(0)
    , m_enableBsDump(false)
    , m_bsDumpfp(NULL)
{
    m_bufferManager.reset(new I420BufferManager(3));

    m_running = true;
    m_thread = boost::thread(&VCMFrameEncoder::encodeLoop, this);
}

VCMFrameEncoder::~VCMFrameEncoder()
{
    m_running = false;
    m_encCond.notify_one();
    m_thread.join();

    m_streamId = 0;

    if(m_encoder) {
        m_encoder->Release();
        m_encoder.reset();
    }

    if (m_bsDumpfp) {
        fclose(m_bsDumpfp);
    }
}

bool VCMFrameEncoder::canSimulcast(FrameFormat format, uint32_t width, uint32_t height)
{
#if 0
    VideoCodec videoCodec;
    videoCodec = m_vcm->GetSendCodec();
    return false
           &&webrtc::VP8EncoderFactoryConfig::use_simulcast_adapter()
           && m_encodeFormat == format
           && videoCodec.width * height == videoCodec.height * width;
#endif
    return false;
}

bool VCMFrameEncoder::isIdle()
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    return m_streams.size() == 0;
}

int32_t VCMFrameEncoder::generateStream(uint32_t width, uint32_t height, uint32_t bitrateKbps, woogeen_base::FrameDestination* dest)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    //uint32_t targetKbps = bitrateKbps * (m_encodeFormat == FRAME_FORMAT_VP8 ? 0.9 : 1);
    uint32_t targetKbps = bitrateKbps;

    VideoCodec codecSettings;
    uint8_t simulcastId {0};
    int ret;

#if 0
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
#endif
    {
        ELOG_DEBUG_T("Create encoder(%s)", getFormatStr(m_encodeFormat));
        switch (m_encodeFormat) {
        case FRAME_FORMAT_VP8:
            m_encoder.reset(VP8Encoder::Create());

            VCMCodecDataBase::Codec(kVideoCodecVP8, &codecSettings);
            codecSettings.VP8()->resilience = kResilienceOff;
            codecSettings.VP8()->denoisingOn = false;
            codecSettings.VP8()->automaticResizeOn = false;
            codecSettings.VP8()->frameDroppingOn = false;
            codecSettings.VP8()->tl_factory = &tl_factory_;
            break;
        case FRAME_FORMAT_VP9:
            m_encoder.reset(VP9Encoder::Create());

            VCMCodecDataBase::Codec(kVideoCodecVP9, &codecSettings);
            codecSettings.VP9()->numberOfTemporalLayers = 1;
            codecSettings.VP9()->numberOfSpatialLayers = 1;
            break;
        case FRAME_FORMAT_H264:
            m_encoder.reset(H264Encoder::Create(cricket::VideoCodec(cricket::kH264CodecName)));

            VCMCodecDataBase::Codec(kVideoCodecH264, &codecSettings);
            codecSettings.H264()->frameDroppingOn = true;
            break;
        default:
            ELOG_ERROR_T("Invalid encoder(%s)", getFormatStr(m_encodeFormat));
            return -1;
        }
    }

    codecSettings.startBitrate  = targetKbps;
    codecSettings.targetBitrate = targetKbps;
    codecSettings.maxBitrate    = targetKbps;
    codecSettings.maxFramerate  = 30;
    codecSettings.width         = width;
    codecSettings.height        = height;

    ret = m_encoder->InitEncode(&codecSettings, webrtc::CpuInfo::DetectNumberOfCores(), 0);
    if (ret) {
        printf("Video encoder init faild.\n");
        return -1;
    }

    m_encoder->RegisterEncodeCompleteCallback(this);

#if 0
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
#endif

    boost::shared_ptr<EncodeOut> encodeOut;
    encodeOut.reset(new EncodeOut(m_streamId, this, dest));
    OutStream stream = {.width = width, .height = height, .simulcastId = simulcastId, .encodeOut = encodeOut};
    m_streams[m_streamId] = stream;
    ELOG_DEBUG_T("generateStream: {.width=%d, .height=%d, .bitrateKbps=%d}, simulcastId=%d", width, height, bitrateKbps, simulcastId);

    m_width = width;
    m_height = height;

    if (m_enableBsDump) {
        char dumpFileName[128];

        snprintf(dumpFileName, 128, "/tmp/vcmFrameEncoder-%p-%d.%s", this, simulcastId, getFormatStr(m_encodeFormat));
        m_bsDumpfp = fopen(dumpFileName, "wb");
        if (m_bsDumpfp) {
            ELOG_DEBUG("Enable bitstream dump, %s", dumpFileName);
        } else {
            ELOG_DEBUG("Can not open dump file, %s", dumpFileName);
        }
    }

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
#if 0 //todo
    int bps = kbps * 1000;
    // TODO: Notify VCM about the packet lost and rtt information.
    m_vcm->SetChannelParameters(bps, 0, 0);
#endif
}

void VCMFrameEncoder::requestKeyFrame(int32_t streamId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG_T("requestKeyFrame(%d)", streamId);

    auto it = m_streams.find(streamId);
    if (it != m_streams.end()) {
        m_requestKeyFrame = true;
    }
}

void VCMFrameEncoder::onFrame(const Frame& frame)
{
    if (m_width == 0 || m_height == 0)
        return;

    rtc::scoped_refptr<webrtc::I420Buffer> rawBuffer = m_bufferManager->getFreeBuffer(m_width, m_height);
    if (!rawBuffer) {
        ELOG_ERROR("No valid buffer");
        return;
    }

    boost::shared_ptr<webrtc::VideoFrame> busyFrame;

    switch (frame.format) {
    case FRAME_FORMAT_I420: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return;

        VideoFrame *inputFrame = reinterpret_cast<VideoFrame*>(frame.payload);
        rtc::scoped_refptr<webrtc::VideoFrameBuffer> inputBuffer = inputFrame->video_frame_buffer();

        if (m_width == inputBuffer->width() && m_height == inputBuffer->height()) {
            libyuv::I420Copy(
                    inputBuffer->DataY(), inputBuffer->StrideY(),
                    inputBuffer->DataU(), inputBuffer->StrideU(),
                    inputBuffer->DataV(), inputBuffer->StrideV(),
                    rawBuffer->MutableDataY(), rawBuffer->StrideY(),
                    rawBuffer->MutableDataU(), rawBuffer->StrideU(),
                    rawBuffer->MutableDataV(), rawBuffer->StrideV(),
                    inputBuffer->width(), inputBuffer->height());
        } else {
            libyuv::I420Scale(
                    inputBuffer->DataY(),   inputBuffer->StrideY(),
                    inputBuffer->DataU(),   inputBuffer->StrideU(),
                    inputBuffer->DataV(),   inputBuffer->StrideV(),
                    inputBuffer->width(),   inputBuffer->height(),
                    rawBuffer->MutableDataY(),  rawBuffer->StrideY(),
                    rawBuffer->MutableDataU(),  rawBuffer->StrideU(),
                    rawBuffer->MutableDataV(),  rawBuffer->StrideV(),
                    rawBuffer->width(),         rawBuffer->height(),
                    libyuv::kFilterBox);
        }

        busyFrame.reset(new VideoFrame(rawBuffer, inputFrame->timestamp(), 0, webrtc::kVideoRotation_0));
        break;
    }
#ifdef ENABLE_YAMI
    case FRAME_FORMAT_YAMI: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return;

        I420VideoFrame rawFrame;
        YamiVideoFrame yamiFrame = *(reinterpret_cast<YamiVideoFrame*>(frame.payload));
        if (!yamiFrame.convertToI420VideoFrame(rawFrame)) {
            m_bufferManager->releaseBuffer(freeFrame);
            return;
        }

        freeFrame->CopyFrame(rawFrame);
        break;
    }
#endif
#ifdef ENABLE_MSDK
    case FRAME_FORMAT_MSDK: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return;

        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
        boost::shared_ptr<MsdkFrame> msdkFrame = holder->frame;
        if (msdkFrame->getCropW() == (uint32_t)m_width && msdkFrame->getCropH() == (uint32_t)m_height) {
            if (!msdkFrame->convertTo(rawBuffer)) {
                ELOG_ERROR("error convert msdk to I420Buffer");
                return;
            }
        } else {
            rtc::scoped_refptr<webrtc::I420Buffer> copyBuffer = m_bufferManager->getFreeBuffer(msdkFrame->getCropW(), msdkFrame->getCropH());
            if (!msdkFrame->convertTo(copyBuffer)) {
                ELOG_ERROR("error convert msdk to I420Buffer");
                return;
            }

            libyuv::I420Scale(
                    copyBuffer->DataY(),   copyBuffer->StrideY(),
                    copyBuffer->DataU(),   copyBuffer->StrideU(),
                    copyBuffer->DataV(),   copyBuffer->StrideV(),
                    copyBuffer->width(),   copyBuffer->height(),
                    rawBuffer->MutableDataY(),  rawBuffer->StrideY(),
                    rawBuffer->MutableDataU(),  rawBuffer->StrideU(),
                    rawBuffer->MutableDataV(),  rawBuffer->StrideV(),
                    rawBuffer->width(),         rawBuffer->height(),
                    libyuv::kFilterBox);
        }

        busyFrame.reset(new VideoFrame(rawBuffer, frame.timeStamp, 0, webrtc::kVideoRotation_0));
        break;
    }
#endif
    default:
        assert(false);
        return;
    }

    m_bufferManager->putBusyFrame(busyFrame);
    encodeOneFrame();
}

void VCMFrameEncoder::doEncoding()
{
    boost::shared_ptr<webrtc::VideoFrame> frame = m_bufferManager->getBusyFrame();
    if (!frame)
        return;

    int ret;
    std::vector<FrameType> types;
    if (m_requestKeyFrame) {
        types.push_back(kVideoFrameKey);
        m_requestKeyFrame = false;
    }

    ret = m_encoder->Encode(*frame.get(), NULL, types.size() ? &types : NULL);
    if (ret != 0) {
        ELOG_ERROR("Encode frame error: %d", ret);
    }
}

void VCMFrameEncoder::encodeLoop()
{
    while (true) {
        {
            boost::mutex::scoped_lock lock(m_encMutex);
            while (m_running && m_incomingFrameCount == 0) {
                m_encCond.wait(lock);
            }

            if (!m_running)
                break;

            m_incomingFrameCount--;
        }

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

webrtc::EncodedImageCallback::Result VCMFrameEncoder::OnEncodedImage(const EncodedImage& encoded_frame,
        const CodecSpecificInfo* codec_specific_info,
        const RTPFragmentationHeader* fragmentation)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (!m_streams.empty()) {
        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = m_encodeFormat;
        frame.payload = encoded_frame._buffer;
        frame.length = encoded_frame._length;
        frame.timeStamp = encoded_frame._timeStamp;
        frame.additionalInfo.video.width = encoded_frame._encodedWidth;
        frame.additionalInfo.video.height = encoded_frame._encodedHeight;
        frame.additionalInfo.video.isKeyFrame = (encoded_frame._frameType == kVideoFrameKey);

        ELOG_TRACE_T("SendData, %s, %dx%d, %s, length(%d), timestamp %d",
                getFormatStr(frame.format),
                frame.additionalInfo.video.width,
                frame.additionalInfo.video.height,
                frame.additionalInfo.video.isKeyFrame ? "key" : "delta",
                frame.length,
                frame.timeStamp / 90
                );

        dump(frame.payload, frame.length);

        auto it = m_streams.begin();
        for (; it != m_streams.end(); ++it) {
            if (it->second.encodeOut.get() && it->second.simulcastId == 0)
                it->second.encodeOut->onEncoded(frame);
        }
    }

    return webrtc::EncodedImageCallback::Result(webrtc::EncodedImageCallback::Result::OK);
}

void VCMFrameEncoder::dump(uint8_t *buf, int len)
{
    if (m_bsDumpfp) {
        fwrite(buf, 1, len, m_bsDumpfp);
    }
}

} // namespace woogeen_base
