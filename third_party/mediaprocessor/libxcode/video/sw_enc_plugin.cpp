/* <COPYRIGHT_TAG> */

#include "sw_enc_plugin.h"

#ifdef ENABLE_SW_CODEC
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "base/fast_copy.h"
#include "base/trace.h"
#include "base/media_common.h"

DEFINE_MLOGINSTANCE_CLASS(SWEncPlugin, "SWEncPlugin");
SWEncPlugin::SWEncPlugin()
{
    memset(&m_mfxPluginParam_, 0, sizeof(m_mfxPluginParam_));
    m_mfxPluginParam_.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_mfxPluginParam_.MaxThreadNum = 1;
    m_MaxNumTasks_ = 0;

    init_flag_ = false;
    frame_buf_ = NULL;
    outbuf_ = NULL;

    c_ = NULL;
    m_pYUVFrame_ = NULL;
    m_enc_num_ = 0;
}

mfxStatus SWEncPlugin::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    if (in && par) {
        in->Info = par->mfx.FrameInfo;
        in->NumFrameMin = 1;
        in->Type = MFX_MEMTYPE_OPAQUE_FRAME + MFX_MEMTYPE_FROM_ENCODE;
        in->NumFrameSuggested = in->NumFrameMin + par->AsyncDepth;
    } else {
        FRMW_TRACE_ERROR("Alloc request parameter is NULL");
        return MFX_ERR_NULL_PTR;
    }
    return MFX_ERR_NONE;
}

mfxStatus SWEncPlugin::Init(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = NULL;
    int i = 0;

    if (!par) {
        FRMW_TRACE_ERROR("Input parameter is NULL");
        return MFX_ERR_NULL_PTR;
    } else {
        m_VideoParam_ = par;
    }

    // Should be opaque case.
    if ((m_VideoParam_->IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY) == 0) {
        FRMW_TRACE_ERROR("IOPattern is 0x%x", m_VideoParam_->IOPattern);
        return MFX_ERR_UNKNOWN;
    }

    m_MaxNumTasks_ = m_VideoParam_->AsyncDepth + 1;
    FRMW_TRACE_DEBUG("Max num task is %d", m_MaxNumTasks_);

    while (!pluginOpaqueAlloc && (i < m_VideoParam_->NumExtParam)) {
        pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(m_VideoParam_->ExtParam,\
                                                                     m_VideoParam_->NumExtParam,\
                                                                     MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        i++;
    }
    if (pluginOpaqueAlloc != NULL) {
        if (pluginOpaqueAlloc->In.Surfaces == NULL) {
            assert(0);
            return MFX_ERR_NULL_PTR;
        } else {
            sts = m_pmfxCore_->MapOpaqueSurface(m_pmfxCore_->pthis,
            pluginOpaqueAlloc->In.NumSurface,
            pluginOpaqueAlloc->In.Type,
            pluginOpaqueAlloc->In.Surfaces);
            assert(MFX_ERR_NONE == sts);
        }
    } else {
        FRMW_TRACE_ERROR("Opaque alloc is NULL");
        return MFX_ERR_NULL_PTR;
    }

    // Create internal task pool
    m_pTasks_ = new MFXTask_SW_ENC[m_MaxNumTasks_];
    memset(m_pTasks_, 0, sizeof(MFXTask_SW_ENC) * m_MaxNumTasks_);

    // check input parameter limits
    if (MFX_ERR_NONE != CheckParameters(par)) {
        FRMW_TRACE_ERROR("Input parameters have error");
        return MFX_ERR_UNKNOWN;
    }

    if (!init_flag_) {
        sts = InitSWEnc();
        assert(MFX_ERR_NONE == sts);
        init_flag_ = true;
    }

    return MFX_ERR_NONE;
}

mfxStatus SWEncPlugin::Close()
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = NULL;
    int i = 0;

    while (!pluginOpaqueAlloc && (i < m_VideoParam_->NumExtParam)) {
        pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(m_VideoParam_->ExtParam,
                                                                     m_VideoParam_->NumExtParam,\
                                                                     MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        i++;
    }
    if (pluginOpaqueAlloc != NULL) {
        sts = m_pmfxCore_->UnmapOpaqueSurface(m_pmfxCore_->pthis,
                                             pluginOpaqueAlloc->In.NumSurface,
                                             pluginOpaqueAlloc->In.Type,
                                             pluginOpaqueAlloc->In.Surfaces);
        if (m_pTasks_) {
            delete [] m_pTasks_;
            m_pTasks_ = NULL;
        }
        if (MFX_ERR_NONE != sts) {
            FRMW_TRACE_ERROR("Failed to umap opaque mem");
        }
        return MFX_ERR_NONE;
    } else {
        FRMW_TRACE_WARNI("SW Enc PTR is NULL");
        return MFX_ERR_NULL_PTR;
    }
}

mfxStatus SWEncPlugin::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_VideoParam_->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV420) {
        par->mfx.BufferSizeInKB = ((m_VideoParam_->mfx.FrameInfo.Width) *\
                                   (m_VideoParam_->mfx.FrameInfo.Height) * 3 / 2 +\
                                   (m_VideoParam_->mfx.FrameInfo.Width) *\
                                   (m_VideoParam_->mfx.FrameInfo.Height) * 3 % 2\
                                  ) / 1000 + 1;
    }

    return sts;
}

