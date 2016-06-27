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

#ifdef ENABLE_MSDK

#include <malloc.h>

#include "FastCopy.h"

#include "MsdkBase.h"
#include "MsdkFrame.h"

namespace woogeen_base {

DEFINE_LOGGER(MsdkFrame, "woogeen.MsdkFrame");

MsdkFrame::MsdkFrame(boost::shared_ptr<mfxFrameAllocator> allocator, mfxFrameInfo &info, mfxMemId id)
    : m_allocator(allocator)
    , m_mainSession(NULL)
    , m_needSync(false)
    , m_nv12TBuffer(NULL)
    , m_nv12TBufferSize(0)
{
    memset(&m_surface, 0, sizeof(mfxFrameSurface1));

    m_surface.Info = info;
    m_surface.Data.MemId = id;
}

MsdkFrame::~MsdkFrame()
{
    printfFuncEnter;

    if (m_nv12TBuffer) {
        free(m_nv12TBuffer);
        m_nv12TBuffer = NULL;
    }

    printfFuncExit;
}

void MsdkFrame::sync(void)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!m_needSync)
        return;

    m_needSync = false;

    if (!m_mainSession && !(m_mainSession = MsdkBase::get()->getMainSession())) {
        ELOG_ERROR("Invalid main session!");

        return;
    }

    sts = m_mainSession->SyncOperation(m_syncP, MFX_INFINITE);
    if(sts != MFX_ERR_NONE)
    {
        ELOG_ERROR("SyncOperation failed, ret %d", sts);
        return;
    }
}

bool MsdkFrame::reSize(uint32_t width, uint32_t height)
{
    ELOG_TRACE("reSize %dx%d(%dx%d) -> %dx%d"
            , m_surface.Info.CropW, m_surface.Info.CropH
            , m_surface.Info.Width, m_surface.Info.Height
            , width, height);

    if (width > m_surface.Info.Width || height > m_surface.Info.Height) {
        ELOG_ERROR("Can not crop %dx%d into %dx%d"
                , m_surface.Info.Width, m_surface.Info.Height
                , width, height);

        return false;
    }

    m_surface.Info.CropW = width;
    m_surface.Info.CropH = height;

    if (m_nv12TBuffer && m_nv12TBufferSize < width * height * 3 / 2) {
        free(m_nv12TBuffer);
        m_nv12TBuffer = NULL;
        m_nv12TBufferSize = 0;
    }

    return true;
}

bool MsdkFrame::fillFrame(uint8_t y, uint8_t u, uint8_t v)
{
    bool ret = false;

    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1 *pSurface = &m_surface;

    mfxFrameInfo& pInfo = pSurface->Info;
    mfxFrameData& pData = pSurface->Data;

    // supports only NV12 mfx surfaces for code transparency,
    // other formats may be added if application requires such functionality
    if (MFX_FOURCC_NV12 != pInfo.FourCC) {
        ELOG_ERROR("Format (%c%c%c%c) is not upported!",
                pInfo.FourCC & 0xff,
                (pInfo.FourCC >> 8) & 0xff,
                (pInfo.FourCC >> 16) & 0xff,
                (pInfo.FourCC >> 24) & 0xff
                );

        return ret;
    }

    sync();

    sts = m_allocator->Lock(m_allocator.get()->pthis, pData.MemId, &pData);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Failed to lock surface!");

        return ret;
    }

    mfxU16 w, h, i, pitch;
    mfxU8 *ptr;

    if (pInfo.CropH > 0 && pInfo.CropW > 0) {
        w = pInfo.CropW;
        h = pInfo.CropH;
    }
    else {
        w = pInfo.Width;
        h = pInfo.Height;
    }

    pitch = pData.Pitch;
    ptr = pData.Y + pInfo.CropX + pInfo.CropY * pData.Pitch;

    // set luminance plane
    for(i = 0; i < h; i++) {
        memset(ptr + i * pitch, y, w);
    }

    // set chroma planes
    switch (pSurface->Info.FourCC)
    {
        case MFX_FOURCC_NV12:
            mfxU32 j;
            w /= 2;
            h /= 2;
            ptr = pData.UV + pInfo.CropX + (pInfo.CropY / 2) * pitch;

            for (i = 0; i < h; i++) {
                for (j = 0; j < w; j++)
                {
                    ptr[i * pitch + j * 2]      = u;
                    ptr[i * pitch + j * 2 + 1]  = v;
                }
            }

            ret = true;
            break;

        default:
            ELOG_ERROR("Format (%c%c%c%c) is not upported!",
                    pInfo.FourCC & 0xff,
                    (pInfo.FourCC >> 8) & 0xff,
                    (pInfo.FourCC >> 16) & 0xff,
                    (pInfo.FourCC >> 24) & 0xff
                    );
            break;
    }

    sts = m_allocator->Unlock(m_allocator.get()->pthis, pData.MemId, &pData);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Failed to unlock surface!");

        return false;
    }

    return ret;
}

