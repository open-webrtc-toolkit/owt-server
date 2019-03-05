// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <boost/make_shared.hpp>

#include <webrtc/system_wrappers/include/cpu_info.h>
#include <webrtc/modules/video_coding/codec_database.h>

#include "VCMFrameEncoder.h"

#include "MediaUtilities.h"

#ifdef ENABLE_MSDK
#include "MsdkFrame.h"
#endif

using namespace webrtc;

namespace owt_base {

DEFINE_LOGGER(VCMFrameEncoder, "owt.VCMFrameEncoder");

VCMFrameEncoder::VCMFrameEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast)
    : m_streamId(0)
    , m_encodeFormat(format)
    , m_profile(profile)
    , m_requestKeyFrame(false)
    , m_updateBitrateKbps(0)
    , m_isAdaptiveMode(false)
    , m_width(0)
    , m_height(0)
    , m_frameRate(0)
    , m_bitrateKbps(0)
    , m_enableBsDump(false)
    , m_bsDumpfp(NULL)
{
    m_bufferManager.reset(new I420BufferManager(3));
    m_converter.reset(new FrameConverter());

    m_srv       = boost::make_shared<boost::asio::io_service>();
    m_srvWork   = boost::make_shared<boost::asio::io_service::work>(*m_srv);
    m_thread    = boost::make_shared<boost::thread>(boost::bind(&boost::asio::io_service::run, m_srv));
}

VCMFrameEncoder::~VCMFrameEncoder()
{
    m_srvWork.reset();
    m_srv->stop();
    m_thread.reset();
    m_srv.reset();

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

int32_t VCMFrameEncoder::generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, owt_base::FrameDestination* dest)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    uint32_t targetKbps = bitrateKbps;

    VideoCodec codecSettings;
    uint8_t simulcastId {0};
    int ret;

    assert(frameRate != 0);
    if (width == 0 || height == 0) {
        m_isAdaptiveMode = true;
        width = 3840;
        height = 2160;
        targetKbps = calcBitrate(width, height, frameRate);
    }

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
            if (m_profile != PROFILE_UNKNOWN) {
                ELOG_WARN_T("Don't support profile setting(%d)", m_profile);
            }

            m_encoder.reset(VP8Encoder::Create());

            VCMCodecDataBase::Codec(kVideoCodecVP8, &codecSettings);
            codecSettings.VP8()->resilience = kResilienceOff;
            codecSettings.VP8()->denoisingOn = false;
            codecSettings.VP8()->automaticResizeOn = false;
            codecSettings.VP8()->frameDroppingOn = false;
            codecSettings.VP8()->tl_factory = &tl_factory_;

            codecSettings.VP8()->keyFrameInterval = frameRate * keyFrameIntervalSeconds;
            break;
        case FRAME_FORMAT_VP9:
            if (m_profile != PROFILE_UNKNOWN) {
                ELOG_WARN_T("Don't support profile setting(%d)", m_profile);
            }

            m_encoder.reset(VP9Encoder::Create());

            VCMCodecDataBase::Codec(kVideoCodecVP9, &codecSettings);
            codecSettings.VP9()->numberOfTemporalLayers = 1;
            codecSettings.VP9()->numberOfSpatialLayers = 1;

            codecSettings.VP9()->keyFrameInterval = frameRate * keyFrameIntervalSeconds;
            break;
        case FRAME_FORMAT_H264:
            if (m_profile != PROFILE_AVC_CONSTRAINED_BASELINE) {
                ELOG_WARN_T("Only support profile (Constrained Baseline), required (%d)", m_profile);
            }

            m_encoder.reset(H264Encoder::Create(cricket::VideoCodec(cricket::kH264CodecName)));

            VCMCodecDataBase::Codec(kVideoCodecH264, &codecSettings);
            codecSettings.H264()->frameDroppingOn = true;

            codecSettings.H264()->keyFrameInterval = frameRate * keyFrameIntervalSeconds;
            break;
        default:
            ELOG_ERROR_T("Invalid encoder(%s)", getFormatStr(m_encodeFormat));
            return -1;
        }
    }

    codecSettings.startBitrate  = targetKbps;
    codecSettings.targetBitrate = targetKbps;
    codecSettings.maxBitrate    = targetKbps;
    codecSettings.maxFramerate  = frameRate;
    codecSettings.width         = width;
    codecSettings.height        = height;

    ret = m_encoder->InitEncode(&codecSettings, webrtc::CpuInfo::DetectNumberOfCores(), 0);
    if (ret) {
        ELOG_ERROR_T("Video encoder init faild.\n");
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
    ELOG_DEBUG_T("generateStream: {.width=%d, .height=%d, .frameRate=%d, .bitrateKbps=%d, .keyFrameIntervalSeconds=%d}, simulcastId=%d, adaptiveMode=%d"
            , width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds, simulcastId, m_isAdaptiveMode);

    m_width = width;
    m_height = height;
    m_frameRate = frameRate;
    m_bitrateKbps = bitrateKbps;

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
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG_T("setBitrate(%d), %d(kbps)", streamId, kbps);

    auto it = m_streams.find(streamId);
    if (it != m_streams.end()) {
        m_updateBitrateKbps = kbps;
    }
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
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    if (m_streams.size() == 0) {
        return;
    }

    boost::shared_ptr<webrtc::VideoFrame> videoFrame = frameConvert(frame);
    if (videoFrame == NULL) {
        return;
    }

    m_srv->post(boost::bind(&VCMFrameEncoder::Encode, this, videoFrame));
}

