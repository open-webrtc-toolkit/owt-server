/* <COPYRIGHT_TAG> */

#include "vp8_encode_plugin.h"

#ifdef ENABLE_VPX_CODEC
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "base/fast_copy.h"
#include "general_allocator.h"


#define INTERFACE (vpx_codec_vp8_cx())
#define FOURCC    0x30385056
#define RAW_HDR_SIZE    4
#define DEFAULT_GOP_SIZE 30
#define MIN_Q 4
#define MAX_Q 63

#ifndef VA_FOURCC_I420
#define VA_FOURCC_I420 0x30323449
#endif

DEFINE_MLOGINSTANCE_CLASS(VP8EncPlugin, "VP8EncPlugin");
enum VP8FILETYPE {
    RAW_FILE,
    IVF_FILE,
    WEBM_FILE
};

pthread_mutex_t VP8EncPlugin::plugin_mutex_ = PTHREAD_MUTEX_INITIALIZER;
static bool plugin_mutex_init = VP8EncPlugin::MutexInit();

void PlgPutMem16(char *mem, unsigned int val) {
    mem[0] = val;
    mem[1] = val >> 8;
}

void PlgPutMem32(char *mem, unsigned int val) {
    mem[0] = val;
    mem[1] = val >> 8;
    mem[2] = val >> 16;
    mem[3] = val >> 24;
}

mfxExtBuffer* VP8EncPlugin::GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
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

VP8EncPlugin::VP8EncPlugin()
{
    memset(&m_mfxPluginParam, 0, sizeof(m_mfxPluginParam));
    m_MaxNumTasks = 0;
    m_mfxPluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_mfxPluginParam.MaxThreadNum = 1;
    // Init parameters
    output_ivf_ = true;
    vpx_init_flag_ = false;
    frame_cnt_ = 0;
    swap_buf_ = NULL;
    frame_buf_ = NULL;
    m_pAlloc = NULL;
    m_bIsOpaque = false;
    m_pTasks = NULL;
    vp8_force_key_frame = false;
    cfg = new vpx_codec_enc_cfg_t();
}

void VP8EncPlugin::SetAllocPoint(MFXFrameAllocator *pMFXAllocator)
{
    if(pMFXAllocator && m_bIsOpaque) {
        m_pAlloc = pMFXAllocator;
    } else {
        m_pAlloc = &m_pmfxCore->FrameAllocator;
    }
}

void VP8EncPlugin::VP8ForceKeyFrame()
{
    vp8_force_key_frame = true;
}

void VP8EncPlugin::VP8ResetBitrate(unsigned int bitrate)
{
    cfg->rc_target_bitrate = bitrate;

    //this api may be called before codec's initialized
    if (!vpx_init_flag_) return;

    /* Reinit codec config */
    if(vpx_codec_enc_config_set(&vpx_codec_, cfg)) {
        MLOG_ERROR("VP8 Failed to change bitrate\n");
    }
}

int VP8EncPlugin::VP8ResetRes(unsigned int width, unsigned int height)
{
    cfg->g_w = width;
    cfg->g_h = height;
    m_width_ = width;
    m_height_ = height;

    if(vpx_codec_enc_config_set(&vpx_codec_, cfg)) {
        MLOG_ERROR("VP8 Failed to change res\n");
        return -1;
    }

    /* re-alloc image */
    vpx_img_free(&raw_);
    if (!vpx_img_alloc(&raw_, VPX_IMG_FMT_I420, m_width_, m_height_, 1)) {
        MLOG_ERROR("VP8: Failed to allocate image\n");
        return -2;
    }
    frame_buf_ = (&raw_)->planes[0];

    return 0;
}