bool MsdkFrame::convertFrom(webrtc::I420VideoFrame& frame)
{
    bool ret = false;
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1 *pSurface = &m_surface;
    mfxFrameInfo& pInfo = pSurface->Info;
    mfxFrameData& pData = pSurface->Data;

    mfxU16 w, h, pitch;
    mfxU8 *ptr;
    mfxU32 i, j;

    uint32_t srcW, srcH;
    uint32_t srcStrideY, srcStrideU, srcStrideV;
    uint8_t *pSrcY, *pSrcU, *pSrcV;

    //dumpI420VideoFrameInfo(frame);

    srcW = frame.width();
    srcH = frame.height();

    srcStrideY = frame.stride(webrtc::kYPlane);
    srcStrideU = frame.stride(webrtc::kUPlane);
    srcStrideV = frame.stride(webrtc::kVPlane);

    pSrcY = frame.buffer(webrtc::kYPlane);
    pSrcU = frame.buffer(webrtc::kUPlane);
    pSrcV = frame.buffer(webrtc::kVPlane);

    // supports only NV12 mfx surfaces for code transparency,
    // other formats may be added if application requires such functionality
    if (MFX_FOURCC_NV12 != pInfo.FourCC) {
        ELOG_ERROR("Format (%c%c%c%c) is not upported!",
                pInfo.FourCC & 0xff,
                (pInfo.FourCC >> 8) & 0xff,
                (pInfo.FourCC >> 16) & 0xff,
                (pInfo.FourCC >> 24) & 0xff
                );

        return false;
    }

    if (pInfo.CropH > 0 && pInfo.CropW > 0) {
        w = pInfo.CropW;
        h = pInfo.CropH;
    }
    else {
        w = pInfo.Width;
        h = pInfo.Height;
    }

    if (w != srcW || h != srcH) {
        ELOG_WARN("Not support scale/crop src I420VideoFrame(%dx%d) into dst msdk surface(%dx%d)!",
                srcW, srcH, w, h);
        return false;
    }

    sync();

    sts = m_allocator->Lock(m_allocator.get()->pthis, pData.MemId, &pData);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Failed to lock surface!");

        return false;
    }

    pitch = pData.Pitch;
    ptr = pData.Y + pInfo.CropX + pInfo.CropY * pData.Pitch;

    // set luminance plane
    for(i = 0; i < h; i++) {
        memcpy(ptr + i * pitch, pSrcY + i * srcStrideY, w);
    }

    // set chroma planes
    switch (pSurface->Info.FourCC)
    {
        case MFX_FOURCC_NV12:
            w /= 2;
            h /= 2;
            ptr = pData.UV + pInfo.CropX + (pInfo.CropY / 2) * pitch;

            for (i = 0; i < h; i++) {
                for (j = 0; j < w; j++) {
                    ptr[i * pitch + j * 2]      = pSrcU[i * srcStrideU + j];
                    ptr[i * pitch + j * 2 + 1]  = pSrcV[i * srcStrideV + j];
                }
            }

            ret = true;
            break;

        default:
            ELOG_ERROR("Format (%c%c%c%c) is not upported!",
                    pInfo.FourCC & 0xff,
                    (pInfo.FourCC >> 8) & 0xff,
                    (pInfo.FourCC >> 16) & 0xff,
                    (pInfo.FourCC >> 24) & 0xff
                    );

            ret = false;
            break;
    }

    sts = m_allocator->Unlock(m_allocator.get()->pthis, pData.MemId, &pData);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Failed to unlock surface!");

        return false;
    }

    return ret;
}

