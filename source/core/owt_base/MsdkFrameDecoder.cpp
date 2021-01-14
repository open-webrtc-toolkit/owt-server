// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_MSDK

#include "MsdkFrameDecoder.h"

#define _MAX_BITSTREAM_BUFFER_ (100 * 1024 * 1024)

using namespace webrtc;

namespace owt_base {

DEFINE_LOGGER(MsdkFrameDecoder, "owt.MsdkFrameDecoder");

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
    , m_pluginID()
    , m_enableBsDump(false)
    , m_bsDumpfp(NULL)
{
    initDefaultParam();
}

MsdkFrameDecoder::~MsdkFrameDecoder()
{
    flushOutput();

    if (m_dec) {
        m_dec->Close();
        delete m_dec;
        m_dec = NULL;
    }

    if (m_session) {
        MsdkBase *msdkBase = MsdkBase::get();
        if (msdkBase) {
            msdkBase->unLoadPlugin(m_session, &m_pluginID);
            msdkBase->destroySession(m_session);
        }
    }

    m_allocator.reset();

    m_videoParam.reset();

    if (m_bitstream && m_bitstream->Data) {
        free(m_bitstream->Data);
        m_bitstream->Data = NULL;
    }
    m_bitstream.reset();

    m_framePool.reset();

    m_timeStamps.clear();

    if (m_bsDumpfp) {
        fclose(m_bsDumpfp);
    }
}

bool MsdkFrameDecoder::supportFormat(FrameFormat format)
{
    mfxU32 codecId = 0;
    switch (format) {
        case FRAME_FORMAT_H264:
            codecId = MFX_CODEC_AVC;
            break;
        case FRAME_FORMAT_H265:
            codecId = MFX_CODEC_HEVC;
            break;
        case FRAME_FORMAT_VP8:
            codecId = MFX_CODEC_VP8;
            break;
        default:
            return false;
    }

    MsdkBase *msdkBase = MsdkBase::get();
    if (!msdkBase)
        return false;

    return msdkBase->isSupportedDecoder(codecId);
}

void MsdkFrameDecoder::initDefaultParam(void)
{
    m_videoParam.reset(new mfxVideoParam);
    memset(m_videoParam.get(), 0, sizeof(mfxVideoParam));
}

bool MsdkFrameDecoder::allocateFrames(void)
{
    mfxStatus sts = MFX_ERR_NONE;

    m_framePool.reset();

    m_videoParam->IOPattern  = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    m_videoParam->AsyncDepth = 1 + MAX_DECODED_FRAME_IN_RENDERING;
    m_dec->Query(m_videoParam.get(), m_videoParam.get());

    mfxFrameAllocRequest Request;
    memset(&Request, 0, sizeof(mfxFrameAllocRequest));

    sts = m_dec->QueryIOSurf(m_videoParam.get(), &Request);
    if (sts > 0) {
        ELOG_TRACE_T("Ignore mfx warning, ret %d", sts);
    } else if (sts != MFX_ERR_NONE) {
        ELOG_ERROR_T("mfx QueryIOSurf() failed, ret %d", sts);
        return false;
    }

    ELOG_TRACE_T("mfx QueryIOSurf: Suggested(%d), Min(%d)", Request.NumFrameSuggested, Request.NumFrameMin);

    m_framePool.reset(new MsdkFramePool(Request, m_allocator));
    return true;
}

void MsdkFrameDecoder::flushOutput(void)
{
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
}

bool MsdkFrameDecoder::resetDecoder(void)
{
    ELOG_DEBUG_T("ResetDecoder");

    flushOutput();

    if (m_dec) {
#if 0
        sts = m_dec->Reset(m_videoParam.get());
        if (sts >= MFX_ERR_NONE) {
            ELOG_DEBUG_T("Reset decoder successfully!");

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
        ELOG_ERROR_T("Create decode failed");

        return false;
    }

    return true;
}

bool MsdkFrameDecoder::init(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_H264:
            m_videoParam->mfx.CodecId = MFX_CODEC_AVC;
            break;

        case FRAME_FORMAT_H265:
            m_videoParam->mfx.CodecId = MFX_CODEC_HEVC;
            break;

        case FRAME_FORMAT_VP8:
            m_videoParam->mfx.CodecId = MFX_CODEC_VP8;
            break;

        default:
            ELOG_ERROR_T("Unspported video frame format %s(%d)", getFormatStr(format), format);
            return false;
    }

    ELOG_DEBUG_T("Created %s decoder.", getFormatStr(format));

    mfxStatus sts = MFX_ERR_NONE;

    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase == NULL) {
        ELOG_ERROR_T("Get MSDK failed.");
        return false;
    }

    m_session = msdkBase->createSession();
    if (!m_session ) {
        ELOG_ERROR_T("Create session failed.");
        return false;
    }

    if (!msdkBase->loadDecoderPlugin(m_videoParam->mfx.CodecId, m_session, &m_pluginID)) {
        ELOG_ERROR_T("Load plugin failed.");
        return false;
    }