mfxStatus VP8EncPlugin::Init(mfxVideoParam *param)
{
    mfxStatus sts = MFX_ERR_NONE;
    m_VideoParam = *param;

    m_bIsOpaque = param->IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY ? true : false;

    if (m_bIsOpaque) {
        m_ExtParam = *m_VideoParam.ExtParam;
        mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer(&m_ExtParam,
                                                                                              m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (pluginOpaqueAlloc == NULL || pluginOpaqueAlloc->In.Surfaces == NULL) {
            assert(0);
            return MFX_ERR_NULL_PTR;
        } else {
            sts = m_pmfxCore->MapOpaqueSurface(m_pmfxCore->pthis,
                                               pluginOpaqueAlloc->In.NumSurface,
                                               pluginOpaqueAlloc->In.Type,
                                               pluginOpaqueAlloc->In.Surfaces);
            assert(MFX_ERR_NONE == sts);
        }
    }
    m_MaxNumTasks = m_VideoParam.AsyncDepth + 1;
    // Create internal task pool
    m_pTasks = new MFXTask1[m_MaxNumTasks];
    memset(m_pTasks, 0, sizeof(MFXTask1) * m_MaxNumTasks);

    /* check input parameter limits */
    if (!CheckParameters(param)) {
        MLOG_ERROR("Input parameters have error.\n");
        return MFX_ERR_UNKNOWN;
    }
    
    return MFX_ERR_NONE;
}

mfxStatus VP8EncPlugin::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    par->mfx.BufferSizeInKB = 1024 * 2;

    return sts;
}


mfxStatus VP8EncPlugin::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    in->Info = par->mfx.FrameInfo;
    in->NumFrameMin = 1;
    if (m_bIsOpaque) {
        in->Type = MFX_MEMTYPE_OPAQUE_FRAME + MFX_MEMTYPE_FROM_ENCODE;
    } else {
        in->Type = MFX_MEMTYPE_EXTERNAL_FRAME + MFX_MEMTYPE_FROM_ENCODE;
    }
    in->NumFrameSuggested = in->NumFrameMin + par->AsyncDepth;

    return MFX_ERR_NONE;
}

mfxStatus VP8EncPlugin::Close()
{
    mfxStatus sts = MFX_ERR_NONE;
    if (pthread_mutex_lock(&VP8EncPlugin::plugin_mutex_)) {
        MLOG_ERROR("Failed to get lock\n");
        assert(0);
    }
    // destroy vpx decoder object
    if (vpx_init_flag_) {
        if (swap_buf_) {
            free(swap_buf_);
            swap_buf_ = NULL;
        }
        vpx_img_free(&raw_);
        vpx_codec_destroy(&vpx_codec_);
    }
    if (pthread_mutex_unlock(&VP8EncPlugin::plugin_mutex_)) {
        MLOG_ERROR("Failed to release lock\n");
        assert(0);
    }
    if (m_bIsOpaque) {
        mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer(&m_ExtParam,
                                                                                              m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (pluginOpaqueAlloc != NULL) {
            sts = m_pmfxCore->UnmapOpaqueSurface(m_pmfxCore->pthis,
                                                 pluginOpaqueAlloc->In.NumSurface,
                                                 pluginOpaqueAlloc->In.Type,
                                                 pluginOpaqueAlloc->In.Surfaces);
            assert(MFX_ERR_NONE == sts);
        }
    }
    if (m_pTasks) {
        delete [] m_pTasks;
    }
    return sts;
}

VP8EncPlugin::~VP8EncPlugin()
{
    if (cfg != NULL) {
        delete cfg;
        cfg = NULL;
    }
}

// ---------  Media SDK plug-in interface --------

mfxStatus VP8EncPlugin::PluginInit(mfxCoreInterface *core)
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pmfxCore = new mfxCoreInterface; 
    if(!m_pmfxCore) {
        return MFX_ERR_MEMORY_ALLOC;
    }
    *m_pmfxCore = *core;    

    return sts;
}

mfxStatus VP8EncPlugin::PluginClose()
{
    mfxStatus sts = MFX_ERR_NONE;

    if(m_pmfxCore) {
        delete m_pmfxCore;
        m_pmfxCore = NULL;
    }

    return sts;
}

mfxStatus VP8EncPlugin::GetPluginParam(mfxPluginParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    *par = m_mfxPluginParam;

    return sts;
}