SWEncPlugin::~SWEncPlugin()
{
    if (frame_buf_) {
        av_free(frame_buf_);
        frame_buf_ = NULL;
    }

    if (m_pYUVFrame_) {
        av_free(m_pYUVFrame_);
        m_pYUVFrame_ = NULL;
    }

    if (outbuf_) {
        free(outbuf_);
        outbuf_ = NULL;
    }

    avcodec_close(c_);
}

mfxExtBuffer *SWEncPlugin::GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
{
    if (!ebuffers) {
        return 0;
    }
    for(mfxU32 i=0; i<nbuffers; i++) {
        if (!ebuffers[i]) {
            continue;
        }
        if (ebuffers[i]->BufferId == BufferId) {
            return ebuffers[i];
        }
    }
    return 0;
}

mfxStatus SWEncPlugin::PluginInit(mfxCoreInterface *core)
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pmfxCore_ = new mfxCoreInterface;
    if(!m_pmfxCore_) {
        return MFX_ERR_MEMORY_ALLOC;
    }
    *m_pmfxCore_ = *core;
    m_pAlloc_ = &m_pmfxCore_->FrameAllocator;

    return sts;
}

mfxStatus SWEncPlugin::PluginClose()
{
    mfxStatus sts = MFX_ERR_NONE;

    if(m_pmfxCore_) {
        delete m_pmfxCore_;
        m_pmfxCore_ = NULL;
    }

    return sts;
}

mfxStatus SWEncPlugin::GetPluginParam(mfxPluginParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    *par = m_mfxPluginParam_;

    return sts;
}

mfxStatus SWEncPlugin::Submit(const mfxHDL *in, mfxU32 , const mfxHDL *out, mfxU32 , mfxThreadTask *task)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxBitstream *bitstream_out = NULL;
    mfxFrameSurface1 *opaque_surface_in = NULL;
    mfxFrameSurface1 *real_surf_in = NULL;

    // find free task in task array
    int taskIdx;
    for(taskIdx = 0; taskIdx < m_MaxNumTasks_; taskIdx++) {
        if (!m_pTasks_[taskIdx].bBusy) {
            break;
        }
    }
    if (taskIdx == m_MaxNumTasks_) {
        return MFX_WRN_DEVICE_BUSY; // currently there are no free tasks available
    }
    MFXTask_SW_ENC *pTask = &m_pTasks_[taskIdx];

    bitstream_out = (mfxBitstream *)out;
    opaque_surface_in = (mfxFrameSurface1 *)in[0];
    if (!bitstream_out || !opaque_surface_in) {
        return MFX_ERR_MORE_SURFACE;
    }

    sts = m_pmfxCore_->GetRealSurface(m_pmfxCore_->pthis, opaque_surface_in, &real_surf_in);
    if(sts != MFX_ERR_NONE) {
        assert(0);
    }

    m_BS_ = bitstream_out;
    pTask->bitstreamOut = bitstream_out;
    pTask->surfaceIn = real_surf_in;
    pTask->bBusy        = true;

    *task = (mfxThreadTask)pTask;
    m_pmfxCore_->IncreaseReference(m_pmfxCore_->pthis, &pTask->surfaceIn->Data);

    return sts;
}

