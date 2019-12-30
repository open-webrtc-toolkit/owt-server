// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "SVTHEVCEncoderBase.h"

#include <libyuv/convert.h>
#include <libyuv/planar_functions.h>
#include <libyuv/scale.h>

#include "MediaUtilities.h"

namespace owt_base {

DEFINE_LOGGER(SVTHEVCEncoderBase, "owt.SVTHEVCEncoderBase");

SVTHEVCEncodedPacket::SVTHEVCEncodedPacket(EB_BUFFERHEADERTYPE *pBuffer)
    : data(NULL)
    , length(0)
    , isKey(false)
{
    if (pBuffer && pBuffer->pBuffer && pBuffer->nFilledLen) {
        m_array.reset(new uint8_t[pBuffer->nFilledLen]);
        memcpy(m_array.get(), pBuffer->pBuffer, pBuffer->nFilledLen);

        data = m_array.get();
        length = pBuffer->nFilledLen;
        isKey = (pBuffer->sliceType == EB_IDR_PICTURE);
        pts = pBuffer->pts;
    }
}

SVTHEVCEncodedPacket::~SVTHEVCEncodedPacket()
{
}

SVTHEVCEncoderBase::SVTHEVCEncoderBase()
    : m_handle(NULL)
    , m_scaling_frame_buffer(NULL)
    , m_forceIDR(false)
    , m_enableBsDump(false)
    , m_bsDumpfp(NULL)
{
    memset(&m_encParameters, 0, sizeof(m_encParameters));

    m_srv       = boost::make_shared<boost::asio::io_service>();
    m_srvWork   = boost::make_shared<boost::asio::io_service::work>(*m_srv);
    m_thread    = boost::make_shared<boost::thread>(boost::bind(&boost::asio::io_service::run, m_srv));
}

SVTHEVCEncoderBase::~SVTHEVCEncoderBase()
{
    m_srvWork.reset();
    m_srv->stop();
    m_thread.reset();
    m_srv.reset();

    if (m_handle) {
        EbDeinitEncoder(m_handle);
        EbDeinitHandle(m_handle);
        m_handle = NULL;

        deallocateBuffers();

        if (m_scaling_frame_buffer) {
            free(m_scaling_frame_buffer);
            m_scaling_frame_buffer = NULL;
        }

        if (m_bsDumpfp) {
            fclose(m_bsDumpfp);
            m_bsDumpfp = NULL;
        }
    }
}

void SVTHEVCEncoderBase::initDefaultParameters()
{
    // Encoding preset
    m_encParameters.encMode                         = 9;
    m_encParameters.tune                            = 1;
    //m_encParameters.latencyMode                     = 1;

    // GOP Structure
    m_encParameters.intraPeriodLength               = 255;
    m_encParameters.intraRefreshType                = 0;
    m_encParameters.hierarchicalLevels              = 0;

    m_encParameters.predStructure                   = 0;
    m_encParameters.baseLayerSwitchMode             = 0;

    // Input Info
    m_encParameters.sourceWidth                     = 0;
    m_encParameters.sourceHeight                    = 0;
    m_encParameters.frameRate                       = 0;
    m_encParameters.frameRateNumerator              = 0;
    m_encParameters.frameRateDenominator            = 0;
    m_encParameters.encoderBitDepth                 = 8;
    m_encParameters.encoderColorFormat              = EB_YUV420;
    m_encParameters.compressedTenBitFormat          = 0;
    m_encParameters.framesToBeEncoded               = 0;

    // Visual quality optimizations only applicable when tune = 1
    m_encParameters.bitRateReduction                = 1;
    m_encParameters.improveSharpness                = 1;

    // Interlaced Video
    m_encParameters.interlacedVideo                 = 0;

#if 0
    // Quantization
    m_encParameters.qp                              = 32;
    m_encParameters.useQpFile                       = 0;
#endif

    // Deblock Filter
    m_encParameters.disableDlfFlag                  = 0;

    // SAO
    m_encParameters.enableSaoFlag                   = 1;

    // Motion Estimation Tools
    m_encParameters.useDefaultMeHme                 = 1;
    m_encParameters.enableHmeFlag                   = 1;

#if 0
    // ME Parameters
    m_encParameters.searchAreaWidth                 = 16;
    m_encParameters.searchAreaHeight                = 7;
#endif

    // MD Parameters
    m_encParameters.constrainedIntra        = 0;

    // Rate Control
    m_encParameters.rateControlMode         = 1; //0 : CQP , 1 : VBR
    m_encParameters.sceneChangeDetection    = 1;
    m_encParameters.lookAheadDistance       = 0;
    m_encParameters.targetBitRate           = 0;
    m_encParameters.maxQpAllowed            = 48;
    m_encParameters.minQpAllowed            = 10;

    // bitstream options
    m_encParameters.codeVpsSpsPps           = 1;
    m_encParameters.codeEosNal              = 0;
    m_encParameters.videoUsabilityInfo      = 0;
    m_encParameters.highDynamicRangeInput   = 0;
    m_encParameters.accessUnitDelimiter     = 0;
    m_encParameters.bufferingPeriodSEI      = 0;
    m_encParameters.pictureTimingSEI        = 0;
    m_encParameters.registeredUserDataSeiFlag   = 0;
    m_encParameters.unregisteredUserDataSeiFlag = 0;
    m_encParameters.recoveryPointSeiFlag    = 0;
    m_encParameters.enableTemporalId        = 1;
    m_encParameters.profile                 = 2;
    m_encParameters.tier                    = 0;
    m_encParameters.level                   = 0;
    m_encParameters.fpsInVps                = 0;

    // Application Specific parameters
    m_encParameters.channelId               = 0;
    m_encParameters.activeChannelCount      = 1;

    // Threads management
    m_encParameters.logicalProcessors           = 0;
    m_encParameters.targetSocket                = -1;
    m_encParameters.switchThreadsToRtPriority   = 1;

    // ASM Type
    m_encParameters.asmType = 1;

    // Demo features
    m_encParameters.speedControlFlag        = 0;
    m_encParameters.injectorFrameRate       = m_encParameters.frameRate << 16;

    // Debug tools
    m_encParameters.reconEnabled            = false;
}

void SVTHEVCEncoderBase::updateParameters(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds)
{
    //resolution
    m_encParameters.sourceWidth         = width;
    m_encParameters.sourceHeight        = height;

    //gop
#if 0
    uint32_t intraPeriodLength          = keyFrameIntervalSeconds * frameRate;
    m_encParameters.intraPeriodLength   = (intraPeriodLength < 255) ? intraPeriodLength : 255;
#else
    //m_encParameters.intraPeriodLength   = frameRate >> 1;
    m_encParameters.intraPeriodLength   = frameRate;
#endif

    //framerate
    m_encParameters.frameRate           = frameRate;
    m_encParameters.injectorFrameRate   = frameRate << 16;

    //bitrate
    m_encParameters.targetBitRate       = bitrateKbps * 1000;
}

void SVTHEVCEncoderBase::setMCTSParameters(uint32_t tiles_col, uint32_t tiles_row)
{
    if (tiles_col * tiles_row > 1) {
        m_encParameters.unrestrictedMotionVector    = 0;
        m_encParameters.tileSliceMode               = 1;
        m_encParameters.tileColumnCount             = tiles_col;
        m_encParameters.tileRowCount                = tiles_row;

        m_encParameters.hierarchicalLevels          = 3;

        m_encParameters.disableDlfFlag              = 1;

        m_encParameters.bitRateReduction            = 0;
        m_encParameters.improveSharpness            = 0;

        ELOG_DEBUG_T("set MCTS, tiles_col=%d, tiles_row=%d",
                tiles_col, tiles_row);
    }
}

bool SVTHEVCEncoderBase::init(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, uint32_t tiles_col, uint32_t tiles_row)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    ELOG_DEBUG_T("initEncoder: width=%d, height=%d, frameRate=%d, bitrateKbps=%d, .keyFrameIntervalSeconds=%d}"
            , width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);

