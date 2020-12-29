// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_MSDK

#include <malloc.h>

#include "FastCopy.h"

#include "MsdkBase.h"
#include "MsdkFrame.h"

namespace owt_base {

struct FreeDelete
{
    void operator()(void* x) { free(x); }
};

struct MockDelete
{
    void operator()(void* x) { }
};

DEFINE_LOGGER(MsdkFrame, "owt.MsdkFrame");

MsdkFrame::MsdkFrame(uint32_t width, uint32_t height, boost::shared_ptr<mfxFrameAllocator> allocator)
    : m_allocator(allocator)
    , m_valid(false)
    , m_externalAlloc(false)
    , m_mainSession(NULL)
    , m_needSync(false)
    , m_nv12TBuffer(nullptr, FreeDelete())
    , m_nv12TBufferSize(0)
{
    memset(&m_response, 0, sizeof(mfxFrameAllocResponse));
    memset(&m_surface, 0, sizeof(mfxFrameSurface1));

    memset(&m_request, 0, sizeof(mfxFrameAllocRequest));

    m_request.Type = MFX_MEMTYPE_FROM_VPPIN | MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET | MFX_MEMTYPE_EXTERNAL_FRAME;

    m_request.NumFrameMin         = 1;
    m_request.NumFrameSuggested   = 1;

    m_request.Info.FourCC         = MFX_FOURCC_NV12;
    m_request.Info.ChromaFormat   = MFX_CHROMAFORMAT_YUV420;
    m_request.Info.PicStruct      = MFX_PICSTRUCT_PROGRESSIVE;

    m_request.Info.BitDepthLuma   = 0;
    m_request.Info.BitDepthChroma = 0;
    m_request.Info.Shift          = 0;

    m_request.Info.AspectRatioW   = 0;
    m_request.Info.AspectRatioH   = 0;

    m_request.Info.FrameRateExtN  = 30;
    m_request.Info.FrameRateExtD  = 1;

    m_request.Info.Width          = ALIGN16(width);
    m_request.Info.Height         = ALIGN16(height);
    m_request.Info.CropX          = 0;
    m_request.Info.CropY          = 0;
    m_request.Info.CropW          = width;
    m_request.Info.CropH          = height;
}

MsdkFrame::MsdkFrame(mfxFrameInfo &info, mfxMemId &id, boost::shared_ptr<mfxFrameAllocator> allocator)
    : m_allocator(allocator)
    , m_valid(false)
    , m_externalAlloc(true)
    , m_mainSession(NULL)
    , m_needSync(false)
    , m_nv12TBuffer(nullptr, FreeDelete())
    , m_nv12TBufferSize(0)
{
    memset(&m_response, 0, sizeof(mfxFrameAllocResponse));
    memset(&m_surface, 0, sizeof(mfxFrameSurface1));
    memset(&m_request, 0, sizeof(mfxFrameAllocRequest));

    m_surface.Info = info;
    m_surface.Data.MemId = id;
    m_valid = true;
}

MsdkFrame::~MsdkFrame()
{
    if (m_valid) {
        mfxStatus sts = MFX_ERR_NONE;

        uint32_t i = 0;
        while (m_surface.Data.Locked > 0 && i < TIMEOUT) {
          ELOG_DEBUG("Wait(%d ms), locked %d...", i, m_surface.Data.Locked);
          usleep(1000); //1ms
          i++;
        }

        if (m_surface.Data.Locked > 0) {
            ELOG_WARN("Free surface lock count, %d > 0", m_surface.Data.Locked);
        }

        if (!m_externalAlloc) {
            sts = m_allocator->Free(m_allocator->pthis, &m_response);
            if (sts != MFX_ERR_NONE) {
                ELOG_ERROR("mfxFrameAllocator Free() failed, ret %d", sts);
            }
        }
    }
}

bool MsdkFrame::init()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_externalAlloc)
        return m_valid;

    sts = m_allocator->Alloc(m_allocator->pthis, &m_request, &m_response);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("mfxFrameAllocator Alloc failed, ret %d", sts);
        return false;
    }

    if (m_response.NumFrameActual != 1) {
        ELOG_ERROR("mfxFrameAllocator Alloc invalid frame num, %d", m_response.NumFrameActual);
        return false;
    }

    m_surface.Info = m_request.Info;
    m_surface.Data.MemId = m_response.mids[0];
    m_valid = true;
    return true;
}

