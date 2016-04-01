/* <COPYRIGHT_TAG> */


#ifdef ENABLE_VPX_CODEC
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "base/fast_copy.h"
#include "general_allocator.h"
#include "vp8_decode_plugin.h"


#define INTERFACE (vpx_codec_vp8_dx())
#define VP8_FOURCC (0x30385056)
#define RAW_FRAME_HDR_SZ (sizeof(uint32_t))
#define IVF_FRAME_HDR_SZ (sizeof(uint32_t) + sizeof(uint64_t))
#define PARSE_TYPE_DATA_LENGTH 32
#define ALIGN16(value) (((value + 15) >> 4) << 4)
#define SURFACE_PITCH_ALIGN 64

unsigned int PlgGetMem32(const void *vmem)
{
    unsigned int  val;
    const unsigned char *mem = (const unsigned char *)vmem;

    val = mem[3] << 24;
    val |= mem[2] << 16;
    val |= mem[1] << 8;
    val |= mem[0];
    return val;
}

unsigned int PlgGetMem16(const void *vmem)
{
    unsigned int  val;
    const unsigned char *mem = (const unsigned char *)vmem;

    val = mem[1] << 8;
    val |= mem[0];
    return val;
}


mfxExtBuffer* VP8DecPlugin::GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
{
    if (!ebuffers) return 0;
    for(mfxU32 i=0; i<nbuffers; i++) {
        if (!ebuffers[i]) continue;
        if (ebuffers[i]->BufferId == BufferId) {
            return ebuffers[i];
        }
    }
    return 0;
}



VP8DecPlugin::VP8DecPlugin()
{
    memset(&m_mfxPluginParam, 0, sizeof(m_mfxPluginParam));
    m_MaxNumTasks = 0;
    m_mfxPluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_mfxPluginParam.MaxThreadNum = 1;
    m_bIsOpaque = false;
    m_pTasks = NULL;
    // Init parameters
    vpx_init_flag_ = false;
}

mfxStatus VP8DecPlugin::Init(mfxVideoParam *param)
{
    mfxStatus sts = MFX_ERR_NONE;
    m_VideoParam = *param;

    // Init vpx decoder
    if (!vpx_init_flag_) {
        sts = InitVpxDec();
        assert(MFX_ERR_NONE == sts);
        vpx_init_flag_ = true;
    }
    if (m_bIsOpaque) {
        m_ExtParam = *m_VideoParam.ExtParam;
        mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer(&m_ExtParam,
                                                                                              m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (pluginOpaqueAlloc == NULL || pluginOpaqueAlloc->Out.Surfaces == NULL) {
            assert(0);
            return MFX_ERR_NULL_PTR;
        } else {
            sts = m_pmfxCore->MapOpaqueSurface(m_pmfxCore->pthis,
                                               pluginOpaqueAlloc->Out.NumSurface, 
                                               pluginOpaqueAlloc->Out.Type,
                                               pluginOpaqueAlloc->Out.Surfaces); 
            assert(MFX_ERR_NONE == sts);
        }
    } 
    m_MaxNumTasks = m_VideoParam.AsyncDepth + 1;
    // Create internal task pool
    m_pTasks = new MFXTask[m_MaxNumTasks];
    memset(m_pTasks, 0, sizeof(MFXTask) * m_MaxNumTasks);
    return MFX_ERR_NONE;
}

mfxStatus VP8DecPlugin::DecodeHeader(mfxBitstream *bs, mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    // Init vpx decoder
    if (!vpx_init_flag_) {
        sts = InitVpxDec();
        assert(MFX_ERR_NONE == sts);
        vpx_init_flag_ = true;
    }

    // Parse input type and info
    sts = ParseInputType(bs);
    assert(MFX_ERR_NONE == sts || MFX_ERR_MORE_DATA == sts);
    m_bIsOpaque = par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY ? true : false;
    par->mfx.FrameInfo.Width = ALIGN16(input_ctx_.width);
    par->mfx.FrameInfo.Height = ALIGN16(input_ctx_.height);
    par->mfx.FrameInfo.CropW = input_ctx_.width;
    par->mfx.FrameInfo.CropH = input_ctx_.height;
    par->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    par->mfx.FrameInfo.FrameRateExtN = input_ctx_.fps_num;
    par->mfx.FrameInfo.FrameRateExtD = input_ctx_.fps_den;
    //par->mfx.FrameInfo.AspectRatioW = 1;
    //par->mfx.FrameInfo.AspectRatioH = 1;
    par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    return sts;
}


mfxStatus VP8DecPlugin::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    out->Info = par->mfx.FrameInfo;
    out->NumFrameMin = 1;
    if (m_bIsOpaque) {
        out->Type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET + MFX_MEMTYPE_OPAQUE_FRAME + MFX_MEMTYPE_FROM_DECODE;
    } else {
        out->Type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET + MFX_MEMTYPE_EXTERNAL_FRAME + MFX_MEMTYPE_FROM_DECODE;
    }
    out->NumFrameSuggested = out->NumFrameMin + par->AsyncDepth;

    return MFX_ERR_NONE;
}

