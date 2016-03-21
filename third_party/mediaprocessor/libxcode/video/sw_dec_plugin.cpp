/* <COPYRIGHT_TAG> */

#include "sw_dec_plugin.h"

#ifdef ENABLE_SW_CODEC
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "base/fast_copy.h"
#include "base/trace.h"

#define ALIGN16(value) (((value + 15) >> 4) << 4)

DEFINE_MLOGINSTANCE_CLASS(SWDecPlugin, "SWDecPlugin");

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    unsigned char* buffer_start = NULL;
    unsigned int buffer_length = 0;

    if (!opaque) {
        FRMW_TRACE_ERROR("Bit stream parameter is NULL");
        return -1;
    }

    mfxBitstream *bs = (mfxBitstream *)opaque;
    buffer_start = bs->Data + bs->DataOffset;
    buffer_length = bs->DataLength;

    buf_size = FFMIN(buf_size, buffer_length);
    FRMW_TRACE_DEBUG("ptr:%p length:%zu size:%d\n", buffer_start, buffer_length, buf_size);

    /* copy internal buffer data to buf */
    memcpy(buf, buffer_start, buf_size);
    bs->DataOffset += buf_size;
    bs->DataLength -= buf_size;

    return buf_size;
}

SWDecPlugin::SWDecPlugin()
{
    memset(&m_mfxPluginParam_, 0, sizeof(m_mfxPluginParam_));
    m_mfxPluginParam_.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_mfxPluginParam_.MaxThreadNum = 1;
    m_MaxNumTasks_ = 0;
    no_data_ = false;
    m_bIsOpaque = false;

    init_flag_ = false;
    av_fmt_ctx_ = NULL;
    avio_ctx_buffer_ = NULL;
    av_io_ctx_ = NULL;
    av_stream_ = NULL;
    av_codec_ctx_ = NULL;
    av_codec_ = NULL;
    av_frame_ = NULL;

    m_dec_num_ = 0;
}

mfxStatus SWDecPlugin::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    if (out && par) {
        out->Info = par->mfx.FrameInfo;
        out->NumFrameMin = 1;
        if (m_bIsOpaque) {
            out->Type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET + MFX_MEMTYPE_OPAQUE_FRAME + MFX_MEMTYPE_FROM_DECODE;
        } else {
            out->Type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET + MFX_MEMTYPE_EXTERNAL_FRAME + MFX_MEMTYPE_FROM_DECODE;
        }
        out->NumFrameSuggested = out->NumFrameMin + par->AsyncDepth;
    } else {
        FRMW_TRACE_ERROR("Alloc request parameter is NULL");
        return MFX_ERR_NULL_PTR;
    }
    return MFX_ERR_NONE;
}

mfxStatus SWDecPlugin::Init(mfxVideoParam *par)
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

    if (m_bIsOpaque) {
    // Should be opaque case.
    if ((m_VideoParam_->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) == 0) {
        FRMW_TRACE_ERROR("IOPattern is 0x%x", m_VideoParam_->IOPattern);
        return MFX_ERR_UNKNOWN;
    }

   // m_MaxNumTasks_ = m_VideoParam_->AsyncDepth + 1;
    FRMW_TRACE_DEBUG("Max num task is %d", m_MaxNumTasks_);

    while (!pluginOpaqueAlloc && (i < m_VideoParam_->NumExtParam)) {
        pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(m_VideoParam_->ExtParam,\
                                                                     m_VideoParam_->NumExtParam,\
                                                                     MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        i++;
    }
    if (pluginOpaqueAlloc != NULL) {
        if (pluginOpaqueAlloc->Out.Surfaces == NULL) {
            assert(0);
            return MFX_ERR_NULL_PTR;
        } else {
            sts = m_pmfxCore_->MapOpaqueSurface(m_pmfxCore_->pthis,
            pluginOpaqueAlloc->Out.NumSurface,
            pluginOpaqueAlloc->Out.Type,
            pluginOpaqueAlloc->Out.Surfaces);
            assert(MFX_ERR_NONE == sts);
        }
    } else {
        FRMW_TRACE_ERROR("Opaque alloc is NULL");
        return MFX_ERR_NULL_PTR;
    }
 
    }
    m_MaxNumTasks_ = m_VideoParam_->AsyncDepth + 1;
    // Create internal task pool
    m_pTasks_ = new MFXTask_SW_DEC[m_MaxNumTasks_];
    memset(m_pTasks_, 0, sizeof(MFXTask_SW_DEC) * m_MaxNumTasks_);

    return MFX_ERR_NONE;
}