mfxStatus VP8EncPlugin::Submit(const mfxHDL *in, mfxU32 , const mfxHDL *out, mfxU32 , mfxThreadTask *task)  
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

    MFXTask1* pTask = &m_pTasks[taskIdx];

    mfxBitstream *bitstream_out = (mfxBitstream *)out;
    mfxFrameSurface1 *opaque_surface_in = (mfxFrameSurface1 *)in[0];
    mfxFrameSurface1 *real_surf_in = NULL;

    if (NULL == opaque_surface_in) {
        return MFX_ERR_MORE_SURFACE;
    }
    if (m_bIsOpaque) {
        sts = m_pmfxCore->GetRealSurface(m_pmfxCore->pthis, opaque_surface_in, &real_surf_in);
        if(sts != MFX_ERR_NONE) {
            assert(0);
        }
    } else {
        real_surf_in = opaque_surface_in;
    }

    // Init vpx decoder
    m_BS = bitstream_out;
    if (!vpx_init_flag_) {
        sts = InitVpxEnc();
        assert(MFX_ERR_NONE == sts);
        vpx_init_flag_ = true;
    }

    pTask->bitstreamOut = bitstream_out;
    pTask->surfaceIn = real_surf_in;
    pTask->bBusy        = true;

    *task = (mfxThreadTask)pTask;

    // To make sure up- or downstream pipeline component does not modify data until we are done.
    m_pmfxCore->IncreaseReference(m_pmfxCore->pthis, &pTask->surfaceIn->Data);

    return sts;
}

mfxStatus VP8EncPlugin::Execute(mfxThreadTask task, mfxU32 , mfxU32 uid_a)
{
    MFXTask1* pTask = (MFXTask1 *)task;
    mfxStatus sts = MFX_ERR_NONE;
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t *pkt;
    int flags = 0;

    if (pthread_mutex_lock(&VP8EncPlugin::plugin_mutex_)) {
        MLOG_ERROR("Failed to get lock\n");
        assert(0);
    }

    mfxFrameSurface1* surf_in = pTask->surfaceIn;
    sts = m_pAlloc->Lock(m_pAlloc->pthis, surf_in->Data.MemId, &surf_in->Data);
    if (sts != MFX_ERR_NONE) {
        MLOG_ERROR("Lock return %d\n", sts);
        assert(sts == MFX_ERR_NONE);
    }

    /* get input frame */
    sts = GetInputYUV(surf_in);
    if (MFX_ERR_NONE != sts) {
        MLOG_ERROR("Get yuv failed\n");
        assert(0);
    }

    sts = m_pAlloc->Unlock(m_pAlloc->pthis, surf_in->Data.MemId, &surf_in->Data);
    if (sts != MFX_ERR_NONE) {
        MLOG_ERROR("Unlock return %d\n", sts);
        assert(sts == MFX_ERR_NONE);
    }

    /* encode frame */
    if (!(frame_cnt_ % keyframe_dist_)) {
        flags = VPX_EFLAG_FORCE_KF;
    }

    if (vp8_force_key_frame) {
        flags = VPX_EFLAG_FORCE_KF;
        vp8_force_key_frame = false;
    }

    if(vpx_codec_encode(&vpx_codec_, &raw_, frame_cnt_, 1, flags, VPX_DL_REALTIME)) {
        MLOG_ERROR("Failed to encode frame\n");
        assert(0);
    }

    if (pthread_mutex_unlock(&VP8EncPlugin::plugin_mutex_)) {
        MLOG_ERROR("Failed to release lock\n");
        assert(0);
    }

    /* get encoded data  */
    while( (pkt = vpx_codec_get_cx_data(&vpx_codec_, &iter)) ) {
        switch(pkt->kind) {
        case VPX_CODEC_CX_FRAME_PKT:
            /* write frame header for ivf */
            if (output_ivf_) {
                WriteIvfFrameHeader(pkt);
            }

            /* output raw header */
            if (!output_ivf_ && is_raw_hdr_) {
                memcpy(m_BS->Data + m_BS->DataLength, (void*)&pkt->data.frame.sz, RAW_HDR_SIZE);
                m_BS->DataLength += RAW_HDR_SIZE;
            }

            /* output encoded frame */
            memcpy(m_BS->Data + m_BS->DataLength, pkt->data.frame.buf, pkt->data.frame.sz);
            m_BS->DataLength += pkt->data.frame.sz;

            break;
        default:
            break;
        }
    }

    frame_cnt_ ++;
    return MFX_TASK_DONE;
}

