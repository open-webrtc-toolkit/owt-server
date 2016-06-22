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

#include "MsdkBase.h"
#include "MsdkFrameDecoder.h"

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(MsdkFrameDecoder, "woogeen.MsdkFrameDecoder");

MsdkFrameDecoder::MsdkFrameDecoder()
    : m_session(NULL)
    , m_dec(NULL)
    , m_allocator(NULL)
    , m_videoParam(NULL)
    , m_bitstream(NULL)
    , m_decBsOffset(0)
    , m_framePool(NULL)
    , m_statDetectHeaderFrameCount(0)
    , m_ready(false)
{
    initDefaultParam();
}

MsdkFrameDecoder::~MsdkFrameDecoder()
{
    printfFuncEnter;

    if (m_dec) {
        m_dec->Close();
        delete m_dec;
        m_dec = NULL;
    }

    if (m_session) {
        //disjoint
        m_session->Close();
        delete m_session;
        m_session = NULL;
    }

    m_allocator.reset();

    m_videoParam.reset();

    if (m_bitstream && m_bitstream->Data) {
        free(m_bitstream->Data);
        m_bitstream->Data = NULL;
    }
    m_bitstream.reset();

    m_framePool.reset();

    printfFuncExit;
}

void MsdkFrameDecoder::initDefaultParam(void)
{
    m_videoParam.reset(new mfxVideoParam);
    memset(m_videoParam.get(), 0, sizeof(mfxVideoParam));
}

bool MsdkFrameDecoder::allocateFrames(void)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_framePool)
        m_framePool.reset(NULL);

    mfxFrameAllocRequest Request;
    memset(&Request, 0, sizeof(mfxFrameAllocRequest));

    sts = m_dec->QueryIOSurf(m_videoParam.get(), &Request);
    if (sts > 0) {
        ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
    }
    else if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("(%p)mfx QueryIOSurf() failed, ret %d", this, sts);
        return false;
    }

    ELOG_TRACE("(%p)mfx QueryIOSurf: Suggested(%d), Min(%d)", this, Request.NumFrameSuggested, Request.NumFrameMin);

    m_framePool.reset(new MsdkFramePool(m_allocator, Request));
    if (!m_framePool->init()) {
        ELOG_ERROR("(%p)Frame pool init failed, ret %d", this, sts);

        m_framePool.reset();
        return false;
    }

    return true;
}

void MsdkFrameDecoder::flushOutput(void)
{
    printfFuncEnter;

    MsdkFrameHolder holder;
    holder.frame = NULL;
    holder.cmd = MsdkCmd_DEC_FLUSH;

    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = FRAME_FORMAT_MSDK;
    frame.payload = reinterpret_cast<uint8_t*>(&holder);
    frame.length = 0;
    frame.additionalInfo.video.width = 0;
    frame.additionalInfo.video.height = 0;
    frame.timeStamp = 0;

    deliverFrame(frame);

    printfFuncExit;
}

bool MsdkFrameDecoder::resetDecoder(void)
{
    mfxStatus sts = MFX_ERR_NONE;

    ELOG_TRACE("(%p)resetDecoder", this);

    flushOutput();

    if (m_dec) {
        int previousWidth   = m_videoParam->mfx.FrameInfo.Width;
        int previousHeight  = m_videoParam->mfx.FrameInfo.Height;
        int previousCropW   = m_videoParam->mfx.FrameInfo.CropW;
        int previousCropH   = m_videoParam->mfx.FrameInfo.CropH;

        m_dec->GetVideoParam(m_videoParam.get());
        printfVideoParam(m_videoParam.get(), MFX_DEC);

        if (previousCropW != m_videoParam->mfx.FrameInfo.CropW
                || previousCropH != m_videoParam->mfx.FrameInfo.CropH) {
            ELOG_DEBUG("(%p)Reset reason - resolution changed %dx%d(crop %dx%d) -> %dx%d(crop %dx%d)", this
                    , previousWidth, previousHeight
                    , previousCropW, previousCropH
                    , m_videoParam->mfx.FrameInfo.Width, m_videoParam->mfx.FrameInfo.Height
                    , m_videoParam->mfx.FrameInfo.CropW, m_videoParam->mfx.FrameInfo.CropH
                    );
        }
        else {
            ELOG_DEBUG("(%p)Reset reason - unknown", this)
        }

#if 0
        sts = m_dec->Reset(m_videoParam.get());
        if (sts >= MFX_ERR_NONE) {
            ELOG_DEBUG("(%p)Reset decoder successfully!", this);

            return true;
        }
#endif

        //mfx reset does not work, try to re-initialize
        m_dec->Close();
        delete m_dec;
        m_dec = NULL;
    }

    m_dec = new MFXVideoDECODE(*m_session);
    if (!m_dec) {
        ELOG_ERROR("(%p)Create decode failed", this);

        return false;
    }

    ELOG_DEBUG("(%p)Re-create decoder successfully!", this);
    return true;
}

