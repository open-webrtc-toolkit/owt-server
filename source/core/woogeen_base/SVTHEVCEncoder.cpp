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

#include "SVTHEVCEncoder.h"

#include <webrtc/api/video/video_frame.h>
#include <webrtc/api/video/video_frame_buffer.h>

#include <libyuv/convert.h>
#include <libyuv/planar_functions.h>
#include <libyuv/scale.h>

#include "MediaUtilities.h"

namespace woogeen_base {

DEFINE_LOGGER(SVTHEVCEncoder, "woogeen.SVTHEVCEncoder");

SVTHEVCEncoder::SVTHEVCEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast)
    : m_encoderReady(false)
    , m_dest(NULL)
    , m_width(0)
    , m_height(0)
    , m_frameRate(0)
    , m_bitrateKbps(0)
    , m_keyFrameIntervalSeconds(0)
    , m_handle(NULL)
    , m_forceIDR(false)
    , m_frameCount(0)
    , m_enableBsDump(false)
    , m_bsDumpfp(NULL)
{
    memset(&m_encParameters, 0, sizeof(m_encParameters));
}

SVTHEVCEncoder::~SVTHEVCEncoder()
{
    if (m_encoderReady) {
        EbDeinitEncoder(m_handle);
        EbDeinitHandle(m_handle);

        deallocateBuffers();

        if (m_bsDumpfp) {
            fclose(m_bsDumpfp);
            m_bsDumpfp = NULL;
        }

        m_encoderReady = false;
    }

    m_dest = NULL;
}

void SVTHEVCEncoder::initDefaultParameters()
{
    // Encoding preset
    m_encParameters.encMode                         = 9;
    m_encParameters.tune                            = 1;
    m_encParameters.latencyMode                     = 0;

    // GOP Structure
    m_encParameters.intraPeriodLength               = 255;
    m_encParameters.intraRefreshType                = 2;
    m_encParameters.hierarchicalLevels              = 3;

    m_encParameters.predStructure                   = 0;
    m_encParameters.baseLayerSwitchMode             = 0;

    // Input Info
    m_encParameters.sourceWidth                     = 0;
    m_encParameters.sourceHeight                    = 0;
    m_encParameters.frameRate                       = 0;
    m_encParameters.frameRateNumerator              = 0;
    m_encParameters.frameRateDenominator            = 0;
    m_encParameters.encoderBitDepth                 = 8;
    m_encParameters.compressedTenBitFormat          = 0;
    m_encParameters.framesToBeEncoded               = 0;

    // Visual quality optimizations only applicable when tune = 1
    m_encParameters.bitRateReduction                = 1;
    m_encParameters.improveSharpness                = 1;

    // Interlaced Video
    m_encParameters.interlacedVideo                 = 0;

#if 0
    // Quantization
    m_encParameters.qp                              = 25;
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
    m_encParameters.profile                 = 1;
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
    m_encParameters.speedControlFlag        = 1;
    m_encParameters.injectorFrameRate       = m_encParameters.frameRate << 16;

    // Debug tools
    m_encParameters.reconEnabled            = false;

#if 0
    // SEI
    m_encParameters.maxCLL;
    m_encParameters.maxFALL;
    m_encParameters.useMasteringDisplayColorVolume;
    m_encParameters.useNaluFile;
    m_encParameters.dolbyVisionProfile;

    // Master Display Color Volume Parameters
    m_encParameters.displayPrimaryX[3];
    m_encParameters.displayPrimaryY[3];
    m_encParameters.whitePointX, whitePointY;
    m_encParameters.maxDisplayMasteringLuminance;
    m_encParameters.minDisplayMasteringLuminance;
#endif
}