mfxStatus VP8EncPlugin::FreeResources(mfxThreadTask task, mfxStatus )
{
    MFXTask1 *pTask = (MFXTask1 *)task;

    m_pmfxCore->DecreaseReference(m_pmfxCore->pthis, &pTask->surfaceIn->Data);

    pTask->bBusy = false;

    return MFX_ERR_NONE;
}

mfxStatus VP8EncPlugin::SetAuxParams(void* auxParam, int auxParamSize)  
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}

void VP8EncPlugin::Release()  
{
    ;
}

mfxStatus VP8EncPlugin::InitVpxEnc()
{
    vpx_codec_err_t res;
    long width;
    long height;

    width = m_width_;
    height = m_height_;

    /* check resolution parameters */
    if (width < 16 || width % 2 || height < 16 || height %2) {
        MLOG_ERROR("Invalid resolutuion: %ldx%ld\n", width, height);
        return MFX_ERR_UNKNOWN;
    }

    /* alloc image */
    if (!vpx_img_alloc(&raw_, VPX_IMG_FMT_I420, width, height, 1)) {
        MLOG_ERROR("VP8: Failed to allocate image\n");
        return MFX_ERR_UNKNOWN;
    }

    MLOG_INFO("VP8 encode using %s\n", vpx_codec_iface_name(INTERFACE));

    /* populate encoder configuration */
    res = vpx_codec_enc_config_default(INTERFACE, cfg, 0);
    if (res) {
        MLOG_ERROR("VP8 Failed to get config: %s\n", vpx_codec_err_to_string(res));
        goto EXIT;
    }

    /* update default configuration */
    if (encoder_options_.threads > 0) {
        cfg->g_threads = encoder_options_.threads;
    }
    if (encoder_options_.profile <= 3) {
        cfg->g_profile = encoder_options_.profile;
    }
    if (encoder_options_.bitrate > 0) {
        cfg->rc_target_bitrate = encoder_options_.bitrate;
    }
    if (encoder_options_.bitratectrl <= VPX_CQ) {
        cfg->rc_end_usage = (vpx_rc_mode)encoder_options_.bitratectrl;
    }
    if (encoder_options_.time_base_num > 0) {
        cfg->g_timebase.num = encoder_options_.time_base_num;
    }
    if (encoder_options_.time_base_den > 0) {
        cfg->g_timebase.den = encoder_options_.time_base_den;
    }
    if (encoder_options_.min_quantizer < 64) {
        cfg->rc_min_quantizer = encoder_options_.min_quantizer;
    }
    if (encoder_options_.max_quantizer < 64) {
        cfg->rc_max_quantizer = encoder_options_.max_quantizer;
    }
    if (encoder_options_.keyframe_auto <= 1) {
        cfg->kf_mode = (vpx_kf_mode)encoder_options_.keyframe_auto;
    }
    if (encoder_options_.kf_min_dist <= 1000) {
        if (cfg->kf_mode) {
            MLOG_WARNING("Keyframe distance could only be set when kf_mode is disabled\n");
        } else {
            cfg->kf_min_dist = encoder_options_.kf_min_dist;
        }
    }
    if (encoder_options_.kf_max_dist <= 1000) {
        if (cfg->kf_mode) {
            MLOG_WARNING("Keyframe distance could only be set when kf_mode is disabled\n");
        } else {
            cfg->kf_max_dist = encoder_options_.kf_max_dist;
        }
    }

    cfg->g_w = width;
    cfg->g_h = height;

    /* write file header */
    if (output_ivf_) {
        WriteIvfFileHeader(cfg, 0);
    }

    /* init codec */
    if (vpx_codec_enc_init(&vpx_codec_, INTERFACE, cfg, 0)) {
        MLOG_ERROR("VP8 Failed to initialize encoder\n");
        goto EXIT;
    }

    /* set cpu use parameter */
    if(vpx_codec_control(&vpx_codec_, VP8E_SET_NOISE_SENSITIVITY, 1)) {
        MLOG_ERROR("VP8 Failed to set noise sensitivity\n");
        goto EXIT;
    }

    if(vpx_codec_control(&vpx_codec_,VP8E_SET_SHARPNESS, 1)) {
        MLOG_ERROR("VP8 Failed to set sharpness\n");
        goto EXIT;
    }

    if(vpx_codec_control(&vpx_codec_, VP8E_SET_CPUUSED, encoder_options_.cpu_speed)) {
        MLOG_ERROR("VP8 Failed to set cpu used\n");
        goto EXIT;
    }

    return MFX_ERR_NONE;

EXIT:
    vpx_img_free(&raw_);
    return MFX_ERR_UNKNOWN;
}