mfxStatus SWDecPlugin::DecodeHeader(mfxBitstream *bs, mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!init_flag_) {
        sts = ParserStreamInfo(bs, par->mfx.CodecId);
        if (MFX_ERR_NONE == sts) {
            sts = InitSWDec();
            if (MFX_ERR_NONE == sts) {
                init_flag_ = true;
            } else {
                return sts;
            }
        } else {
            return sts;
        }
    }

    switch(av_stream_->codec->codec_id) {
        case AV_CODEC_ID_H264:
            par->mfx.CodecId = MFX_CODEC_AVC;
            break;
        case AV_CODEC_ID_MPEG2VIDEO:
            par->mfx.CodecId = MFX_CODEC_MPEG2;
            break;
        case AV_CODEC_ID_MJPEG:
            par->mfx.CodecId = MFX_CODEC_JPEG;
            break;
        default:
            break;
    }

    m_bIsOpaque = par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY ? true : false;
    par->mfx.FrameInfo.Width = ALIGN16(av_stream_->codec->width);
    par->mfx.FrameInfo.Height = ALIGN16(av_stream_->codec->height);
    par->mfx.FrameInfo.CropW = av_stream_->codec->width;
    par->mfx.FrameInfo.CropH = av_stream_->codec->height;
    par->mfx.FrameInfo.FrameRateExtN = av_stream_->r_frame_rate.num;
    par->mfx.FrameInfo.FrameRateExtD = av_stream_->r_frame_rate.den;
    par->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    FRMW_TRACE_INFO("Width:%d, Height:%d, frame rate:%d/%d", \
                    av_stream_->codec->width, av_stream_->codec->height, \
                    av_stream_->r_frame_rate.num, av_stream_->r_frame_rate.den);

    return sts;
}

mfxStatus SWDecPlugin::Close()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_bIsOpaque) {
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
                                             pluginOpaqueAlloc->Out.NumSurface,
                                             pluginOpaqueAlloc->Out.Type,
                                             pluginOpaqueAlloc->Out.Surfaces);
#if 1
        if (m_pTasks_) {
            delete [] m_pTasks_;
            m_pTasks_ = NULL;
        }
#endif
        if (MFX_ERR_NONE != sts) {
            FRMW_TRACE_ERROR("Failed to umap opaque mem");
        }
        return MFX_ERR_NONE;
    } else {
        FRMW_TRACE_WARNI("SW Dec PTR is NULL");
        return MFX_ERR_NULL_PTR;
    }
    }

    if (m_pTasks_) {
        delete [] m_pTasks_;
        m_pTasks_ = NULL;
    }
    return MFX_ERR_NONE;
}

SWDecPlugin::~SWDecPlugin()
{
    if (av_frame_) {
        av_frame_free(&av_frame_);
        av_frame_ = NULL;
    }

    if (av_fmt_ctx_) {
        avformat_close_input(&av_fmt_ctx_);
        av_fmt_ctx_ = NULL;
    }

    if(av_io_ctx_) {
        av_freep(&av_io_ctx_->buffer);
        av_freep(&av_io_ctx_);
        av_io_ctx_ = NULL;
    }
}

void SWDecPlugin::SetAllocPoint(MFXFrameAllocator *pMFXAllocator)
{
    if (pMFXAllocator && m_bIsOpaque) {
        m_pAlloc_ = pMFXAllocator;
    } else {
        m_pAlloc_ = &m_pmfxCore_->FrameAllocator;
    }
}

mfxExtBuffer *SWDecPlugin::GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
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

mfxStatus SWDecPlugin::PluginInit(mfxCoreInterface *core)
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pmfxCore_ = new mfxCoreInterface;
    if(!m_pmfxCore_) {
        return MFX_ERR_MEMORY_ALLOC;
    }
    *m_pmfxCore_ = *core;
    //m_pAlloc_ = &m_pmfxCore_->FrameAllocator;
    m_pAlloc_ = NULL;

    return sts;
}

mfxStatus SWDecPlugin::PluginClose()
{
    mfxStatus sts = MFX_ERR_NONE;

    if(m_pmfxCore_) {
        delete m_pmfxCore_;
        m_pmfxCore_ = NULL;
    }

    return sts;
}

mfxStatus SWDecPlugin::GetPluginParam(mfxPluginParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    *par = m_mfxPluginParam_;

    return sts;
}