void MsdkFrame::sync(void)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!m_needSync)
        return;

    m_needSync = false;

    if (!m_mainSession) {
        MsdkBase *msdkBase = MsdkBase::get();
        if (msdkBase)
            m_mainSession = msdkBase->getMainSession();

        if (!m_mainSession) {
            ELOG_ERROR("Invalid main session!");
            return;
        }
    }

    sts = m_mainSession->SyncOperation(m_syncP, MFX_INFINITE);
    if(sts != MFX_ERR_NONE)
    {
        ELOG_ERROR("SyncOperation failed, ret %d", sts);
        return;
    }
}

bool MsdkFrame::setCrop(uint32_t cropX, uint32_t cropY, uint32_t cropW, uint32_t cropH)
{
    ELOG_TRACE("setCrop %d-%d-%d-%d(%dx%d) -> %d-%d-%d-%d"
            , m_surface.Info.CropX, m_surface.Info.CropY
            , m_surface.Info.CropW, m_surface.Info.CropH
            , m_surface.Info.Width, m_surface.Info.Height
            , cropX, cropY
            , cropW, cropH
            );

    if (cropX + cropW > m_surface.Info.Width || cropY + cropH >  m_surface.Info.Height) {
        ELOG_ERROR("Can not crop %dx%d into %d-%d-%d-%d"
                , m_surface.Info.Width, m_surface.Info.Height
                , cropX, cropY, cropW, cropH);

        return false;
    }

    m_surface.Info.CropX = cropX;
    m_surface.Info.CropY = cropY;
    m_surface.Info.CropW = cropW;
    m_surface.Info.CropH = cropH;

    if (m_nv12TBuffer.get() && m_nv12TBufferSize < cropW * cropH * 3 / 2) {
        m_nv12TBuffer.reset();
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

bool MsdkFrame::convertFrom(webrtc::VideoFrameBuffer *buffer)
{
    bool ret = false;
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1 *pSurface = &m_surface;
    mfxFrameInfo& pInfo = pSurface->Info;
    mfxFrameData& pData = pSurface->Data;

    mfxU16 w, h, pitch;
    mfxU8 *ptr;
    mfxU32 i, j;

    uint32_t srcW = buffer->width();
    uint32_t srcH = buffer->height();

    uint32_t srcStrideY = buffer->StrideY();
    uint32_t srcStrideU = buffer->StrideU();;
    uint32_t srcStrideV = buffer->StrideV();;

    const uint8_t *pSrcY = buffer->DataY();;
    const uint8_t *pSrcU = buffer->DataU();;
    const uint8_t *pSrcV = buffer->DataV();;

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

    if (pInfo.CropX != 0 || pInfo.CropY != 0) {
        ELOG_ERROR("Dont support set x, y crop: %d-%d", pInfo.CropX, pInfo.CropY);
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
        ELOG_WARN("Not support scale/crop src VideoFrame(%dx%d) into dst msdk surface(%dx%d)!",
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

bool MsdkFrame::nv12ConvertTo(mfxFrameInfo& pInfo, mfxFrameData& pData, webrtc::I420Buffer *buffer)
{
    uint32_t w = getCropW();
    uint32_t h = getCropH();

    if (!m_nv12TBuffer.get()) {
        m_nv12TBuffer.reset((uint8_t*) memalign(16, h * pData.Pitch * 3 / 2));
        if (!m_nv12TBuffer.get()) {
            ELOG_ERROR("memalign failed, %p", m_nv12TBuffer.get());

            return false;
        }
        m_nv12TBufferSize = h * pData.Pitch * 3 / 2;
    }

    if (m_nv12TBufferSize < h * pData.Pitch * 3 / 2) {
        ELOG_ERROR("invalid m_nv12TBufferSize, %ld", m_nv12TBufferSize);
        return false;
    }

    boost::shared_ptr<uint8_t> uvPos(m_nv12TBuffer.get() + h * pData.Pitch, MockDelete());
    memcpy_from_uswc_sse4(m_nv12TBuffer, pData.Y, h * pData.Pitch);
    memcpy_from_uswc_sse4(uvPos, pData.UV, h * pData.Pitch / 2);

    //uint32_t dstW, dstH;
    uint32_t dstStrideY, dstStrideU, dstStrideV;
    uint8_t *pDstY, *pDstU, *pDstV;

    dstStrideY = buffer->StrideY();
    dstStrideU = buffer->StrideU();
    dstStrideV = buffer->StrideV();

    pDstY = buffer->MutableDataY();
    pDstU = buffer->MutableDataU();
    pDstV = buffer->MutableDataV();

    uint8_t *ptrY;
    uint8_t *ptrUV;

    ptrY = m_nv12TBuffer.get();
    ptrUV = m_nv12TBuffer.get() + h * pData.Pitch;

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

bool MsdkFrame::convertTo(webrtc::I420Buffer *buffer)
{
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

    if (pInfo.CropX != 0 || pInfo.CropY != 0) {
        ELOG_ERROR("Dont support set x, y crop: %d-%d", pInfo.CropX, pInfo.CropY);

        return false;
    }

    if (m_surface.Info.CropW != buffer->width() || m_surface.Info.CropH != buffer->height()) {
        ELOG_WARN("Dont support scale/crop msdk frame(%dx%d) into to i420 buffer(%dx%d)!",
                m_surface.Info.CropW, m_surface.Info.CropH, buffer->width(), buffer->height());
        return false;
    }

    sync();

    sts = m_allocator->Lock(m_allocator.get()->pthis, pData.MemId, &pData);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Failed to lock surface!");

        return false;
    }

    int ret;
    switch (pSurface->Info.FourCC)
    {
        case MFX_FOURCC_NV12:
            ret = nv12ConvertTo(pInfo, pData, buffer);
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

DEFINE_LOGGER(MsdkFramePool, "owt.MsdkFramePool");

MsdkFramePool::MsdkFramePool(const uint32_t width, const uint32_t height, const int32_t count, boost::shared_ptr<mfxFrameAllocator> allocator)
{
    for (int i = 0; i < count; i++) {
        boost::shared_ptr<MsdkFrame> frame(new MsdkFrame(width, height, allocator));
        if (!frame->init()) {
            continue;
        }
        m_framePool.push_back(frame);
    }
}

MsdkFramePool::MsdkFramePool(mfxFrameAllocRequest &request, boost::shared_ptr<mfxFrameAllocator> allocator)
    : m_allocator(allocator)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_allocator->Alloc(m_allocator->pthis, &request, &m_response);
    if (sts != MFX_ERR_NONE) {
        memset(&m_response, 0, sizeof(m_response));
        ELOG_ERROR("mfxFrameAllocator Alloc failed, ret %d", sts);
        return;
    }

    for (int i = 0; i < m_response.NumFrameActual; i++) {
        boost::shared_ptr<MsdkFrame> frame(new MsdkFrame(request.Info, m_response.mids[i], m_allocator));
        if (!frame->init()) {
            continue;
        }
        m_framePool.push_back(frame);
    }
}

MsdkFramePool::~MsdkFramePool()
{
    m_framePool.clear();
    if (m_allocator && m_response.NumFrameActual > 0) {
        mfxStatus sts = MFX_ERR_NONE;

        sts = m_allocator->Free(m_allocator->pthis, &m_response);
        if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("mfxFrameAllocator Free() failed, ret %d", sts);
        }
    }
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
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    for (auto& it : m_framePool) {
        if(pSurface == it->getSurface()) {
            return it;
        }
    }

    return NULL;
}

void MsdkFramePool::dumpInfo()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    int i;

    i = 0;
    for (auto& it : m_framePool) {
        ELOG_DEBUG("Frame(%d), use_count(%ld), isFree(%d)"
                , i, it.use_count(), it->isFree());
        i++;
    }
}

}//namespace owt_base

#endif /* ENABLE_MSDK */