void VP8EncPlugin::WriteIvfFileHeader(const vpx_codec_enc_cfg_t *cfg, int frame_cnt) {
    char header[32];

    header[0] = 'D';
    header[1] = 'K';
    header[2] = 'I';
    header[3] = 'F';
    PlgPutMem16(header + 4,  0);                   /* version */
    PlgPutMem16(header + 6,  32);                  /* headersize */
    PlgPutMem32(header + 8,  FOURCC);              /* headersize */
    PlgPutMem16(header + 12, cfg->g_w);            /* width */
    PlgPutMem16(header + 14, cfg->g_h);            /* height */
    PlgPutMem32(header + 16, cfg->g_timebase.den); /* rate */
    PlgPutMem32(header + 20, cfg->g_timebase.num); /* scale */
    PlgPutMem32(header + 24, frame_cnt);           /* length */
    PlgPutMem32(header + 28, 0);                   /* unused */

    memcpy(m_BS->Data + m_BS->DataLength, header, 32);
    m_BS->DataLength += 32;
}

void VP8EncPlugin::WriteIvfFrameHeader(const vpx_codec_cx_pkt_t *pkt)
{
    char header[12];
    vpx_codec_pts_t pts;

    if(pkt->kind != VPX_CODEC_CX_FRAME_PKT) {
        return;
    }

    pts = pkt->data.frame.pts;
    PlgPutMem32(header, pkt->data.frame.sz);
    PlgPutMem32(header + 4, pts & 0xFFFFFFFF);
    PlgPutMem32(header + 8, pts >> 32);

    memcpy(m_BS->Data + m_BS->DataLength, header, 12);
    m_BS->DataLength += 12;
}

bool VP8EncPlugin::SetBitRateCtrlMode(unsigned int mode)
{
    if (mode != VPX_CBR && mode != VPX_VBR && mode != VPX_CQ) {
        MLOG_ERROR("Invalid encoding rate control mode\n");
        return false;
    }

    if (VPX_CBR == mode && encoder_options_.bitrate == 0) {
        MLOG_ERROR("Can not be CBR without configured bitrate\n");
        return false;
    }

    encoder_options_.bitratectrl = mode;
    return true;
}

bool VP8EncPlugin::SetBitRate(unsigned int bitrate)
{
    if (bitrate == 0) {
        MLOG_ERROR("Bitrate should not be equal to 0\n");
        return false;
    }

    encoder_options_.bitrate = bitrate; //kbps
    return true;
}

bool VP8EncPlugin::SetProfile(unsigned int profile)
{
    /* bitstream profile number : 0 - 3 */
    if (profile > 3) {
        MLOG_ERROR("Invalid encoding profile number\n");
        return false;
    }

    encoder_options_.profile = profile;
    return true;
}