mfxStatus SWEncPlugin::Execute(mfxThreadTask task, mfxU32 , mfxU32 uid_a)
{
    MFXTask_SW_ENC *pTask = (MFXTask_SW_ENC *)task;
    mfxStatus sts = MFX_ERR_NONE;
    int ret = 0;

    mfxFrameSurface1 *surf_in = pTask->surfaceIn;
    sts = m_pAlloc_->Lock(m_pAlloc_->pthis, surf_in->Data.MemId, &surf_in->Data);
    if (sts != MFX_ERR_NONE) {
        FRMW_TRACE_ERROR("Lock return %d", sts);
    }

    /* get input frame */
    sts = GetInputYUV(surf_in);
    if (MFX_ERR_NONE != sts) {
        FRMW_TRACE_ERROR("Get yuv failed");
    }

    ret = avcodec_encode_video(c_, outbuf_, outbuf_size_, m_pYUVFrame_);
    if (ret >= 0) {
        memcpy(m_BS_->Data + m_BS_->DataLength, outbuf_, ret);
        m_BS_->DataLength += ret;
        m_pYUVFrame_->pts++;
        m_enc_num_++;
    }

    sts = m_pAlloc_->Unlock(m_pAlloc_->pthis, surf_in->Data.MemId, &surf_in->Data);
    if (sts != MFX_ERR_NONE) {
        FRMW_TRACE_ERROR("Unlock return %d", sts);
    }

    return MFX_TASK_DONE;
}

mfxStatus SWEncPlugin::FreeResources(mfxThreadTask task, mfxStatus )
{
    MFXTask_SW_ENC *pTask = (MFXTask_SW_ENC *)task;

    m_pmfxCore_->DecreaseReference(m_pmfxCore_->pthis, &pTask->surfaceIn->Data);
    //m_pmfxCore_->DecreaseReference(m_pmfxCore_->pthis, &pTask->surfaceOut->Data);

    pTask->bBusy = false;

    return MFX_ERR_NONE;
}

mfxStatus SWEncPlugin::SetAuxParams(void *auxParam, int auxParamSize)
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}

void SWEncPlugin::Release()
{
    return;
}