void SVTHEVCEncoder::updateParameters(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds)
{
    //resolution
    m_encParameters.sourceWidth         = width;
    m_encParameters.sourceHeight        = height;

    //gop
    uint32_t intraPeriodLength          = keyFrameIntervalSeconds * frameRate;
    m_encParameters.intraPeriodLength   = (intraPeriodLength < 255) ? intraPeriodLength : 255;

    //framerate
    m_encParameters.frameRate           = frameRate;
    m_encParameters.injectorFrameRate   = frameRate << 16;

    //bitrate
    m_encParameters.targetBitRate       = bitrateKbps * 1000;
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
    EB_ERRORTYPE return_error = EB_ErrorNone;

    ELOG_DEBUG_T("initEncoder: width=%d, height=%d, frameRate=%d, bitrateKbps=%d, .keyFrameIntervalSeconds=%d}"
            , width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);

    return_error = EbInitHandle(&m_handle, this, &m_encParameters);
    if (return_error != EB_ErrorNone) {
        ELOG_ERROR_T("InitHandle failed, ret 0x%x", return_error);
        return false;
    }

    initDefaultParameters();
    updateParameters(width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);

    return_error = EbH265EncSetParameter(m_handle, &m_encParameters);
    if (return_error != EB_ErrorNone) {
        ELOG_ERROR_T("SetParameter failed, ret 0x%x", return_error);

        EbDeinitHandle(m_handle);
        return false;
    }

    return_error = EbInitEncoder(m_handle);
    if (return_error != EB_ErrorNone) {
        ELOG_ERROR_T("InitEncoder failed, ret 0x%x", return_error);

        EbDeinitHandle(m_handle);
        return false;
    }

    if (!allocateBuffers()) {
        ELOG_ERROR_T("allocateBuffers failed");

        deallocateBuffers();
        EbDeinitEncoder(m_handle);
        EbDeinitHandle(m_handle);
        return false;
    }

    if (m_enableBsDump) {
        char dumpFileName[128];

        snprintf(dumpFileName, 128, "/tmp/svtHEVCEncoder-%p.%s", this, "hevc");
        m_bsDumpfp = fopen(dumpFileName, "wb");
        if (m_bsDumpfp) {
            ELOG_DEBUG("Enable bitstream dump, %s", dumpFileName);
        } else {
            ELOG_DEBUG("Can not open dump file, %s", dumpFileName);
        }
    }

    m_encoderReady = true;
    return true;
}

int32_t SVTHEVCEncoder::generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, woogeen_base::FrameDestination* dest)
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
        if (!initEncoder(m_width, m_height, m_frameRate, m_bitrateKbps, m_keyFrameIntervalSeconds))
            return -1;
    }

    m_frameCount = 0;
    m_dest = dest;

    return 0;
}

void SVTHEVCEncoder::degenerateStream(int32_t streamId)
{
    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

    ELOG_DEBUG_T("degenerateStream");

    if (m_encoderReady) {
        EbDeinitEncoder(m_handle);
        EbDeinitHandle(m_handle);

        deallocateBuffers();

        if (m_bsDumpfp) {
            fclose(m_bsDumpfp);
            m_bsDumpfp = NULL;
        }

        m_encoderReady = false;
    }

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

    m_forceIDR = true;
}

void SVTHEVCEncoder::onFrame(const Frame& frame)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    EB_ERRORTYPE return_error = EB_ErrorNone;

    if (m_dest == NULL) {
        return;
    }

    if (m_width == 0 || m_height == 0) {
        m_width = frame.additionalInfo.video.width;
        m_height = frame.additionalInfo.video.height;

        if (m_bitrateKbps == 0)
            m_bitrateKbps = calcBitrate(m_width, m_height, m_frameRate);

        if (!initEncoder(m_width, m_height, m_frameRate, m_bitrateKbps, m_keyFrameIntervalSeconds)) {
            return;
        }
    }

    if (!m_encoderReady) {
        ELOG_ERROR_T("Encoder not ready!");
        return;
    }

    if (m_freeInputBuffers.empty()) {
        ELOG_WARN_T("No free input buffer available!");
        return;
    }
    EB_BUFFERHEADERTYPE *inputBufferHeader = m_freeInputBuffers.front();
    if (!convert2BufferHeader(frame, inputBufferHeader)) {
        return;
    }

    if (m_forceIDR) {
        inputBufferHeader->sliceType = EB_IDR_PICTURE;
        m_forceIDR = false;
    } else {
        inputBufferHeader->sliceType = EB_INVALID_PICTURE;
    }

    ELOG_TRACE_T("SendPicture, sliceType(%d)", inputBufferHeader->sliceType);
    return_error = EbH265EncSendPicture(m_handle, inputBufferHeader);
    if (return_error != EB_ErrorNone) {
        ELOG_ERROR_T("SendPicture failed, ret 0x%x", return_error);
        return;
    }

    EB_BUFFERHEADERTYPE *streamBufferHeader = &m_streamBufferPool[0];

    return_error = EbH265GetPacket(m_handle, &streamBufferHeader, false);
    if (return_error == EB_ErrorMax) {
        ELOG_ERROR_T("Error while encoding, code 0x%x", streamBufferHeader->nFlags);
        return;
    }else if (return_error != EB_NoErrorEmptyQueue) {
        fillPacketDone(streamBufferHeader);
    }
}