mfxStatus VP8DecPlugin::Close()
{
    mfxStatus sts = MFX_ERR_NONE;

    // destroy vpx decoder object
    vpx_codec_destroy(&vpx_codec_);
    if (m_bIsOpaque) {
        mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer(&m_ExtParam,
                                                                                              m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (pluginOpaqueAlloc != NULL) {
            sts = m_pmfxCore->UnmapOpaqueSurface(m_pmfxCore->pthis,
                                                 pluginOpaqueAlloc->Out.NumSurface,
                                                 pluginOpaqueAlloc->Out.Type,
                                                 pluginOpaqueAlloc->Out.Surfaces);
        }
    }
    if (m_pTasks) {
        delete [] m_pTasks;
    }
    return MFX_ERR_NONE;
}

VP8DecPlugin::~VP8DecPlugin()
{

}

// ---------  Media SDK plug-in interface --------
void VP8DecPlugin::SetAllocPoint(MFXFrameAllocator *pMFXAllocator)
{
    if (pMFXAllocator && m_bIsOpaque) {
        m_pAlloc = pMFXAllocator;
    } else {
        m_pAlloc = &m_pmfxCore->FrameAllocator;
    }
}

mfxStatus VP8DecPlugin::PluginInit(mfxCoreInterface *core)
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pmfxCore = new mfxCoreInterface; 
    if(!m_pmfxCore) {
        return MFX_ERR_MEMORY_ALLOC;
    }
    *m_pmfxCore = *core;    
    m_pAlloc = NULL;
    return sts;
}

mfxStatus VP8DecPlugin::PluginClose()
{
    mfxStatus sts = MFX_ERR_NONE;

    if(m_pmfxCore) {
        delete m_pmfxCore;
        m_pmfxCore = NULL;
    }

    return sts;
}

mfxStatus VP8DecPlugin::GetPluginParam(mfxPluginParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    *par = m_mfxPluginParam;

    return sts;
}