mfxStatus SWEncPlugin::InitSWEnc()
{
    int bit_rate = 0;

    av_register_all();
    avcodec_register_all();

    /* find it */
    pCodec_ = avcodec_find_encoder(enc_opt_.codec_id);
    if(!pCodec_)
    {
        MLOG_ERROR("Codec can not found\n");
        return MFX_ERR_NULL_PTR;
    } else {
        if (AV_CODEC_ID_H264 == enc_opt_.codec_id) {
            MLOG_INFO("h264 codec found\n");
        } else if (AV_CODEC_ID_H263 == enc_opt_.codec_id) {
            MLOG_INFO("h263 codec found\n");
        } else {
            MLOG_INFO("Other codec found\n");
        }
    }

    c_= avcodec_alloc_context3(pCodec_);

    /* set it*/
    c_->codec_id = enc_opt_.codec_id;
    c_->codec_type = AVMEDIA_TYPE_VIDEO;
    c_->pix_fmt = AV_PIX_FMT_YUV420P;

    if (AV_CODEC_ID_H264 == c_->codec_id) {
        av_opt_set(c_->priv_data, "preset", "superfast", 0);
        //av_opt_set(c_->priv_data, "tune", "zerolatency", 0);
    }

    c_->width = m_width_;
    c_->height = m_height_;

    c_->time_base.num = enc_opt_.time_base_num;
    c_->time_base.den = enc_opt_.time_base_den;

    c_->gop_size = enc_opt_.gop_size;
    c_->keyint_min =  enc_opt_.gop_size;
    c_->max_b_frames = 0;
    c_->slices = enc_opt_.num_slice;
    //c_->refs = 1;
    //c_->thread_count = 1;

    c_->profile = enc_opt_.profile;
    c_->level = enc_opt_.level;

    if (enc_opt_.bit_rate) {
        bit_rate = enc_opt_.bit_rate;
    } else {
        bit_rate = c_->width * c_->height;
    }

    if (AV_CODEC_ID_H264 == c_->codec_id) {
        switch (enc_opt_.rate_ctrl) {
            case SW_CBR:
                c_->bit_rate = bit_rate;
                c_->rc_min_rate = c_->bit_rate;
                c_->rc_max_rate = c_->bit_rate;
                c_->bit_rate_tolerance = c_->bit_rate;
                c_->rc_buffer_size = c_->bit_rate;
                c_->rc_initial_buffer_occupancy = c_->rc_buffer_size*3/4;
                c_->rc_buffer_aggressivity= (float)1.0;
                c_->rc_initial_cplx= 0.5;
                break;
            case SW_VBR:
                c_->bit_rate = bit_rate;
                c_->flags |= CODEC_FLAG_QSCALE;
                c_->rc_min_rate = BIT_RATE_MIN;
                c_->rc_max_rate = BIT_RATE_MAX;
                break;
            case SW_CQP:
                c_->qmin = enc_opt_.qp_min;
                c_->qmax = enc_opt_.qp_max;
                c_->max_qdiff = 5;
                break;
            default:
                break;
        }
    }

    /* open it */
    if (avcodec_open2(c_, pCodec_, NULL) < 0) {
        MLOG_ERROR("could not open codec\n");
        return MFX_ERR_NULL_PTR;
    } else {
        MLOG_INFO("Codec open success\n");
    }

    if (!m_pYUVFrame_) {
        m_pYUVFrame_ = avcodec_alloc_frame();
    }

    frame_size_ = avpicture_get_size(c_->pix_fmt, c_->width, c_->height);

    if (!frame_buf_) {
        frame_buf_ = (uint8_t *)av_malloc(frame_size_);
        avpicture_fill((AVPicture*)(m_pYUVFrame_), (uint8_t*)frame_buf_, c_->pix_fmt, c_->width, c_->height);
        m_pYUVFrame_->data[0] = frame_buf_;
        m_pYUVFrame_->data[1] = frame_buf_ + c_->width * c_->height;
        m_pYUVFrame_->data[2] = frame_buf_ + c_->width * c_->height * 5 / 4;

        m_pYUVFrame_->linesize[0] = c_->width;
        m_pYUVFrame_->linesize[1] = c_->width / 2;
        m_pYUVFrame_->linesize[2] = c_->width / 2;
        m_pYUVFrame_->pts = 0;
    }

    if (!frame_buf_)
    {
        MLOG_ERROR("av_malloc fail\n");
        return MFX_ERR_NULL_PTR;
    }

    outbuf_size_ = frame_size_ * 50;
    if (!outbuf_) {
        outbuf_ = (uint8_t*)malloc(sizeof(uint8_t) * outbuf_size_);
    }

    if (!outbuf_)
    {
        MLOG_ERROR("outbuf_ malloc fail\n");
        return MFX_ERR_NULL_PTR;
    }

    return MFX_ERR_NONE;
}