bool SVTHEVCEncoder::convert2BufferHeader(const Frame& frame, EB_BUFFERHEADERTYPE *bufferHeader)
{
    EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)bufferHeader->pBuffer;

    switch (frame.format) {
        case FRAME_FORMAT_I420: {
            int ret;
            webrtc::VideoFrame *videoFrame = reinterpret_cast<webrtc::VideoFrame*>(frame.payload);
            rtc::scoped_refptr<webrtc::VideoFrameBuffer> videoBuffer = videoFrame->video_frame_buffer();

            if ((uint32_t)videoBuffer->width() == m_encParameters.sourceWidth
                    && (uint32_t)videoBuffer->height() == m_encParameters.sourceHeight) {
                ret = libyuv::I420Copy(
                        videoBuffer->DataY(), videoBuffer->StrideY(),
                        videoBuffer->DataU(), videoBuffer->StrideU(),
                        videoBuffer->DataV(), videoBuffer->StrideV(),
                        inputPtr->luma, inputPtr->yStride,
                        inputPtr->cb,   inputPtr->cbStride,
                        inputPtr->cr,   inputPtr->crStride,
                        m_encParameters.sourceWidth,
                        m_encParameters.sourceHeight);
                if (ret != 0) {
                    ELOG_ERROR_T("Copy frame failed(%d), %dx%d", ret
                            , m_encParameters.sourceWidth
                            , m_encParameters.sourceHeight
                            );
                    return false;
                }
            } else {
                ret = libyuv::I420Scale(
                        videoBuffer->DataY(),   videoBuffer->StrideY(),
                        videoBuffer->DataU(),   videoBuffer->StrideU(),
                        videoBuffer->DataV(),   videoBuffer->StrideV(),
                        videoBuffer->width(),   videoBuffer->height(),
                        inputPtr->luma, inputPtr->yStride,
                        inputPtr->cb,   inputPtr->cbStride,
                        inputPtr->cr,   inputPtr->crStride,
                        m_encParameters.sourceWidth,
                        m_encParameters.sourceHeight,
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
            }
            break;
        }

        default:
            ELOG_ERROR_T("Unspported video frame format %s(%d)", getFormatStr(frame.format), frame.format);
            return false;
    }

    return true;
}

bool SVTHEVCEncoder::allocateBuffers()
{
    // one buffer
    uint32_t inputOutputBufferFifoInitCount = 1;

    // input buffers
    const size_t luma8bitSize = m_encParameters.sourceWidth * m_encParameters.sourceHeight;
    const size_t chroma8bitSize = luma8bitSize >> 2;

    m_inputBufferPool.resize(inputOutputBufferFifoInitCount);
    memset(m_inputBufferPool.data(), 0, m_inputBufferPool.size() * sizeof(EB_BUFFERHEADERTYPE));
    for (unsigned int bufferIndex = 0; bufferIndex < inputOutputBufferFifoInitCount; ++bufferIndex) {
        m_inputBufferPool[bufferIndex].nSize        = sizeof(EB_BUFFERHEADERTYPE);

        m_inputBufferPool[bufferIndex].pBuffer      = (unsigned char *)calloc(1, sizeof(EB_H265_ENC_INPUT));
        if (!m_inputBufferPool[bufferIndex].pBuffer) {
            ELOG_ERROR_T("Can not alloc mem, size(%ld)", sizeof(EB_H265_ENC_INPUT));
            return false;
        }

        // alloc frame
        EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)m_inputBufferPool[bufferIndex].pBuffer;
        inputPtr->luma = (unsigned char *)malloc(luma8bitSize);
        if (!inputPtr->luma) {
            ELOG_ERROR_T("Can not alloc mem, size(%ld)", luma8bitSize);
            return false;
        }

        inputPtr->cb = (unsigned char *)malloc(chroma8bitSize);
        if (!inputPtr->cb) {
            ELOG_ERROR_T("Can not alloc mem, size(%ld)", chroma8bitSize);
            return false;
        }

        inputPtr->cr = (unsigned char *)malloc(chroma8bitSize);
        if (!inputPtr->cr) {
            ELOG_ERROR_T("Can not alloc mem, size(%ld)", chroma8bitSize);
            return false;
        }

        inputPtr->yStride   = m_encParameters.sourceWidth;
        inputPtr->crStride  = m_encParameters.sourceWidth >> 1;
        inputPtr->cbStride  = m_encParameters.sourceWidth >> 1;


        m_inputBufferPool[bufferIndex].nAllocLen    = luma8bitSize + chroma8bitSize + chroma8bitSize;
        m_inputBufferPool[bufferIndex].pAppPrivate  = this;
        m_inputBufferPool[bufferIndex].sliceType    = EB_INVALID_PICTURE;

        m_freeInputBuffers.push(&m_inputBufferPool[bufferIndex]);
    }

    // output buffers
    size_t outputStreamBufferSize = m_encParameters.sourceWidth * m_encParameters.sourceHeight * 3 / 2;

    m_streamBufferPool.resize(inputOutputBufferFifoInitCount);
    memset(m_streamBufferPool.data(), 0, m_streamBufferPool.size() * sizeof(EB_BUFFERHEADERTYPE));
    for (uint32_t bufferIndex = 0; bufferIndex < inputOutputBufferFifoInitCount; ++bufferIndex) {
        m_streamBufferPool[bufferIndex].nSize = sizeof(EB_BUFFERHEADERTYPE);
        m_streamBufferPool[bufferIndex].pBuffer = (unsigned char *)malloc(outputStreamBufferSize);
        if (!m_streamBufferPool[bufferIndex].pBuffer) {
            ELOG_ERROR_T("Can not alloc mem, size(%ld)", outputStreamBufferSize);
            return false;
        }

        m_streamBufferPool[bufferIndex].nAllocLen   = outputStreamBufferSize;
        m_streamBufferPool[bufferIndex].pAppPrivate = this;
        m_streamBufferPool[bufferIndex].sliceType   = EB_INVALID_PICTURE;
    }

    return true;
}