bool MsdkFrame::nv12ConvertTo(mfxFrameInfo& pInfo, mfxFrameData& pData, webrtc::I420VideoFrame& frame)
{
    uint32_t w = getWidth();
    uint32_t h = getHeight();

    if (!m_nv12TBuffer) {
        m_nv12TBuffer = (uint8_t *)memalign(16, h * pData.Pitch * 3 / 2);
        if (!m_nv12TBuffer)
        {
            ELOG_ERROR("memalign failed, %p", m_nv12TBuffer);

            return false;
        }
        m_nv12TBufferSize = h * pData.Pitch * 3 / 2;
    }

    //uint32_t dstW, dstH;
    uint32_t dstStrideY, dstStrideU, dstStrideV;
    uint8_t *pDstY, *pDstU, *pDstV;

    dstStrideY = frame.stride(webrtc::kYPlane);
    dstStrideU = frame.stride(webrtc::kUPlane);
    dstStrideV = frame.stride(webrtc::kVPlane);

    pDstY = frame.buffer(webrtc::kYPlane);
    pDstU = frame.buffer(webrtc::kUPlane);
    pDstV = frame.buffer(webrtc::kVPlane);

    memcpy_from_uswc_sse4(m_nv12TBuffer, pData.Y, h * pData.Pitch);
    memcpy_from_uswc_sse4(m_nv12TBuffer + h * pData.Pitch, pData.UV, h * pData.Pitch / 2);

    uint8_t *ptrY;
    uint8_t *ptrUV;

    ptrY = m_nv12TBuffer;
    ptrUV = m_nv12TBuffer + h * pData.Pitch;

    uint32_t i, j;

    // set luminance plane
    for(i = 0; i < h; i++) {
        memcpy(pDstY + i * dstStrideY, ptrY + i * pData.Pitch, w);
    }

    w /= 2;
    h /= 2;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            pDstU[i * dstStrideU + j] = ptrUV[i * pData.Pitch + j * 2];
            pDstV[i * dstStrideV + j] = ptrUV[i * pData.Pitch + j * 2 + 1] ;
        }
    }

    return true;
}

bool MsdkFrame::convertTo(webrtc::I420VideoFrame& frame)
{
    bool ret = false;

    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1 *pSurface = &m_surface;
    mfxFrameInfo& pInfo = pSurface->Info;
    mfxFrameData& pData = pSurface->Data;

    if (MFX_FOURCC_NV12 != pInfo.FourCC) {
        ELOG_ERROR("Format (%c%c%c%c) is not upported!",
                pInfo.FourCC & 0xff,
                (pInfo.FourCC >> 8) & 0xff,
                (pInfo.FourCC >> 16) & 0xff,
                (pInfo.FourCC >> 24) & 0xff
                );

        return false;
    }

    frame.CreateEmptyFrame(
            getWidth(),
            getHeight(),
            getWidth(),
            getWidth() / 2,
            getWidth() / 2
            );

    //dumpI420VideoFrameInfo(frame);

    sync();

    sts = m_allocator->Lock(m_allocator.get()->pthis, pData.MemId, &pData);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Failed to lock surface!");

        return false;
    }

    switch (pSurface->Info.FourCC)
    {
        case MFX_FOURCC_NV12:
            ret = nv12ConvertTo(pInfo, pData, frame);
            break;

        default:
            ELOG_ERROR("Format (%c%c%c%c) is not upported!",
                    pInfo.FourCC & 0xff,
                    (pInfo.FourCC >> 8) & 0xff,
                    (pInfo.FourCC >> 16) & 0xff,
                    (pInfo.FourCC >> 24) & 0xff
                    );

            ret = false;
            break;
    }

    sts = m_allocator->Unlock(m_allocator.get()->pthis, pData.MemId, &pData);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Failed to unlock surface!");

        return false;
    }

    return ret;
}

void MsdkFrame::dumpI420VideoFrameInfo(webrtc::I420VideoFrame& frame)
{
    uint32_t srcW, srcH;
    uint32_t srcStrideY, srcStrideU, srcStrideV;
    //uint8_t *pSrcY, *pSrcU, *pSrcV;

    srcW = frame.width();
    srcH = frame.height();

    srcStrideY = frame.stride(webrtc::kYPlane);
    srcStrideU = frame.stride(webrtc::kUPlane);
    srcStrideV = frame.stride(webrtc::kVPlane);

    //pSrcY = frame.buffer(webrtc::kYPlane);
    //pSrcU = frame.buffer(webrtc::kUPlane);
    //pSrcV = frame.buffer(webrtc::kVPlane);

    ELOG_DEBUG("I420VideoFrame: %dx%d, stride %d-%d-%d, timestamp %d"
            , srcW, srcH
            , srcStrideY, srcStrideU, srcStrideV
            , frame.timestamp()
            );
}