mfxStatus SWDecPlugin::Submit(const mfxHDL *in, mfxU32 , const mfxHDL *out, mfxU32 , mfxThreadTask *task)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxBitstream *bitstream_in = NULL;
    mfxFrameSurface1 *opaque_surface_out = NULL;
    mfxFrameSurface1 *real_surf_out = NULL;
    bool input_eos = false;

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
    MFXTask_SW_DEC *pTask = &m_pTasks_[taskIdx];

    bitstream_in = (mfxBitstream *)in;
    opaque_surface_out = (mfxFrameSurface1 *)out[0];
    if (!bitstream_in || no_data_) {
        return MFX_ERR_MORE_DATA;
    } else if (!opaque_surface_out) {
        return MFX_ERR_MORE_SURFACE;
    }
    if (bitstream_in && (bitstream_in->DataLength == 0)) {
        return MFX_ERR_MORE_DATA;
    }

    if (m_bIsOpaque) {
    sts = m_pmfxCore_->GetRealSurface(m_pmfxCore_->pthis, opaque_surface_out, &real_surf_out);
    if(sts != MFX_ERR_NONE) {
        assert(0);
    }

    } else {
        real_surf_out = opaque_surface_out;
    }

    // Check data fullness
    input_eos = (bitstream_in->DataFlag == MFX_BITSTREAM_EOS);
    if (input_eos) {
        FRMW_TRACE_ERROR("No more data...\n");
        return MFX_ERR_MORE_DATA;
    }

    pTask->bitstreamIn = bitstream_in;
    pTask->surfaceOut= real_surf_out;
    pTask->bBusy        = true;

    *task = (mfxThreadTask)pTask;
    m_pmfxCore_->IncreaseReference(m_pmfxCore_->pthis, &pTask->surfaceOut->Data);

    return sts;
}

mfxStatus SWDecPlugin::Execute(mfxThreadTask task, mfxU32 , mfxU32 uid_a)
{
    MFXTask_SW_DEC *pTask = (MFXTask_SW_DEC *)task;
    int ret = 0;
    //unsigned char* buffer_start = NULL;
    //unsigned int buffer_length = 0;
    int got_picture_ptr = 0;
    //int count = 0;

    mfxBitstream* bs_in = pTask->bitstreamIn;
    mfxFrameSurface1 *surf_out = pTask->surfaceOut;

    if (!bs_in->DataLength) {
       //no_data_ = true;
       //return MFX_ERR_MORE_DATA;
    }
    ret = av_read_frame(av_fmt_ctx_, &avpkt_);
    if (ret < 0) {
        FRMW_TRACE_ERROR("Error while reading frame %d\n", m_dec_num_);
        fprintf(stderr, "Error while reading frame %d\n", m_dec_num_);
        if (!bs_in->DataLength) {
            fprintf(stderr,"------read frame need more data\n");
            no_data_ = true;
            return MFX_ERR_MORE_DATA;
        } else {
            no_data_ = false;
            fprintf(stderr,"------read frame need unknow\n");
            return MFX_ERR_UNKNOWN;
        }
    } else {
        no_data_ = false;
    }

    while (0 == ret) {
        ret = avcodec_decode_video2(av_codec_ctx_, av_frame_, &got_picture_ptr, &avpkt_);
        if (ret < 0) {
            FRMW_TRACE_ERROR("Error while decoding frame %d\n", m_dec_num_);
            av_free_packet(&avpkt_);
            return MFX_ERR_UNKNOWN;
        }
    }

    if (got_picture_ptr) {
        m_dec_num_++;
        FRMW_TRACE_DEBUG("Saving the decoder frame num:%d\n", m_dec_num_);
        m_pAlloc_->Lock(m_pAlloc_->pthis, surf_out->Data.MemId, &surf_out->Data);
        UploadToSurface(av_frame_, surf_out);
        m_pAlloc_->Unlock(m_pAlloc_->pthis, surf_out->Data.MemId, &surf_out->Data);
       // break;
    }

    av_free_packet(&avpkt_);
    return MFX_TASK_DONE;
}

mfxStatus SWDecPlugin::FreeResources(mfxThreadTask task, mfxStatus )
{
    MFXTask_SW_DEC *pTask = (MFXTask_SW_DEC *)task;

    //m_pmfxCore_->DecreaseReference(m_pmfxCore_->pthis, &pTask->surfaceIn->Data);
    m_pmfxCore_->DecreaseReference(m_pmfxCore_->pthis, &pTask->surfaceOut->Data);

    pTask->bBusy = false;

    return MFX_ERR_NONE;
}

mfxStatus SWDecPlugin::SetAuxParams(void *auxParam, int auxParamSize)
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}

void SWDecPlugin::Release()
{
    return;
}