void SVTHEVCEncoder::deallocateBuffers()
{
    for (auto& bufferHeader : m_inputBufferPool) {
        if (bufferHeader.pBuffer) {
            EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)bufferHeader.pBuffer;

            if (inputPtr->luma) {
                free(inputPtr->luma);
                inputPtr->luma = NULL;
            }

            if (inputPtr->cb) {
                free(inputPtr->cb);
                inputPtr->cb = NULL;
            }

            if (inputPtr->cr) {
                free(inputPtr->cr);
                inputPtr->cr = NULL;
            }

            free(bufferHeader.pBuffer);
            bufferHeader.pBuffer = NULL;
        }
    }
    m_inputBufferPool.resize(0);

    for (auto& bufferHeader : m_streamBufferPool) {
        if (bufferHeader.pBuffer) {
            free(bufferHeader.pBuffer);
            bufferHeader.pBuffer = NULL;
        }
    }
    m_streamBufferPool.resize(0);
}

void SVTHEVCEncoder::fillPacketDone(EB_BUFFERHEADERTYPE* pBufferHeader)
{
    ELOG_TRACE_T("Fill packet done, nFilledLen(%d), nTickCount %d(ms), dts(%ld), pts(%ld), nFlags(0x%x), qpValue(%d), sliceType(%d)"
            , pBufferHeader->nFilledLen
            , pBufferHeader->nTickCount
            , pBufferHeader->dts
            , pBufferHeader->pts
            , pBufferHeader->nFlags
            , pBufferHeader->qpValue
            , pBufferHeader->sliceType
            );

    dump(pBufferHeader->pBuffer, pBufferHeader->nFilledLen);

    Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));
    outFrame.format     = FRAME_FORMAT_H265;
    outFrame.payload    = pBufferHeader->pBuffer;
    outFrame.length     = pBufferHeader->nFilledLen;
    outFrame.timeStamp = (m_frameCount++) * 1000 / m_encParameters.frameRate * 90;
    outFrame.additionalInfo.video.width         = m_encParameters.sourceWidth;
    outFrame.additionalInfo.video.height        = m_encParameters.sourceHeight;
    outFrame.additionalInfo.video.isKeyFrame    = (pBufferHeader->sliceType == EB_IDR_PICTURE);

    ELOG_TRACE_T("deliverFrame, %s, %dx%d(%s), length(%d)",
            getFormatStr(outFrame.format),
            outFrame.additionalInfo.video.width,
            outFrame.additionalInfo.video.height,
            outFrame.additionalInfo.video.isKeyFrame ? "key" : "delta",
            outFrame.length);

    m_dest->onFrame(outFrame);
}

void SVTHEVCEncoder::dump(uint8_t *buf, int len)
{
    if (m_bsDumpfp) {
        fwrite(buf, 1, len, m_bsDumpfp);
    }
}

} // namespace woogeen_base