boost::shared_ptr<webrtc::VideoFrame> VCMFrameEncoder::frameConvert(const Frame& frame)
{
    int32_t dstFrameWidth = m_isAdaptiveMode ? frame.additionalInfo.video.width : m_width;
    int32_t dstFrameHeight = m_isAdaptiveMode ? frame.additionalInfo.video.height: m_height;

    rtc::scoped_refptr<webrtc::I420Buffer> rawBuffer = m_bufferManager->getFreeBuffer(dstFrameWidth, dstFrameHeight);
    if (!rawBuffer) {
        ELOG_ERROR_T("No valid buffer");
        return NULL;
    }

    boost::shared_ptr<webrtc::VideoFrame> dstFrame;

    switch (frame.format) {
    case FRAME_FORMAT_I420: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return NULL;

        VideoFrame *inputFrame = reinterpret_cast<VideoFrame*>(frame.payload);
        rtc::scoped_refptr<webrtc::VideoFrameBuffer> inputBuffer = inputFrame->video_frame_buffer();

        if (!m_converter->convert(inputBuffer.get(), rawBuffer.get())) {
            ELOG_ERROR_T("frameConverter failed");
            return NULL;
        }

        dstFrame.reset(new VideoFrame(rawBuffer, inputFrame->timestamp(), 0, webrtc::kVideoRotation_0));
        break;
    }
#ifdef ENABLE_MSDK
    case FRAME_FORMAT_MSDK: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return NULL;

        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
        boost::shared_ptr<MsdkFrame> msdkFrame = holder->frame;

        if (!m_converter->convert(msdkFrame.get(), rawBuffer.get())) {
            ELOG_ERROR_T("frameConverter failed");
            return NULL;
        }

        dstFrame.reset(new VideoFrame(rawBuffer, frame.timeStamp, 0, webrtc::kVideoRotation_0));
        break;
    }
#endif
    default:
        assert(false);
        return NULL;
    }

    return dstFrame;
}

void VCMFrameEncoder::encode(boost::shared_ptr<webrtc::VideoFrame> frame)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    int ret;

    if (m_streams.size() == 0) {
        return;
    }

    if (m_width != frame->width() || m_height != frame->height()) {
        ELOG_DEBUG_T("Update encoder resolution %dx%d->%dx%d", m_width, m_height, frame->width(), frame->height());

        ret = m_encoder->SetResolution(frame->width(), frame->height());
        if (ret != 0) {
            ELOG_WARN_T("Update Encode size error: %d", ret);
        }

        m_width = frame->width();
        m_height = frame->height();
        m_updateBitrateKbps = calcBitrate(m_width, m_height, m_frameRate);
    }

    if (m_updateBitrateKbps) {
        ELOG_DEBUG_T("Update encoder bitrate %d(kbps)->%d(kbps)", m_bitrateKbps, m_updateBitrateKbps.load());

        if (m_bitrateKbps != m_updateBitrateKbps) {
            BitrateAllocation bitrate;
            bitrate.SetBitrate(0, 0, m_updateBitrateKbps * 1000);

            ret = m_encoder->SetRateAllocation(bitrate, m_frameRate);
            if (ret != 0) {
                ELOG_WARN_T("Update Encode bitrate error: %d", ret);
            }
            m_bitrateKbps = m_updateBitrateKbps;
        }
        m_updateBitrateKbps = 0;
    }

    std::vector<FrameType> types;
    if (m_requestKeyFrame) {
        types.push_back(kVideoFrameKey);
        m_requestKeyFrame = false;
    }

    ret = m_encoder->Encode(*frame.get(), NULL, types.size() ? &types : NULL);
    if (ret != 0) {
        ELOG_ERROR_T("Encode frame error: %d", ret);
    }
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
        if (m_encodeFormat == FRAME_FORMAT_VP8 || m_encodeFormat == FRAME_FORMAT_VP9) {
            unsigned char mem[4];

            mem[0] = (len >>  0) & 0xff;
            mem[1] = (len >>  8) & 0xff;
            mem[2] = (len >> 16) & 0xff;
            mem[3] = (len >> 24) & 0xff;

            fwrite(&mem, 1, 4, m_bsDumpfp);
        }

        fwrite(buf, 1, len, m_bsDumpfp);
    }
}

} // namespace owt_base