bool MsdkFrameDecoder::init(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_H264:

            m_videoParam->mfx.CodecId = MFX_CODEC_AVC;

            ELOG_DEBUG("(%p)Created H.264 deocder.", this);
            break;

        case FRAME_FORMAT_VP8:
        default:
            ELOG_ERROR("(%p)Unspported video frame format %d", this, format);
            return false;
    }

    mfxStatus sts = MFX_ERR_NONE;

    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase == NULL) {
        ELOG_ERROR("(%p)Get MSDK failed.", this);
        return false;
    }

    m_session = msdkBase->createSession();
    if (!m_session ) {
        ELOG_ERROR("(%p)Create session failed.", this);
        return false;
    }

    m_allocator = msdkBase->createFrameAllocator();
    if (!m_allocator) {
        ELOG_ERROR("(%p)Create frame allocator failed.", this);
        return false;
    }

    sts = m_session->SetFrameAllocator(m_allocator.get());
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("(%p)Set frame allocator failed.", this);
        return false;
    }

    m_dec = new MFXVideoDECODE(*m_session);
    if (!m_dec) {
        ELOG_ERROR("(%p)Create decode failed", this);

        return false;
    }

    return true;
}

bool MsdkFrameDecoder::decHeader(mfxBitstream *pBitstream)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_dec->DecodeHeader(pBitstream, m_videoParam.get());
    if (sts != MFX_ERR_NONE) {
        ELOG_TRACE("(%p)More data needed by DecodeHeader", this);

        if (!(m_statDetectHeaderFrameCount++ % 5))
        {
            ELOG_DEBUG("(%p)No header in last %d frames, request key frame!", this, m_statDetectHeaderFrameCount);

            FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
            deliverFeedbackMsg(msg);
        }

        return false;
    }

    ELOG_DEBUG("(%p)Decode header successed after %d frames!", this, m_statDetectHeaderFrameCount);
    m_statDetectHeaderFrameCount = 0;

    m_videoParam->IOPattern  = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    m_videoParam->AsyncDepth = 1 + MAX_DECODED_FRAME_IN_RENDERING;

    sts = m_dec->Init(m_videoParam.get());
    if (sts > 0) {
        ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
    }
    else if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("(%p)mfx init failed, ret %d", this, sts);

        printfVideoParam(m_videoParam.get(), MFX_DEC);
        return false;
    }

    m_dec->GetVideoParam(m_videoParam.get());
    printfVideoParam(m_videoParam.get(), MFX_DEC);

    if (!allocateFrames())
        return false;

    m_ready = true;
    return true;
}

void MsdkFrameDecoder::decFrame(mfxBitstream *pBitstream)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1 *pOutSurface = NULL;
    mfxSyncPoint syncP;

    // drain bitstream
    while (true) {
more_surface:
        boost::shared_ptr<MsdkFrame> workFrame = m_framePool->getFreeFrame();
        if (!workFrame)
        {
            ELOG_WARN("(%p)No WorkSurface available", this);
            return;
        }

retry:

//reserved for debug
#if 0
        ELOG_TRACE("(%p)***bitstream buffer offset %d, dataLength %d, maxLength %d",
            this, pBitstream->DataOffset, pBitstream->DataLength, pBitstream->MaxLength);

        dumpH264BitstreamInfo(pBitstream->Data + pBitstream->DataOffset, pBitstream->DataLength);
#endif

        m_decBsOffset = pBitstream->DataOffset;

        // no always to set worksurface, right?
        sts = m_dec->DecodeFrameAsync(pBitstream, workFrame->getSurface(), &pOutSurface, &syncP);
        if (sts == MFX_WRN_DEVICE_BUSY) {
            ELOG_TRACE("(%p)Device busy, retry!", this);

            usleep(1000); //1ms
            goto retry;
        }
        else if (sts == MFX_ERR_MORE_SURFACE) {
            ELOG_TRACE("(%p)Require more surface!", this);

            goto more_surface;
        }
        else if (sts == MFX_WRN_VIDEO_PARAM_CHANGED) { //ignore
            ELOG_TRACE("(%p)New sequence header!", this);

            goto retry;
        }
        else if (sts == MFX_ERR_MORE_DATA) {
            ELOG_TRACE("(%p)Require more data!", this);

            return;
        }
        else if (sts != MFX_ERR_NONE) {
            ELOG_INFO("(%p)mfx decode error, ret %d", this, sts);

            printfVideoParam(m_videoParam.get(), MFX_DEC);

            resetDecoder();

            // rollback current bs
            pBitstream->DataLength += pBitstream->DataOffset - m_decBsOffset;
            pBitstream->DataOffset = m_decBsOffset;

            m_ready = false;
            FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
            deliverFeedbackMsg(msg);

            return;
        }

#if 0
        sts = m_session->SyncOperation(syncP, MFX_INFINITE);
        if(sts != MFX_ERR_NONE)
        {
            ELOG_ERROR("(%p)SyncOperation failed, ret %d", this, sts);
        }
#endif

        //printfFrameInfo(&pOutSurface->Info);

        boost::shared_ptr<MsdkFrame> outFrame = m_framePool->getFrame(pOutSurface);
        outFrame->setSyncPoint(syncP);
        outFrame->setSyncFlag(true);
        //outFrame->dump();

        MsdkFrameHolder holder;
        holder.frame = outFrame;
        holder.cmd = MsdkCmd_NONE;

        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = FRAME_FORMAT_MSDK;
        frame.payload = reinterpret_cast<uint8_t*>(&holder);
        frame.length = 0;
        frame.additionalInfo.video.width = holder.frame->getWidth();
        frame.additionalInfo.video.height = holder.frame->getHeight();
        frame.timeStamp = 0;

        //ELOG_TRACE("timeStamp %u", frame.timeStamp);

        deliverFrame(frame);
    }
}