    return_error = EbInitHandle(&m_handle, this, &m_encParameters);
    if (return_error != EB_ErrorNone) {
        ELOG_ERROR_T("InitHandle failed, ret 0x%x", return_error);

        m_handle = NULL;
        return false;
    }

    initDefaultParameters();
    updateParameters(width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);
    setMCTSParameters(tiles_col, tiles_row);

    return_error = EbH265EncSetParameter(m_handle, &m_encParameters);
    if (return_error != EB_ErrorNone) {
        ELOG_ERROR_T("SetParameter failed, ret 0x%x", return_error);

        EbDeinitHandle(m_handle);
        m_handle = NULL;
        return false;
    }

    return_error = EbInitEncoder(m_handle);
    if (return_error != EB_ErrorNone) {
        ELOG_ERROR_T("InitEncoder failed, ret 0x%x", return_error);

        EbDeinitHandle(m_handle);
        m_handle = NULL;
        return false;
    }

    if (!allocateBuffers()) {
        ELOG_ERROR_T("allocateBuffers failed");

        deallocateBuffers();
        EbDeinitEncoder(m_handle);
        EbDeinitHandle(m_handle);
        m_handle = NULL;
        return false;
    }

    if (m_enableBsDump) {
        char dumpFileName[128];

        snprintf(dumpFileName, 128, "/tmp/SVTHEVCEncoderBase-%dx%d-%p.%s", width, height, this, "hevc");
        m_bsDumpfp = fopen(dumpFileName, "wb");
        if (m_bsDumpfp) {
            ELOG_INFO_T("Enable bitstream dump, %s", dumpFileName);
        } else {
            ELOG_INFO_T("Can not open dump file, %s", dumpFileName);
        }
    }