mfxStatus VP8DecPlugin::Submit(const mfxHDL *in, mfxU32 , const mfxHDL *out, mfxU32 , mfxThreadTask *task)  
{
    mfxStatus sts = MFX_ERR_NONE;

    // find free task in task array
    int taskIdx;
    for(taskIdx = 0; taskIdx < m_MaxNumTasks; taskIdx++) {
        if (!m_pTasks[taskIdx].bBusy) {
            break;
        }
    }
    if (taskIdx == m_MaxNumTasks) {
        return MFX_WRN_DEVICE_BUSY; // currently there are no free tasks available
    }
    if (NULL == in) {
        return MFX_ERR_MORE_DATA;
    }

    MFXTask* pTask = &m_pTasks[taskIdx];

    mfxBitstream *bitstream_in = (mfxBitstream *)in;
    mfxFrameSurface1 *opaque_surface_out = (mfxFrameSurface1 *)out[0];
    mfxFrameSurface1 *real_surf_out = NULL;

    if (m_bIsOpaque) {
        sts = m_pmfxCore->GetRealSurface(m_pmfxCore->pthis, opaque_surface_out, &real_surf_out);
        assert(sts == MFX_ERR_NONE);
    } else {
        real_surf_out = opaque_surface_out;
    }

    // Check data fullness
    bool input_eos = (bitstream_in->DataFlag == MFX_BITSTREAM_EOS);
    unsigned char* buffer_start = bitstream_in->Data + bitstream_in->DataOffset;
    unsigned int buffer_length = bitstream_in->DataLength;

    if (buffer_length < IVF_FRAME_HDR_SZ || input_eos) {
        //printf("VP8 : No more data - %p\n", buffer_start);
        return MFX_ERR_MORE_DATA;
    }

    // Frame size pre-check
    size_t new_buf_sz = PlgGetMem32(buffer_start);
    unsigned int hdr_size = (input_ctx_.kind == IVF_FILE) ?
                            IVF_FRAME_HDR_SZ : RAW_FRAME_HDR_SZ;

    /* Frame size limit for vp8 */
    if ((new_buf_sz > 256 * 1024 * 1024) || ((new_buf_sz + hdr_size) > buffer_length)
        || (input_ctx_.kind == RAW_FILE && new_buf_sz > 256 * 1024)) {
        printf("WARNING: Read invalid frame size (%lu), buffer length (%d)\n",
            new_buf_sz, buffer_length);
        return MFX_ERR_MORE_DATA;
    }

    pTask->bitstreamIn  = bitstream_in;
    pTask->surfaceOut   = real_surf_out;
    pTask->bBusy        = true;

    *task = (mfxThreadTask)pTask;

    // To make sure up- or downstream pipeline component does not modify data until we are done.
    m_pmfxCore->IncreaseReference(m_pmfxCore->pthis, &pTask->surfaceOut->Data);

    return sts;
}

mfxStatus VP8DecPlugin::Execute(mfxThreadTask task, mfxU32 , mfxU32 uid_a)
{
    MFXTask* pTask = (MFXTask *)task;
    vpx_codec_iter_t iter = NULL;
    vpx_image_t *img;
    size_t new_buf_sz;
    unsigned char* buffer_start = NULL;
    unsigned int buffer_length = 0;
    unsigned int hdr_size = 0;

    mfxBitstream* bs_in = pTask->bitstreamIn;
    mfxFrameSurface1* surf_out = pTask->surfaceOut;
    m_pAlloc->Lock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
    
    buffer_start = bs_in->Data + bs_in->DataOffset;
    buffer_length = bs_in->DataLength;

    new_buf_sz = PlgGetMem32(buffer_start);
    hdr_size = (input_ctx_.kind == IVF_FILE) ?
                IVF_FRAME_HDR_SZ : RAW_FRAME_HDR_SZ;

    // Frame size limit for vp8 
    if ((new_buf_sz > 256 * 1024 * 1024) || ((new_buf_sz + hdr_size) > buffer_length)
        || (input_ctx_.kind == RAW_FILE && new_buf_sz > 256 * 1024)) {
        printf("Error: Read invalid frame size (%lu), buffer length (%d) - %p\n",
            new_buf_sz, buffer_length, buffer_start);
        m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
        return MFX_ERR_UNKNOWN;
    }

    buffer_start = buffer_start + hdr_size;
    buffer_length = new_buf_sz;
    bs_in->DataOffset += buffer_length + hdr_size;
    bs_in->DataLength -= buffer_length + hdr_size;

    if (vpx_codec_decode(&vpx_codec_, buffer_start, buffer_length, NULL, 0)) {
        printf("Failed to decode frame\n");
        m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
        //ignore decode fail if passed data check
        return MFX_TASK_DONE;
    }

    if ((img = vpx_codec_get_frame(&vpx_codec_, &iter))) {
        /* Upload img to surface */
        UploadToSurface(img, surf_out);
    } else {
        printf("Failed to get decoded frame");
        m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
        return MFX_ERR_UNKNOWN;
    }

    m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
    return MFX_TASK_DONE;
}