    m_allocator = msdkBase->createFrameAllocator();
    if (!m_allocator) {
        ELOG_ERROR_T("Create frame allocator failed.");
        return false;
    }

    sts = m_session->SetFrameAllocator(m_allocator.get());
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR_T("Set frame allocator failed.");
        return false;
    }

    m_dec = new MFXVideoDECODE(*m_session);
    if (!m_dec) {
        ELOG_ERROR_T("Create decode failed");

        return false;
    }

    if (m_enableBsDump) {
        char dumpFileName[128];

        snprintf(dumpFileName, 128, "/tmp/msdkFrameDecoder-%p.%s", this, getFormatStr(format));
        m_bsDumpfp = fopen(dumpFileName, "wb");
        if (m_bsDumpfp) {
            ELOG_DEBUG("Enable bitstream dump, %s", dumpFileName);
        } else {
            ELOG_DEBUG("Can not open dump file, %s", dumpFileName);
        }
    }

    return true;
}

bool MsdkFrameDecoder::decHeader(mfxBitstream *pBitstream)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_dec->DecodeHeader(pBitstream, m_videoParam.get());
    if (sts != MFX_ERR_NONE) {
        ELOG_TRACE_T("More data needed by DecodeHeader");

        if (!(m_statDetectHeaderFrameCount++ % 5)) {
            ELOG_DEBUG_T("No header in last %d frames, request key frame!", m_statDetectHeaderFrameCount);

            FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
            deliverFeedbackMsg(msg);
        }

        return false;
    }

    ELOG_DEBUG_T("Decode header successed after %d frames! Resolution %dx%d(%d-%d-%d-%d)"
            , m_statDetectHeaderFrameCount
            , m_videoParam->mfx.FrameInfo.Width, m_videoParam->mfx.FrameInfo.Height
            , m_videoParam->mfx.FrameInfo.CropX, m_videoParam->mfx.FrameInfo.CropY
            , m_videoParam->mfx.FrameInfo.CropW, m_videoParam->mfx.FrameInfo.CropH
            );
    m_statDetectHeaderFrameCount = 0;

    if (!allocateFrames()) {
        ELOG_ERROR_T("Failed to allocate frames for decoder.");
        return false;
    }

    sts = m_dec->Init(m_videoParam.get());
    if (sts > 0) {
        ELOG_TRACE_T("Ignore mfx warning, ret %d", sts);
    } else if (sts != MFX_ERR_NONE) {
        ELOG_ERROR_T("mfx init failed, ret %d", sts);

        MsdkBase::printfVideoParam(m_videoParam.get(), MFX_DEC);
        return false;
    }

    m_dec->GetVideoParam(m_videoParam.get());
    MsdkBase::printfVideoParam(m_videoParam.get(), MFX_DEC);

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
        if (!workFrame) {
            ELOG_WARN_T("No WorkSurface available");
            return;
        }

retry:

//reserved for debug
#if 0
        ELOG_TRACE_T("***bitstream buffer offset %d, dataLength %d, maxLength %d",
            pBitstream->DataOffset, pBitstream->DataLength, pBitstream->MaxLength);
#endif

        m_decBsOffset = pBitstream->DataOffset;

        sts = m_dec->DecodeFrameAsync(pBitstream, workFrame->getSurface(), &pOutSurface, &syncP);
        if (sts == MFX_WRN_DEVICE_BUSY) {
            ELOG_TRACE_T("Device busy, retry!");

            usleep(1000); //1ms
            goto retry;
        } else if (sts == MFX_ERR_MORE_SURFACE) {
            ELOG_TRACE_T("Require more surface!");

            goto more_surface;
        } else if (sts == MFX_WRN_VIDEO_PARAM_CHANGED) { //ignore
            ELOG_TRACE_T("New sequence header!");

            goto retry;
        } else if (sts == MFX_ERR_MORE_DATA) {
            ELOG_TRACE_T("Require more data!");

            return;
        } else if (sts != MFX_ERR_NONE) {
            if (sts == MFX_ERR_INCOMPATIBLE_VIDEO_PARAM) {
                ELOG_DEBUG_T("Stream changed!");
            } else {
                ELOG_ERROR_T("mfx decode error, ret %d", sts);
            }

            workFrame.reset();

            MsdkBase::printfVideoParam(m_videoParam.get(), MFX_DEC);

            resetDecoder();

            // rollback current bs
            pBitstream->DataLength += pBitstream->DataOffset - m_decBsOffset;
            pBitstream->DataOffset = m_decBsOffset;

            m_ready = false;
            return;
        }

#if 0
        sts = m_session->SyncOperation(syncP, MFX_INFINITE);
        if(sts != MFX_ERR_NONE) {
            ELOG_ERROR_T("SyncOperation failed, ret %d", sts);
        }