bool VP8EncPlugin::SetGopSize(unsigned int gopsize)
{
    if (gopsize != 0) {
        keyframe_dist_ = encoder_options_.kf_min_dist = encoder_options_.kf_max_dist = gopsize;
    } else {
        keyframe_dist_ = encoder_options_.kf_min_dist = encoder_options_.kf_max_dist = DEFAULT_GOP_SIZE;
    }

    encoder_options_.keyframe_auto = VPX_KF_FIXED;
    return true;
}

bool VP8EncPlugin::SetGenericThread(unsigned int cpu_cores)
{
    if (m_width_ * m_height_ > 1280 * 960 && cpu_cores >= 6) {
        encoder_options_.threads = 3;  // 3 threads for 1080p
    } else if (m_width_ * m_height_ > 640 * 480 && cpu_cores >= 3) {
        encoder_options_.threads = 2;  // 2 threads for qHD/HD.
    } else {
        encoder_options_.threads = 1;  // 1 thread for VGA or less
    }
    return true;
}

bool VP8EncPlugin::SetCPUSpeed(VP8Complexity complexity_level)
{
    switch (complexity_level) {
        case complexityHigh:
            encoder_options_.cpu_speed = -5;
            break;
        case complexityHigher:
            encoder_options_.cpu_speed = -4;
            break;
        case complexityMax:
            encoder_options_.cpu_speed = -3;
            break;
        case complexityNormal:
        default:
            encoder_options_.cpu_speed = -6;
            break;
    }

    return true;
}

bool VP8EncPlugin::CheckParameters(mfxVideoParam *par)
{
    if (NULL == par) {
        MLOG_ERROR("Invalid input parameter\n");
        return false;
    }

    mfxInfoMFX *opt = &(par->mfx);

    m_width_ = opt->FrameInfo.CropW;
    m_height_ = opt->FrameInfo.CropH;

    //the number of cores is currently hard coded to be 4.
    //Need a function to get the correct information from the system.
    SetGenericThread(4);
    SetCPUSpeed(complexityNormal);

    /* set encoder configure parameters */
    SetProfile(opt->CodecProfile);

    SetBitRate(opt->TargetKbps);

    SetBitRateCtrlMode((MFX_RATECONTROL_CBR == opt->RateControlMethod) ? VPX_CBR :
        ((MFX_RATECONTROL_VBR == opt->RateControlMethod) ? VPX_VBR : VPX_CQ));

    SetGopSize(opt->GopPicSize);

    encoder_options_.time_base_num = opt->FrameInfo.FrameRateExtD;
    encoder_options_.time_base_den = opt->FrameInfo.FrameRateExtN;
    encoder_options_.min_quantizer = MIN_Q;
    encoder_options_.max_quantizer = MAX_Q;
    encoder_options_.output_type = par->Protected; //use Protected to mark output file type
    encoder_options_.is_raw_header = 0;

    /* set output flag */
    if ((unsigned int)RAW_FILE == encoder_options_.output_type) {
        output_ivf_ = false;
    }

    /* set raw stream header flag */
    if (!encoder_options_.is_raw_header) {
        is_raw_hdr_ = false;
    }

    return true;
}

mfxStatus VP8EncPlugin::GetInputYUV(mfxFrameSurface1* surf)
{
    if (frame_buf_ == NULL) {
        frame_buf_ = (&raw_)->planes[0];

        if (NULL == frame_buf_) {
            MLOG_ERROR("Invalid allocation for yuv buffer\n");
            return MFX_ERR_UNKNOWN;
        }
    }

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
                swap_buf_ = (unsigned char*)malloc(m_height_ * surf->Data.Pitch + m_height_ / 2 * surf->Data.Pitch);
            }
            if (swap_buf_ == NULL) {
                assert(0);
                return MFX_ERR_NULL_PTR;
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

    }else {
        MLOG_ERROR("Invalid surface!\n");
        assert(0);
    }

    return MFX_ERR_NONE;
}
#endif