MsdkFramePool::MsdkFramePool(boost::shared_ptr<mfxFrameAllocator> allocator, mfxFrameAllocRequest &request)
    : m_allocator(allocator)
    , m_request(request)
    , m_allocatedWidth(0)
    , m_allocatedHeight(0)
{
}

DEFINE_LOGGER(MsdkFramePool, "woogeen.MsdkFramePool");

MsdkFramePool::~MsdkFramePool()
{
    printfFuncEnter;

    freeFrames();

    printfFuncExit;
}

bool MsdkFramePool::init()
{
    if (!allocateFrames())
    {
        return false;
    }

    return true;
}

bool MsdkFramePool::allocateFrames()
{
    mfxStatus sts = MFX_ERR_NONE;

    printfFrameAllocRequest(&m_request);

    sts = m_allocator->Alloc(m_allocator->pthis, &m_request, &m_response);
    if (sts != MFX_ERR_NONE)
    {
        ELOG_ERROR("mfxFrameAllocator Alloc() failed, ret %d", sts);
        return false;
    }

    for(int i = 0;i < m_response.NumFrameActual;i++)
    {
        m_framePool.push_back(boost::shared_ptr<MsdkFrame>(new MsdkFrame(m_allocator, m_request.Info, m_response.mids[i])));
    }

    m_allocatedWidth     = m_request.Info.Width;
    m_allocatedHeight    = m_request.Info.Height;

    ELOG_DEBUG("allocate %d frames(%dx%d)", m_response.NumFrameActual, m_allocatedWidth, m_allocatedHeight);
    return true;
}

bool MsdkFramePool::waitForFrameFree(boost::shared_ptr<MsdkFrame>& frame, int timeout)
{
    int i = 0;

    while (!(frame.use_count() == 1 && frame->isFree()) && i < timeout)
    {
        dumpMsdkFrameInfo("waitFrame", frame);

        usleep(1000); //1ms
        i++;

        ELOG_DEBUG("Wait(%d ms)...", i);
    }

    if (!(frame.use_count() == 1 && frame->isFree())) {
        ELOG_WARN("waitForFrameFree timeout %d ms", timeout);

        dumpMsdkFrameInfo("waitFrameTimeout", frame);

        return false;
    }

    return true;
}

bool MsdkFramePool::freeFrames()
{
    mfxStatus sts = MFX_ERR_NONE;

    for (auto& it : m_framePool) {
        if (!waitForFrameFree(it, TIMEOUT)) {
            ELOG_WARN("Free frame in using is dangerous!");
        }

        dumpMsdkFrameInfo("free", it);
    }

    m_framePool.clear();

    sts = m_allocator->Free(m_allocator->pthis, &m_response);
    if (sts != MFX_ERR_NONE)
    {
        ELOG_ERROR("mfxFrameAllocator Free() failed, ret %d", sts);
    }

    return true;
}

bool MsdkFramePool::reAllocate(uint32_t width, uint32_t height)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG("reAllocate %dx%d -> %dx%d", m_allocatedWidth, m_allocatedHeight, width, height);

    if (width <= m_allocatedWidth && height <= m_allocatedHeight) {
        ELOG_DEBUG("Dont need resize into smaller size");

        return true;
    }

    // realloc
    freeFrames();

    m_request.Info.Width    = ALIGN16(width);
    m_request.Info.Height   = ALIGN16(height);
    m_request.Info.CropX    = 0;
    m_request.Info.CropY    = 0;
    m_request.Info.CropW    = width;
    m_request.Info.CropH    = height;

    if (!allocateFrames())
    {
        return false;
    }

    return true;
}

boost::shared_ptr<MsdkFrame> MsdkFramePool::getFreeFrame()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    for (auto& it : m_framePool) {
        if(it.use_count() == 1 && it->isFree()) {
            return it;
        }
    }

    return NULL;
}

boost::shared_ptr<MsdkFrame> MsdkFramePool::getFrame(mfxFrameSurface1 *pSurface)
{
    for (auto& it : m_framePool) {
        if(pSurface == it->getSurface()) {
            return it;
        }
    }

    return NULL;
}

}//namespace woogeen_base

#endif /* ENABLE_MSDK */