mfxStatus VP8DecPlugin::FreeResources(mfxThreadTask task, mfxStatus )
{
    MFXTask *pTask = (MFXTask *)task;

    m_pmfxCore->DecreaseReference(m_pmfxCore->pthis, &pTask->surfaceOut->Data);

    pTask->bBusy = false;

    return MFX_ERR_NONE;
}

mfxStatus VP8DecPlugin::SetAuxParams(void* auxParam, int auxParamSize)  
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}

void VP8DecPlugin::Release()  
{
    ;
}

mfxStatus VP8DecPlugin::InitVpxDec()
{
    mfxStatus sts = MFX_ERR_NONE;
    int flags = 0;
    /* Initialize codec */
    if(vpx_codec_dec_init(&vpx_codec_, INTERFACE, NULL, flags)) {
        printf("Failed to initialize VP8 decoder");
        sts = MFX_ERR_UNKNOWN;
    }

    return sts;
}

bool VP8DecPlugin::IsIvfFile(unsigned char *raw_hdr,
                             unsigned int  *fourcc,
                             unsigned int  *width,
                             unsigned int  *height,
                             unsigned int  *fps_den,
                             unsigned int  *fps_num,
                             unsigned int  *bytes_read)
{
    bool is_ivf = false;

    if (raw_hdr[0] == 'D' && raw_hdr[1] == 'K'
        && raw_hdr[2] == 'I' && raw_hdr[3] == 'F') {
        is_ivf = true;

        if (PlgGetMem16(raw_hdr + 4) != 0) {
            printf("Unrecognized IVF version! This file may not decode properly\n");
        }

        *fourcc = PlgGetMem32(raw_hdr + 8);
        *width = PlgGetMem16(raw_hdr + 12);
        *height = PlgGetMem16(raw_hdr + 14);
        *fps_num = PlgGetMem32(raw_hdr + 16);
        *fps_den = PlgGetMem32(raw_hdr + 20);

        /* Some versions of vpxenc used 1/(2*fps) for the timebase, so
            * we can guess the framerate using only the timebase in this
            * case. Other files would require reading ahead to guess the
            * timebase, like we do for webm.
            */
        if (*fps_num < 1000) {
            /* Correct for the factor of 2 applied to the timebase in the
                  * encoder.
                  */
            if (*fps_num & 1) {
                *fps_den <<= 1;
            } else {
                *fps_num >>= 1;
            }
        } else {
            /* Don't know FPS for sure, and don't have readahead code
                  * (yet?), so just default to 30fps.
                  */
            *fps_num = 30;
            *fps_den = 1;
        }
    }

    *bytes_read = PARSE_TYPE_DATA_LENGTH;
    return is_ivf;
}

bool VP8DecPlugin::IsRawFile(unsigned char *raw_hdr,
                               unsigned int  *fourcc,
                               unsigned int  *width,
                               unsigned int  *height,
                               unsigned int  *fps_den,
                               unsigned int  *fps_num,
                               unsigned int  *bytes_read)
{
    bool is_raw = false;
    vpx_codec_stream_info_t si;
    si.sz = sizeof(si);

    if (PlgGetMem32(raw_hdr) < 256 * 1024 * 1024) {
        if (!vpx_codec_peek_stream_info(INTERFACE, raw_hdr + 4, 32 - 4, &si)) {
          is_raw = true;
          *fourcc = VP8_FOURCC;
          *width = si.w;
          *height = si.h;
          *fps_num = 30;
          *fps_den = 1;
        }
    }

    /* rewind memory read */
    *bytes_read = 0;
    return is_raw;
}