    return true;
}

bool SVTHEVCEncoderBase::sendFrame(const Frame& frame)
{
    switch (frame.format) {
        case FRAME_FORMAT_I420: {
            webrtc::VideoFrame *videoFrame = reinterpret_cast<webrtc::VideoFrame*>(frame.payload);
            rtc::scoped_refptr<webrtc::VideoFrameBuffer> videoBuffer = videoFrame->video_frame_buffer();
            return sendVideoBuffer(videoBuffer, videoFrame->timestamp_us(), videoFrame->timestamp_us());
            break;
        }

        default:
            ELOG_ERROR_T("Unspported video frame format %s(%d)", getFormatStr(frame.format), frame.format);
            return false;
    }

}

bool SVTHEVCEncoderBase::sendVideoBuffer(rtc::scoped_refptr<webrtc::VideoFrameBuffer> videoBuffer, int64_t dts, int64_t pts)
{
    int ret;

    if ((uint32_t)videoBuffer->width() == m_encParameters.sourceWidth
            && (uint32_t)videoBuffer->height() == m_encParameters.sourceHeight) {
        return sendPicture(videoBuffer->DataY(), videoBuffer->StrideY(),
                videoBuffer->DataU(), videoBuffer->StrideU(),
                videoBuffer->DataV(), videoBuffer->StrideV(),
                dts, pts);
    } else {
        if (!m_scaling_frame_buffer) {
            m_scaling_frame_buffer = (uint8_t *)malloc(m_encParameters.sourceWidth * m_encParameters.sourceHeight * 3 / 2);
        }

        ret = libyuv::I420Scale(
                videoBuffer->DataY(),   videoBuffer->StrideY(),
                videoBuffer->DataU(),   videoBuffer->StrideU(),
                videoBuffer->DataV(),   videoBuffer->StrideV(),
                videoBuffer->width(),   videoBuffer->height(),
                m_scaling_frame_buffer,
                m_encParameters.sourceWidth,
                m_scaling_frame_buffer + m_encParameters.sourceWidth * m_encParameters.sourceHeight,
                m_encParameters.sourceWidth / 2,
                m_scaling_frame_buffer + m_encParameters.sourceWidth * m_encParameters.sourceHeight * 5 / 4,
                m_encParameters.sourceWidth / 2,
                m_encParameters.sourceWidth, m_encParameters.sourceHeight,
                libyuv::kFilterBox);
        if (ret != 0) {
            ELOG_ERROR_T("Convert frame failed(%d), %dx%d -> %dx%d", ret
                    , videoBuffer->width()
                    , videoBuffer->height()
                    , m_encParameters.sourceWidth
                    , m_encParameters.sourceHeight
                    );
            return false;
        }

        return sendPicture(
                m_scaling_frame_buffer,
                m_encParameters.sourceWidth,
                m_scaling_frame_buffer + m_encParameters.sourceWidth * m_encParameters.sourceHeight,
                m_encParameters.sourceWidth / 2,
                m_scaling_frame_buffer + m_encParameters.sourceWidth * m_encParameters.sourceHeight * 5 / 4,
                m_encParameters.sourceWidth / 2,
                dts, pts);
    }
}

bool SVTHEVCEncoderBase::sendPicture(const uint8_t *y_plane, uint32_t y_stride,
        const uint8_t *u_plane, uint32_t u_stride,
        const uint8_t *v_plane, uint32_t v_stride,
        int64_t dts, int64_t pts)
{
    // frame buffer
    m_inputFrameBuffer.luma = const_cast<uint8_t *>(y_plane);
    m_inputFrameBuffer.cb = const_cast<uint8_t *>(u_plane);
    m_inputFrameBuffer.cr = const_cast<uint8_t *>(v_plane);

    m_inputFrameBuffer.yStride   = y_stride;
    m_inputFrameBuffer.crStride  = u_stride;
    m_inputFrameBuffer.cbStride  = v_stride;

    m_inputBuffer.pts = pts;
    m_inputBuffer.dts = dts;

    // frame type
    if (m_forceIDR) {
        m_inputBuffer.sliceType = EB_IDR_PICTURE;
        m_forceIDR = false;

        ELOG_DEBUG_T("SendPicture, IDR");
    } else {
        m_inputBuffer.sliceType = EB_INVALID_PICTURE;
    }

    int return_error = EbH265EncSendPicture(m_handle, &m_inputBuffer);
    if (return_error != EB_ErrorNone) {
        ELOG_ERROR_T("SendPicture failed, ret 0x%x", return_error);
        return false;
    }

    return true;
}