mfxStatus SWDecPlugin::ParserStreamInfo(mfxBitstream *bs, unsigned int codec_id)
{
    int ret = -1;
    unsigned char* buffer_start = NULL;
    unsigned int buffer_length = 0;
    AVInputFormat* ifmt = NULL;

    if (!bs) {
        FRMW_TRACE_ERROR("Bit stream parameter is NULL");
        return MFX_ERR_NULL_PTR;
    }

    buffer_start = bs->Data + bs->DataOffset;
    buffer_length = bs->DataLength;
    if (buffer_length == 0) {
        FRMW_TRACE_ERROR("Bit stream data is not enough\n");
        return MFX_ERR_MORE_DATA;
    }

    av_register_all();
    avcodec_register_all();
    switch(codec_id)
    {
        case MFX_CODEC_JPEG:
            ifmt = av_find_input_format("mjpeg");
            break;
        default:
            break;
    
    }

    av_fmt_ctx_ = avformat_alloc_context();
    if (!av_fmt_ctx_) {
        FRMW_TRACE_ERROR("AVFormat alloc context is NULL");
        return MFX_ERR_NULL_PTR;
    }

    avio_ctx_buffer_ = (unsigned char *)av_malloc(bs->MaxLength);
    if (!avio_ctx_buffer_) {
        FRMW_TRACE_ERROR("AVIO alloc buffer is NULL");
        return MFX_ERR_NULL_PTR;
    }

    av_io_ctx_ = avio_alloc_context(avio_ctx_buffer_, bs->MaxLength, 0, bs, &read_packet, NULL, NULL);
    if (!av_io_ctx_) {
        FRMW_TRACE_ERROR("AVIO alloc context is NULL");
        return MFX_ERR_NULL_PTR;
    }

    av_fmt_ctx_->pb = av_io_ctx_;
    ret = avformat_open_input(&av_fmt_ctx_, "", ifmt, NULL);
    if (ret < 0) {
        FRMW_TRACE_ERROR("AVFormat open input fail");
        return MFX_ERR_UNKNOWN;
    }

    ret = avformat_find_stream_info(av_fmt_ctx_, NULL);
    if (ret < 0) {
        FRMW_TRACE_ERROR("AVFormat find stream info fail");
        return MFX_ERR_UNKNOWN;
    }

    ret = av_find_best_stream(av_fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        FRMW_TRACE_ERROR("Find best stream fail");
        return MFX_ERR_UNKNOWN;
    }
    av_stream_ = av_fmt_ctx_->streams[ret];

    av_dump_format(av_fmt_ctx_, 0, NULL, 0);
    return MFX_ERR_NONE;

}

mfxStatus SWDecPlugin::InitSWDec()
{
    if (!av_stream_) {
        FRMW_TRACE_ERROR("No stream info can be used");
        return MFX_ERR_NULL_PTR;
    }

    if (AVMEDIA_TYPE_VIDEO == av_stream_->codec->codec_type) {
        av_codec_ctx_ = av_stream_->codec;
    } else {
        FRMW_TRACE_ERROR("No video stream info can be used");
        return MFX_ERR_UNKNOWN;
    }

    /* find it */
    av_codec_ = avcodec_find_decoder(av_codec_ctx_->codec_id);
    if(!av_codec_)
    {
        FRMW_TRACE_ERROR("H264 codec not found\n");
        return MFX_ERR_UNKNOWN;
    }

    /* open it */
    if (avcodec_open2(av_codec_ctx_, av_codec_, NULL) < 0) {
        FRMW_TRACE_ERROR("Can not open codec\n");
        return MFX_ERR_UNKNOWN;
    }

    av_frame_ = av_frame_alloc();
    if (!av_frame_) {
        FRMW_TRACE_ERROR("AVFrame alloc fail\n");
        return MFX_ERR_NULL_PTR;
    }

    av_init_packet(&avpkt_);
    avpkt_.data = NULL;
    avpkt_.size = 0;

    return MFX_ERR_NONE;
}

void SWDecPlugin::UploadToSurface(AVFrame *av_frame, mfxFrameSurface1 *surf_out)
{
    unsigned char *y_src, *u_src, *v_src;
    unsigned char *y_dst, *u_dst;
    int row, col;

    y_src = av_frame->data[0];
    u_src = av_frame->data[1];
    v_src = av_frame->data[2];
    y_dst = surf_out->Data.Y;
    u_dst = surf_out->Data.VU;

    /* Y plane */
    for (row = 0; row < av_frame->height; row++) {
        fast_copy(y_dst, y_src, av_frame->width);
        y_dst += surf_out->Data.Pitch;
        y_src += av_frame->linesize[0];
    }

    if (surf_out->Info.FourCC == MFX_FOURCC_NV12) { /* UV plane */
        for (row = 0; row < av_frame->height / 2; row++) {
            for (col = 0; col < av_frame->width / 2; col++) {
                u_dst[col * 2] = u_src[col];
                u_dst[col * 2 + 1] = v_src[col];
            }

            u_dst += surf_out->Data.Pitch; //To Check
            u_src += av_frame->linesize[1];
            v_src += av_frame->linesize[2];
        }
    } else {
        FRMW_TRACE_ERROR("Surface Fourcc not supported!\n");
    }

    return;
}
#endif