#endif

        //printfFrameInfo(&pOutSurface->Info);

        boost::shared_ptr<MsdkFrame> outFrame = m_framePool->getFrame(pOutSurface);
        outFrame->setSyncPoint(syncP);
        outFrame->setSyncFlag(true);

        //workaroung vp8 vpp issue
        if (m_videoParam->mfx.CodecId == MFX_CODEC_VP8)
            outFrame->getSurface()->Info.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

        //outFrame->dump();

        MsdkFrameHolder holder;
        holder.frame = outFrame;
        holder.cmd = MsdkCmd_NONE;

        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = FRAME_FORMAT_MSDK;
        frame.payload = reinterpret_cast<uint8_t*>(&holder);
        frame.length = 0;
        frame.additionalInfo.video.width = holder.frame->getVideoWidth();
        frame.additionalInfo.video.height = holder.frame->getVideoHeight();

        if (m_timeStamps.size() > 0) {
            frame.timeStamp = m_timeStamps.front();
            m_timeStamps.pop_front();
        } else {
            ELOG_ERROR_T("No timeStamp available in queue");
        }

        deliverFrame(frame);
    }
}

void MsdkFrameDecoder::updateBitstream(const Frame& frame)
{
    if(!m_bitstream) {
        int size = frame.additionalInfo.video.width * frame.additionalInfo.video.height * 3 / 2;

        if (!size)
            size = 1 * 1024 * 1024;

        m_bitstream.reset(new mfxBitstream());
        memset((void *)m_bitstream.get(), 0, sizeof(mfxBitstream));

        m_bitstream->Data         = (mfxU8 *)malloc(size);
        if (m_bitstream->Data == nullptr) {
            m_bitstream.reset();
            ELOG_ERROR_T("OOM! Allocate size %d", size);
            return;
        }

        m_bitstream->MaxLength    = size;
        m_bitstream->DataOffset   = 0;
        m_bitstream->DataLength   = 0;

        ELOG_TRACE_T("Init bistream buffer, size %d (%dx%d).",
                size,
                frame.additionalInfo.video.width,
                frame.additionalInfo.video.height
                );
    }

    ELOG_TRACE_T("+++bitstream buffer offset %d, dataLength %d, maxLength %d",
            m_bitstream->DataOffset, m_bitstream->DataLength, m_bitstream->MaxLength);

    ELOG_TRACE_T("Incoming frame length: %d", frame.length);

    if(m_bitstream->MaxLength < m_bitstream->DataLength + frame.length) {
        uint32_t newSize = m_bitstream->MaxLength * 2;

        while (newSize < m_bitstream->DataLength + frame.length)
            newSize *= 2;

        if (newSize > _MAX_BITSTREAM_BUFFER_) {
            ELOG_ERROR_T("Exceed max bitstream buffer size(%d), %d!",  _MAX_BITSTREAM_BUFFER_, newSize);
            assert(0);
        }

        ELOG_DEBUG_T("bitstream buffer need to remalloc %d -> %d"
                , m_bitstream->MaxLength
                , newSize
                );

        m_bitstream->Data         = (mfxU8 *)realloc(m_bitstream->Data, newSize);
        if (m_bitstream->Data == nullptr) {
            m_bitstream.reset();
            ELOG_ERROR_T("OOM! Allocate size %d", newSize);
            return;
        }

        m_bitstream->MaxLength    = newSize;
    }

    if(m_bitstream->DataOffset + m_bitstream->DataLength + frame.length > m_bitstream->MaxLength) {
        if (m_bitstream->Data) {
            memmove(m_bitstream->Data, m_bitstream->Data + m_bitstream->DataOffset, m_bitstream->DataLength);
            m_bitstream->DataOffset = 0;
            ELOG_TRACE_T("Move bitstream buffer offset");
        } else { // make code scanner happy
            return;
        }
    }

    memcpy(m_bitstream->Data + m_bitstream->DataOffset + m_bitstream->DataLength, frame.payload, frame.length);
    m_bitstream->DataLength += frame.length;

    ELOG_TRACE_T("bitstream buffer offset %d, dataLength %d, maxLength %d",
            m_bitstream->DataOffset, m_bitstream->DataLength, m_bitstream->MaxLength);
}

void MsdkFrameDecoder::onFrame(const Frame& frame)
{
    if (frame.payload == NULL || frame.length == 0) {
        ELOG_DEBUG_T("Null frame, request key frame");
        FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
        deliverFeedbackMsg(msg);
        return;
    }

    dump(frame.payload, frame.length);
    updateBitstream(frame);

retry:
    if (m_ready || decHeader(m_bitstream.get())) {
        m_timeStamps.push_back(frame.timeStamp);
        decFrame(m_bitstream.get());

        // if decFrame reset, retry decHeader w/ current bs (most likely SPS) and continuely decFrame
        if (!m_ready) {
            m_timeStamps.clear();
            goto retry;
        }
    }
}

void MsdkFrameDecoder::dump(uint8_t *buf, int len)
{
    if (m_bsDumpfp) {
        fwrite(buf, 1, len, m_bsDumpfp);
    }
}

}//namespace owt_base
#endif /* ENABLE_MSDK */