EB_BUFFERHEADERTYPE *SVTHEVCEncoderBase::getOutBuffer(void)
{
    EB_BUFFERHEADERTYPE *pOutBuffer = &m_outputBuffer;
    int return_error;

    return_error = EbH265GetPacket(m_handle, &pOutBuffer , false);
    if (return_error == EB_ErrorMax) {
        ELOG_ERROR_T("Error while encoding, code 0x%x", pOutBuffer->nFlags);
        return NULL;
    } else if (return_error != EB_NoErrorEmptyQueue) {
        ELOG_TRACE_T("nFilledLen(%d), nTickCount %d(ms), dts(%ld), pts(%ld), nFlags(0x%x), qpValue(%d), sliceType(%d)"
                , pOutBuffer->nFilledLen
                , pOutBuffer->nTickCount
                , pOutBuffer->dts
                , pOutBuffer->pts
                , pOutBuffer->nFlags
                , pOutBuffer->qpValue
                , pOutBuffer->sliceType
                );

        dump(pOutBuffer->pBuffer, pOutBuffer->nFilledLen);

        return pOutBuffer;
    } else {
        return NULL;
    }
}

boost::shared_ptr<SVTHEVCEncodedPacket> SVTHEVCEncoderBase::getEncodedPacket(void)
{
    EB_BUFFERHEADERTYPE *pOutBuffer = getOutBuffer();
    if (!pOutBuffer)
        return NULL;

    boost::shared_ptr<SVTHEVCEncodedPacket> encoded_pkt = boost::make_shared<SVTHEVCEncodedPacket>(pOutBuffer);
    EbH265ReleaseOutBuffer(&pOutBuffer);
    return encoded_pkt;
}

void SVTHEVCEncoderBase::sendVideoBuffer_async(rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_buffer, int64_t dts, int64_t pts)
{
    m_srv->post(boost::bind(&SVTHEVCEncoderBase::SendVideoBuffer_async, this, video_buffer, dts, pts));
}

void SVTHEVCEncoderBase::SendVideoBuffer_async(SVTHEVCEncoderBase *This, rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_buffer, int64_t dts, int64_t pts)
{
    int ret;

    ret = This->sendVideoBuffer(video_buffer, dts, pts);
    if (!ret)
        return;

    boost::shared_ptr<SVTHEVCEncodedPacket> encoder_pkt;
    encoder_pkt = This->getEncodedPacket();
    if (This->m_listener)
        This->m_listener->onEncodedPacket(This, encoder_pkt);
}

void SVTHEVCEncoderBase::requestKeyFrame()
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG_T("%s", __FUNCTION__);

    m_forceIDR = true;
}

bool SVTHEVCEncoderBase::allocateBuffers()
{
    memset(&m_inputFrameBuffer, 0, sizeof(m_inputFrameBuffer));
    memset(&m_inputBuffer, 0, sizeof(m_inputBuffer));
    memset(&m_outputBuffer, 0, sizeof(m_outputBuffer));

    // output buffer
    m_inputBuffer.nSize        = sizeof(EB_BUFFERHEADERTYPE);
    m_inputBuffer.nAllocLen    = 0;
    m_inputBuffer.pAppPrivate  = NULL;
    m_inputBuffer.sliceType    = EB_INVALID_PICTURE;
    m_inputBuffer.pBuffer      = (unsigned char *)&m_inputFrameBuffer;

    // output buffer
    size_t outputStreamBufferSize = m_encParameters.sourceWidth * m_encParameters.sourceHeight * 2;
    m_outputBuffer.nSize = sizeof(EB_BUFFERHEADERTYPE);
    m_outputBuffer.nAllocLen   = outputStreamBufferSize;
    m_outputBuffer.pAppPrivate = this;
    m_outputBuffer.sliceType   = EB_INVALID_PICTURE;
    m_outputBuffer.pBuffer = (unsigned char *)malloc(outputStreamBufferSize);
    if (!m_outputBuffer.pBuffer) {
        ELOG_ERROR_T("Can not alloc mem, size(%ld)", outputStreamBufferSize);
        return false;
    }

    return true;
}

void SVTHEVCEncoderBase::deallocateBuffers()
{
    if (!m_outputBuffer.pBuffer) {
        free(m_outputBuffer.pBuffer);
        m_outputBuffer.pBuffer = NULL;
    }
}

void SVTHEVCEncoderBase::setDebugDump(bool enable)
{
    m_enableBsDump = enable;
}

void SVTHEVCEncoderBase::dump(uint8_t *buf, int len)
{
    if (m_bsDumpfp) {
        fwrite(buf, 1, len, m_bsDumpfp);
    }
}

} // namespace owt_base