mfxStatus SWEncPlugin::GetInputYUV(mfxFrameSurface1 *surf)
{
    if (surf && frame_buf_) {
        /* trans NV12 to I420 */
        if (MFX_FOURCC_NV12 == surf->Info.FourCC) {
            unsigned char *sY = surf->Data.Y;
            unsigned char *sUV = surf->Data.VU;
            unsigned char *dY = frame_buf_;
            unsigned char *dU = NULL;
            unsigned char *dV = NULL;

            /* init swap buffer*/
            if (!swap_buf_) {
                swap_buf_ = (unsigned char*)malloc(m_height_ * surf->Data.Pitch + \
                                                   m_height_ / 2 * surf->Data.Pitch);
                if (!swap_buf_) {
                    assert(0);
                    return MFX_ERR_NULL_PTR;
                }
            }

            /* copy surface data to local buffer */
            fast_copy(swap_buf_, sY, m_height_ * surf->Data.Pitch);
            fast_copy(swap_buf_ + m_height_ * surf->Data.Pitch, sUV, m_height_ / 2 * surf->Data.Pitch);

            sY = swap_buf_;
            sUV = swap_buf_ + surf->Data.Pitch * m_height_;
            dU = frame_buf_ + m_width_ * m_height_;
            dV = dU + m_width_ * m_height_ / 4;

            for (unsigned int j = 0; j < m_height_ >> 1; j++) {
                fast_copy(dY, sY, m_width_);
                dY += m_width_;
                sY += surf->Data.Pitch;
                fast_copy(dY, sY, m_width_);
                dY += m_width_;
                sY += surf->Data.Pitch;

                for (unsigned int i = 0; i < m_width_ >> 1; i++) {
                    *dU++ = *sUV++;
                    *dV++ = *sUV++;
                }
                sUV += surf->Data.Pitch - m_width_;
            }
        } else {
            MLOG_ERROR("Surface Fourcc not supported!\n");
            assert(0);
        }
    } else {
        MLOG_ERROR("Invalid surface!\n");
        assert(0);
    }

    return MFX_ERR_NONE;
}

mfxStatus SWEncPlugin::CheckParameters(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!par) {
        FRMW_TRACE_ERROR("Invalid input parameter");
        sts = MFX_ERR_NULL_PTR;
    }
    mfxInfoMFX *opt = &(par->mfx);
    m_width_ = opt->FrameInfo.CropW;
    m_height_ = opt->FrameInfo.CropH;

    if (SW_CODEC_ID_H264 == opt->CodecId) {
        enc_opt_.codec_id = AV_CODEC_ID_H264;
    } else if (SW_CODEC_ID_H263 == opt->CodecId) {
        enc_opt_.codec_id = AV_CODEC_ID_H263;
    }

    if (opt->FrameInfo.FrameRateExtN) {
        enc_opt_.time_base_den = opt->FrameInfo.FrameRateExtN;
    }

    if (opt->FrameInfo.FrameRateExtD) {
        enc_opt_.time_base_num = opt->FrameInfo.FrameRateExtD;
    }

    switch (opt->CodecProfile) {
        case MFX_PROFILE_AVC_BASELINE:
            enc_opt_.profile = FF_PROFILE_H264_BASELINE;
            break;
        case MFX_PROFILE_AVC_MAIN:
            enc_opt_.profile = FF_PROFILE_H264_MAIN;
            break;
        case MFX_PROFILE_AVC_HIGH:
            enc_opt_.profile = FF_PROFILE_H264_HIGH;
            break;
        default:
            enc_opt_.profile = FF_PROFILE_H264_MAIN;
            break;
    }

    if (opt->CodecLevel) {
        enc_opt_.level = opt->CodecLevel;
    }

    switch (opt->RateControlMethod) {
        case MFX_RATECONTROL_CBR:
            enc_opt_.rate_ctrl = SW_CBR;
            break;
        case MFX_RATECONTROL_VBR:
            enc_opt_.rate_ctrl = SW_VBR;
            break;
        case MFX_RATECONTROL_CQP:
            enc_opt_.rate_ctrl = SW_CQP;
            break;
        default:
            enc_opt_.rate_ctrl = SW_CBR;
            break;
    }

    enc_opt_.bit_rate = opt->TargetKbps * 1000;

    if (opt->QPI) {
        enc_opt_.qp_min = opt->QPI;
        enc_opt_.qp_max = opt->QPI;
    }

    if (opt->GopPicSize) {
        enc_opt_.gop_size = opt->GopPicSize;
    }
#if 0
    if (opt->GopRefDist) {
    }

    if (opt->IdrInterval) {
    }

    if (opt->TargetUsage) {
    }
#endif
    if (opt->NumSlice) {
        enc_opt_.num_slice = opt->NumSlice;
    }
#if 0
    if (opt->NumRefFrame) {
        enc_opt_.refs = opt->NumRefFrame;
    }

    if (opt->NumThread) {
        enc_opt_.thread_count = opt->NumThread;
    }
#endif
    return sts;
}
#endif