bool VP8DecPlugin::IsWebMFile(unsigned char *raw_hdr,
                                 unsigned int  *fourcc,
                                 unsigned int  *width,
                                 unsigned int  *height,
                                 unsigned int  *fps_den,
                                 unsigned int  *fps_num,
                                 unsigned int  *bytes_read)
{
    bool is_webm = false;
    *bytes_read = 0;
    return is_webm;
}


mfxStatus VP8DecPlugin::ParseInputType(mfxBitstream *bs)
{
    unsigned char* buffer_start = NULL;
    unsigned int buffer_length = 0;
    unsigned int bytes_read = 0;
    mfxStatus sts = MFX_ERR_NONE;

    buffer_start = bs->Data + bs->DataOffset;
    buffer_length = bs->DataLength;

    if (buffer_length < PARSE_TYPE_DATA_LENGTH) {
        printf("VP8: header data is not enough\n");
        sts = MFX_ERR_MORE_DATA;
        return sts;
    }

    if (IsIvfFile(buffer_start, &input_ctx_.fourcc, &input_ctx_.width, &input_ctx_.height,
           &input_ctx_.fps_den, &input_ctx_.fps_num, &bytes_read)) {
        input_ctx_.kind = IVF_FILE;
    } else if (IsRawFile(buffer_start, &input_ctx_.fourcc, &input_ctx_.width, &input_ctx_.height,
           &input_ctx_.fps_den, &input_ctx_.fps_num, &bytes_read)) {
        input_ctx_.kind = RAW_FILE;
    } else if (IsWebMFile(buffer_start, &input_ctx_.fourcc, &input_ctx_.width, &input_ctx_.height,
           &input_ctx_.fps_den, &input_ctx_.fps_num, &bytes_read)) {
        input_ctx_.kind = WEBM_FILE;
    } else {
        printf("VP8: unrecognized file format, treat it as raw data\n");
        bytes_read = PlgGetMem32(buffer_start) + RAW_FRAME_HDR_SZ;
        sts = MFX_ERR_MORE_DATA;
    }

    if (bytes_read) {
        /* Update read frame ptr */
        bs->DataOffset += bytes_read;
        bs->DataLength -= bytes_read;
    }

    return sts;
}

void VP8DecPlugin::UploadToSurface(vpx_image_t *yuv_image, mfxFrameSurface1 *surf_out)
{
    unsigned char *y_src, *u_src, *v_src;
    unsigned char *y_dst, *u_dst;
    int row, col;

    y_src = yuv_image->planes[VPX_PLANE_Y];
    u_src = yuv_image->planes[VPX_PLANE_U];
    v_src = yuv_image->planes[VPX_PLANE_V];
    y_dst = surf_out->Data.Y;
    u_dst = surf_out->Data.VU;

    /* Y plane */
    for (row = 0; row < yuv_image->d_h; row++) {
        fast_copy(y_dst, y_src, yuv_image->d_w);
        y_dst += surf_out->Data.Pitch;
        y_src += yuv_image->stride[VPX_PLANE_Y];
    }

    if (surf_out->Info.FourCC == MFX_FOURCC_NV12) { /* UV plane */
        for (row = 0; row < (yuv_image->d_h / 2); row++) {
            for (col = 0; col < (yuv_image->d_w / 2); col++) {
                u_dst[col * 2] = u_src[col];
                u_dst[col * 2 + 1] = v_src[col];
            }

            u_dst += surf_out->Data.Pitch; //To Check
            u_src += yuv_image->stride[VPX_PLANE_U];
            v_src += yuv_image->stride[VPX_PLANE_V];
        }
    } else {
        printf("Surface Fourcc not supported!\n");
    }

    surf_out->Info.CropH = yuv_image->d_h;
    surf_out->Info.CropW = yuv_image->d_w;
    surf_out->Info.Height = ALIGN16(yuv_image->d_h);
    surf_out->Info.Width = ALIGN16(yuv_image->d_w);
    surf_out->Data.Pitch = (yuv_image->d_w / SURFACE_PITCH_ALIGN + 1) * SURFACE_PITCH_ALIGN;

    return;
}
#endif