void MsdkFrameDecoder::updateBitstream(const Frame& frame)
{
    if(!m_bitstream)
    {
        int size = frame.additionalInfo.video.width * frame.additionalInfo.video.height * 3 / 2;

        if (!size)
            size = 1 * 1024 * 1024;

        m_bitstream.reset(new mfxBitstream());
        memset((void *)m_bitstream.get(), 0, sizeof(mfxBitstream));

        m_bitstream->Data         = (mfxU8 *)malloc(size);
        m_bitstream->MaxLength    = size;
        m_bitstream->DataOffset   = 0;
        m_bitstream->DataLength   = 0;

        ELOG_TRACE("(%p)Init bistream buffer, size %d (%dx%d).",
                this,
                size,
                frame.additionalInfo.video.width,
                frame.additionalInfo.video.height
                );
    }

    ELOG_TRACE("(%p)+++bitstream buffer offset %d, dataLength %d, maxLength %d",
            this, m_bitstream->DataOffset, m_bitstream->DataLength, m_bitstream->MaxLength);

    ELOG_TRACE("(%p)Incoming frame length: %d", this, frame.length);

    if(m_bitstream->MaxLength < m_bitstream->DataLength + frame.length)
    {
        uint32_t newSize = m_bitstream->MaxLength * 2;

        while (newSize < m_bitstream->DataLength + frame.length)
            newSize *= 2;

        ELOG_ERROR("(%p)bitstream buffer need to remalloc %d -> %d"
                , this
                , m_bitstream->MaxLength
                , newSize
                );

        mfxU8 *newBuffer = (mfxU8 *)malloc(newSize);
        if (m_bitstream->DataLength > 0)
            memcpy(newBuffer, m_bitstream->Data, m_bitstream->DataLength);

        free(m_bitstream->Data);
        m_bitstream->Data         = newBuffer;
        m_bitstream->MaxLength    = newSize;
    }

    if(m_bitstream->DataOffset + m_bitstream->DataLength + frame.length > m_bitstream->MaxLength)
    {
        memmove(m_bitstream->Data, m_bitstream->Data + m_bitstream->DataOffset, m_bitstream->DataLength);
        m_bitstream->DataOffset = 0;
        ELOG_TRACE("(%p)Move bitstream buffer offset", this);
    }

    memcpy(m_bitstream->Data + m_bitstream->DataOffset + m_bitstream->DataLength, frame.payload, frame.length);
    m_bitstream->DataLength += frame.length;

    ELOG_TRACE("(%p)---bitstream buffer offset %d, dataLength %d, maxLength %d",
            this, m_bitstream->DataOffset, m_bitstream->DataLength, m_bitstream->MaxLength);
}

void MsdkFrameDecoder::onFrame(const Frame& frame)
{
    printfFuncEnter;

    //dumpH264BitstreamInfo(frame);

    updateBitstream(frame);

retry:
    if (m_ready || decHeader(m_bitstream.get())) {
        decFrame(m_bitstream.get());

        // if decFrame reset, retry decHeader w/ current bs (most likely SPS) and continuely decFrame
        if (!m_ready)
            goto retry;
    }

    printfFuncExit;
}

void MsdkFrameDecoder::dumpFrameInfo(const Frame& frame)
{
    dumpH264BitstreamInfo(frame.payload, frame.length);
}

void MsdkFrameDecoder::dumpH264BitstreamInfo(uint8_t *data, int len)
{
    int nal_ref_idc;
    int nal_unit_type;
    const char *type = NULL;

    if (len < 5) {
        ELOG_DEBUG("(%p)Invalid bitstream head size(%d)", this, len);
        return;
    }

    if (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 1) {
        ELOG_ERROR("(%p)Invalid Start code(0001): %d%d%d%d", this, data[0], data[1], data[2], data[3]);
        return;
    }

    nal_ref_idc     = (data[4] >> 5) & 0x3;
    nal_unit_type   = data[4] & 0x1f;

    switch (nal_unit_type) {
        case 1:
            type = "NON-IDR";
            break;

        case 5:
            type = "IDR";
            break;

        case 7:
            type = "SPS";
            break;

        case 8:
            type = "PPS";
            break;

        default:
            type = "N/A";
            break;
    }

    ELOG_TRACE("(%p)nal_ref_idc %d, nal_unit_type %d(%s)", this, nal_ref_idc, nal_unit_type, type);
}

}//namespace woogeen_base
#endif /* ENABLE_MSDK */
