/* <COPYRIGHT_TAG> */

#ifdef ENABLE_PICTURE_CODEC
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "base/fast_copy.h"
#include "general_allocator.h"
#include "picture_decode_plugin.h"

#define ALIGN16(value) (((value + 15) >> 4) << 4)

mfxExtBuffer *PicDecPlugin::GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
{
    if (!ebuffers) {
        return NULL;
    }

    for (mfxU32 i = 0; i < nbuffers; i++) {
        if (!ebuffers[i]) {
            continue;
        }

        if (ebuffers[i]->BufferId == BufferId) {
            return ebuffers[i];
        }
    }

    return NULL;
}

PicDecPlugin::PicDecPlugin()
{
    memset(&m_mfxPluginParam, 0, sizeof(m_mfxPluginParam));
    m_MaxNumTasks = 0;
    m_mfxPluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_mfxPluginParam.MaxThreadNum = 1;
    //Init parameters
    offset_bytes_ = 0;
    bmp_width_ = 0;
    bmp_height_ = 0;
    bit_cnt_ = 0;
    bytes_per_line_ = 0;
    bmp_data_ = NULL;
    yuv_buf_ = NULL;
    pic_info_ = NULL;
}

mfxStatus PicDecPlugin::Init(mfxVideoParam *param)
{
    mfxStatus sts = MFX_ERR_NONE;
    m_VideoParam = *param;

    if (m_bIsOpaque) {
        m_ExtParam = *m_VideoParam.ExtParam;
        mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(&m_ExtParam, m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

        if (pluginOpaqueAlloc == NULL) {
            assert(0);
            return MFX_ERR_NULL_PTR;
        }
        if (pluginOpaqueAlloc->Out.Surfaces == NULL) {
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
    m_pTasks = new MFXTask3[m_MaxNumTasks];
    memset(m_pTasks, 0, sizeof(MFXTask3) * m_MaxNumTasks);
    return MFX_ERR_NONE;
}

mfxStatus PicDecPlugin::DecodeHeader(PicInfo *picinfo, mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    pic_info_ = picinfo;
    char *bmp = pic_info_->bmp;
    // Detect whether is bitmap
    char first_byte = *bmp;
    char second_byte = *(bmp + 1);
    printf("first_byte is %0x, second_byte is %0x\n", first_byte, second_byte);

    if ((first_byte != 0x42) || (second_byte != 0x4d)) {
        printf("It is not bitmap.\n");
        return MFX_ERR_UNKNOWN;
    }

    unsigned char byte1 = (unsigned char)(*(bmp + 10));
    unsigned char byte2 = (unsigned char)(*(bmp + 11));
    unsigned char byte3 = (unsigned char)(*(bmp + 12));
    unsigned char byte4 = (unsigned char)(*(bmp + 13));
    offset_bytes_ = (unsigned long)byte1 + ((unsigned long)byte2 << 8)
                    + ((unsigned long)byte3 << 16) + ((unsigned long)byte4 << 24);
    //if (offset_bytes_ != 54) {
    //printf("This bitmap has color table, doesn't belong to processing scope.\n");
    //return MFX_ERR_UNKNOWN;
    //}
    bmp_data_ = bmp + offset_bytes_;
    byte1 = (unsigned char)(*(bmp + 18));
    byte2 = (unsigned char)(*(bmp + 19));
    byte3 = (unsigned char)(*(bmp + 20));
    byte4 = (unsigned char)(*(bmp + 21));
    bmp_width_ = (unsigned long)byte1 + ((unsigned long)byte2 << 8)
                 + ((unsigned long)byte3 << 16) + ((unsigned long)byte4 << 24);
    byte1 = (unsigned char)(*(bmp + 22));
    byte2 = (unsigned char)(*(bmp + 23));
    byte3 = (unsigned char)(*(bmp + 24));
    byte4 = (unsigned char)(*(bmp + 25));
    bmp_height_ = (unsigned long)byte1 + ((unsigned long)byte2 << 8)
                  + ((unsigned long)byte3 << 16) + ((unsigned long)byte4 << 24);
    byte1 = (unsigned char)(*(bmp + 28));
    byte2 = (unsigned char)(*(bmp + 29));
    bit_cnt_ = (int)byte1 + ((int)byte2 << 8);

    if ((bit_cnt_ == 24) && (offset_bytes_ != 54)) {
        printf("This bitmap is 24bits/pixel but has color table, not belong to processing scope.\n");
        return MFX_ERR_UNKNOWN;
    }

    if ((bit_cnt_ == 32) && (offset_bytes_ != 54)) {
        printf("This bitmap is 32bits/pixel but has color table, not belong to processing scope.\n");
        return MFX_ERR_UNKNOWN;
    }

    if ((bit_cnt_ == 16) && (offset_bytes_ <= 54)) {
        printf("This bitmap is 16bits/pixel but doesn't have color table, not belong to processing scope.\n");
        return MFX_ERR_UNKNOWN;
    }

    byte1 = (unsigned char)(*(bmp + 30));
    byte2 = (unsigned char)(*(bmp + 31));
    byte3 = (unsigned char)(*(bmp + 32));
    byte4 = (unsigned char)(*(bmp + 33));

    if (((byte1 != 0) || (byte2 != 0) || (byte3 != 0) || (byte4 != 0)) && ((bit_cnt_ == 24) || (bit_cnt_ == 32))) {
        printf("This bitmap is 24bits/pixel or 32bits/pixel meanwhile has been compressed, doesn't belong to processig scope.\n");
        return MFX_ERR_UNKNOWN;
    }

    if (((byte1 == 0) && (byte2 == 0) && (byte3 == 0) && (byte4 == 0)) && (bit_cnt_ == 16)) {
        printf("This bitmap is 16bits/pixel meanwhile has not been compressed, doesn't belong to processing scope.\n");
        return MFX_ERR_UNKNOWN;
    }

    bytes_per_line_ = (((bmp_width_ * (bit_cnt_ / 8)) + 3) / 4) * 4;
    par->mfx.FrameInfo.Width = ALIGN16(bmp_width_);
    par->mfx.FrameInfo.Height = ALIGN16(bmp_height_);
    par->mfx.FrameInfo.CropW = bmp_width_;
    par->mfx.FrameInfo.CropH = bmp_height_;
    par->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    par->mfx.FrameInfo.FrameRateExtN = 120;
    par->mfx.FrameInfo.FrameRateExtD = 4;
    par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    m_bIsOpaque = par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY ? true : false;
    return sts;
}

mfxStatus PicDecPlugin::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
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

mfxStatus PicDecPlugin::Close()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_bIsOpaque) {
        mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(&m_ExtParam, m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

        if (pluginOpaqueAlloc == NULL) {
            assert(0);
            return MFX_ERR_NULL_PTR;
        }
        if (pluginOpaqueAlloc->Out.Surfaces == NULL) {
            assert(0);
            return MFX_ERR_NULL_PTR;
        } else {
            sts = m_pmfxCore->UnmapOpaqueSurface(m_pmfxCore->pthis,
                                                 pluginOpaqueAlloc->Out.NumSurface,
                                                 pluginOpaqueAlloc->Out.Type,
                                                 pluginOpaqueAlloc->Out.Surfaces);
        }
    }

    delete [] m_pTasks;

    if (yuv_buf_) {
        free(yuv_buf_);
        yuv_buf_ = NULL;
    }

    return MFX_ERR_NONE;
}

PicDecPlugin::~PicDecPlugin()
{
}

void PicDecPlugin::SetAllocPoint(MFXFrameAllocator *pMFXAllocator)
{
    if (m_bIsOpaque) {
        m_pAlloc = &m_pmfxCore->FrameAllocator;
    } else {
        m_pAlloc = pMFXAllocator;
    }
}

mfxStatus PicDecPlugin::PluginInit(mfxCoreInterface *core)
{
    mfxStatus sts = MFX_ERR_NONE;
    m_pmfxCore = new mfxCoreInterface;

    if (!m_pmfxCore) {
        return MFX_ERR_MEMORY_ALLOC;
    }

    *m_pmfxCore = *core;
    m_pAlloc = NULL;
    return sts;
}

mfxStatus PicDecPlugin::PluginClose()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_pmfxCore) {
        delete m_pmfxCore;
        m_pmfxCore = NULL;
    }

    return sts;
}

mfxStatus PicDecPlugin::GetPluginParam(mfxPluginParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    *par = m_mfxPluginParam;
    return sts;
}

mfxStatus PicDecPlugin::Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task)
{
    mfxStatus sts = MFX_ERR_NONE;
    // find free task in task array
    int taskIdx;

    for (taskIdx = 0; taskIdx < m_MaxNumTasks; taskIdx++) {
        if (!m_pTasks[taskIdx].bBusy) {
            break;
        }
    }

    if (taskIdx == m_MaxNumTasks) {
        return MFX_WRN_DEVICE_BUSY; //currently there is no free task available
    }

    if (NULL == in) {
        return MFX_ERR_MORE_DATA;
    }

    MFXTask3 *pTask = &m_pTasks[taskIdx];
    PicInfo *pic_in = pic_info_;
    mfxFrameSurface1 *opaque_surface_out = (mfxFrameSurface1 *)out[0];
    mfxFrameSurface1 *real_surf_out = NULL;

    if (m_bIsOpaque) {
        sts = m_pmfxCore->GetRealSurface(m_pmfxCore->pthis, opaque_surface_out, &real_surf_out);
        assert(sts == MFX_ERR_NONE);
    } else {
        real_surf_out = opaque_surface_out;
    }

    pTask->picInfo    = pic_in;
    pTask->surfaceOut = real_surf_out;
    pTask->bBusy      = true;
    *task = (mfxThreadTask)pTask;
    m_pmfxCore->IncreaseReference(m_pmfxCore->pthis, &pTask->surfaceOut->Data);
    return sts;
}

mfxStatus PicDecPlugin::Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a)
{
    MFXTask3 *pTask = (MFXTask3 *)task;
    mfxStatus sts = MFX_ERR_NONE;
    PicInfo *pic_in = pTask->picInfo;
    mfxFrameSurface1 *surf_out = pTask->surfaceOut;
    m_pAlloc->Lock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
    sts = Bmp2Yuv(bmp_data_, bmp_width_, bmp_height_, pic_in);

    if (sts != MFX_ERR_NONE) {
        printf("Failed to convert the bmp to NV12 frame.\n");
        m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);

        if (yuv_buf_) {
            free(yuv_buf_);
            yuv_buf_ = NULL;
        }

        return sts;
    }

    sts = UploadToSurface(yuv_buf_, surf_out);

    if (sts != MFX_ERR_NONE) {
        printf("Failed to upload the bmp to surface.\n");
        m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);

        if (yuv_buf_) {
            free(yuv_buf_);
            yuv_buf_ = NULL;
        }

        return sts;
    }

    m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
    return MFX_TASK_DONE;
}

mfxStatus PicDecPlugin::FreeResources(mfxThreadTask task, mfxStatus sts)
{
    MFXTask3 *pTask = (MFXTask3 *)task;
    m_pmfxCore->DecreaseReference(m_pmfxCore->pthis, &pTask->surfaceOut->Data);
    pTask->bBusy = false;
    return MFX_ERR_NONE;
}

mfxStatus PicDecPlugin::SetAuxParams(void *auxParam, int auxParamSize)
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}

void PicDecPlugin::Release()
{
}

mfxStatus PicDecPlugin::Bmp2Yuv(char *bmp_data, int bmp_width, int bmp_height, PicInfo *picinfo)
{
    mfxStatus sts = MFX_ERR_NONE;
    char *data = bmp_data;

    if (!yuv_buf_) {
        yuv_buf_ = (unsigned char *)malloc((bmp_width * bmp_height * 3) / 2);

        if (yuv_buf_ == NULL) {
            printf("Error in mallocing yuv_buf_.\n");
            return MFX_ERR_UNKNOWN;
        }
    } else {
        memset(yuv_buf_, 0, ((bmp_width * bmp_height * 3) / 2));
    }

    unsigned char *yuv_buf = yuv_buf_;
    int width = bmp_width;
    int height = bmp_height;
    unsigned char *y_ptr = yuv_buf;
    unsigned char *uv_ptr = yuv_buf + width * height;
    unsigned char *u_ptr = (unsigned char *)malloc(width * height);
    unsigned char *v_ptr = (unsigned char *)malloc(width * height);

    if (!u_ptr || !v_ptr) {
        printf("Error in mallocing u_ptr or v_ptr.\n");

        if (u_ptr) {
            free(u_ptr);
            u_ptr = NULL;
        }

        if (v_ptr) {
            free(v_ptr);
            v_ptr = NULL;
        }

        return MFX_ERR_UNKNOWN;
    }

    memset(u_ptr, 0, width*height*sizeof(unsigned char));
    memset(v_ptr, 0, width*height*sizeof(unsigned char));
    unsigned char *rgb_r = (unsigned char *)malloc(width * height);
    unsigned char *rgb_g = (unsigned char *)malloc(width * height);
    unsigned char *rgb_b = (unsigned char *)malloc(width * height);

    if (!rgb_r || !rgb_g || !rgb_b) {
        printf("Error in mallocing rgb_r or rgb_g or rgb_b.\n");

        if (rgb_r) {
            free(rgb_r);
            rgb_r = NULL;
        }

        if (rgb_g) {
            free(rgb_g);
            rgb_g = NULL;
        }

        if (rgb_b) {
            free(rgb_b);
            rgb_b = NULL;
        }

        free(u_ptr);
        u_ptr = NULL;
        free(v_ptr);
        v_ptr = NULL;
        return MFX_ERR_UNKNOWN;
    }

    if (bit_cnt_ == 16) {
        //printf("This bitmap is 16 bits/pixel.\n");
        sts = GetRGB16(data, rgb_r, rgb_g, rgb_b);
    } else if (bit_cnt_ == 24) {
        //printf("This bitmap is 24 bits/pixel.\n");
        sts = GetRGB24(data, rgb_r, rgb_g, rgb_b);
    } else if (bit_cnt_ == 32) {
        //printf("This bitmap is 32 bits/pixel.\n");
        sts = GetRGB32(data, rgb_r, rgb_g, rgb_b);
    } else {
        printf("This bitmap is %d bits/pixel. It doesn't belong to processing scope.\n", bit_cnt_);
        free(u_ptr);
        u_ptr = NULL;
        free(v_ptr);
        v_ptr = NULL;
        free(rgb_r);
        rgb_r = NULL;
        free(rgb_g);
        rgb_g = NULL;
        free(rgb_b);
        rgb_b = NULL;
        return MFX_ERR_UNKNOWN;
    }

    if (sts != MFX_ERR_NONE) {
        free(u_ptr);
        u_ptr = NULL;
        free(v_ptr);
        v_ptr = NULL;
        free(rgb_r);
        rgb_r = NULL;
        free(rgb_g);
        rgb_g = NULL;
        free(rgb_b);
        rgb_b = NULL;
        return sts;
    }

    unsigned char R, G, B;
    unsigned char Y, U, V;
    int i, j;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            R = rgb_r[(height - 1 - i) * width + j];
            G = rgb_g[(height - 1 - i) * width + j];
            B = rgb_b[(height - 1 - i) * width + j];
            Y = (unsigned char)(0.299 * R + 0.587 * G +  0.114 * B);
            V = (unsigned char)(0.5 * R -  0.4187 * G - 0.0813 * B + 128);
            U = (unsigned char)(-0.1687 * R -  0.3313 * G + 0.5 * B + 128);
            y_ptr[width * i + j] = Y;
            u_ptr[width * i + j] = U;
            v_ptr[width * i + j] = V;
        }
    }

    for (i = 0; i < (height / 2); i++) {
        for (j = 0; j < (width / 2); j++) {
            U = (unsigned char)((u_ptr[width * (2 * i) + 2 * j] + u_ptr[width * (2 * i) + 2 * j + 1]
                                 + u_ptr[width * (2 * i + 1) + 2 * j] + u_ptr[width * (2 * i + 1) + 2 * j + 1] + 2) >> 2);
            V = (unsigned char)((v_ptr[width * (2 * i) + 2 * j] + v_ptr[width * (2 * i) + 2 * j + 1]
                                 + v_ptr[width * (2 * i + 1) + 2 * j] + v_ptr[width * (2 * i + 1) + 2 * j + 1] + 2) >> 2);
            uv_ptr[width * i + 2 * j] = U;
            uv_ptr[width * i + 2 * j + 1] = V;
        }
    }

    free(u_ptr);
    u_ptr = NULL;
    free(v_ptr);
    v_ptr = NULL;
    free(rgb_r);
    rgb_r = NULL;
    free(rgb_g);
    rgb_g = NULL;
    free(rgb_b);
    rgb_b = NULL;
    return MFX_ERR_NONE;
}

mfxStatus PicDecPlugin::GetRGB16(char *bmp_data, unsigned char *rgb_r, unsigned char *rgb_g, unsigned char *rgb_b)
{
    if (!bmp_data || !rgb_r || !rgb_g || !rgb_b) {
        printf("Invalid pointer for GetRGB16.\n");
        return MFX_ERR_NULL_PTR;
    }

    char *data = bmp_data;
    unsigned char *ptr_r = rgb_r;
    unsigned char *ptr_g = rgb_g;
    unsigned char *ptr_b = rgb_b;
    int i, j;
    unsigned char R, G, B;
    unsigned char temp1, temp2;

    for (i = 0; i < bmp_height_; i++) {
        for (j = 0; j < (bmp_width_ * 2); j += 2) {
            B = (((unsigned char)data[j]) & 0x1F) << 3;
            temp1 = (((unsigned char)data[j]) & 0xE0) >> 3;
            temp2 = (((unsigned char)data[j + 1]) & 0x07) << 5;
            G = (unsigned char)(temp1 + temp2);
            R = (((unsigned char)data[j + 1]) & 0xF8);
            *ptr_r = R;
            *ptr_g = G;
            *ptr_b = B;
            ptr_r++;
            ptr_g++;
            ptr_b++;
        }

        data += bytes_per_line_;
    }

    return MFX_ERR_NONE;
}

mfxStatus PicDecPlugin::GetRGB24(char *bmp_data, unsigned char *rgb_r, unsigned char *rgb_g, unsigned char *rgb_b)
{
    if (!bmp_data || !rgb_r || !rgb_g || !rgb_b) {
        printf("Invalid pointer for GetRGB24.\n");
        return MFX_ERR_NULL_PTR;
    }

    char *data = bmp_data;
    unsigned char *ptr_r = rgb_r;
    unsigned char *ptr_g = rgb_g;
    unsigned char *ptr_b = rgb_b;
    int i, j;
    unsigned char R, G, B;

    for (i = 0; i < bmp_height_; i++) {
        for (j = 0; j < (bmp_width_ * 3); j += 3) {
            B = data[j];
            G = data[j + 1];
            R = data[j + 2];
            *ptr_r = R;
            *ptr_g = G;
            *ptr_b = B;
            ptr_r++;
            ptr_g++;
            ptr_b++;
        }

        data += bytes_per_line_;
    }

    return MFX_ERR_NONE;
}

mfxStatus PicDecPlugin::GetRGB32(char *bmp_data, unsigned char *rgb_r, unsigned char *rgb_g, unsigned char *rgb_b)
{
    if (!bmp_data || !rgb_r || !rgb_g || !rgb_b) {
        printf("Invalid pointer for GetRGB32.\n");
        return MFX_ERR_NULL_PTR;
    }

    char *data = bmp_data;
    unsigned char *ptr_r = rgb_r;
    unsigned char *ptr_g = rgb_g;
    unsigned char *ptr_b = rgb_b;
    int i, j;
    unsigned char R, G, B;

    for (i = 0; i < bmp_height_; i++) {
        for (j = 0; j < (bmp_width_ * 4); j += 4) {
            B = data[j];
            G = data[j + 1];
            R = data[j + 2];
            *ptr_r = R;
            *ptr_g = G;
            *ptr_b = B;
            ptr_r++;
            ptr_g++;
            ptr_b++;
        }

        data += bytes_per_line_;
    }

    return MFX_ERR_NONE;
}

mfxStatus PicDecPlugin::UploadToSurface(unsigned char *yuv_buf, mfxFrameSurface1 *surface_out)
{
    int row;
    unsigned char *y_dst, *uv_dst;
    unsigned char *y_src, *uv_src;
    y_src = yuv_buf;
    uv_src = yuv_buf + bmp_width_ * bmp_height_;
    y_dst = surface_out->Data.Y;
    uv_dst = surface_out->Data.VU;

    /* Y plane */
    for (row = 0; row < surface_out->Info.Height; row++) {
        fast_copy(y_dst, y_src, surface_out->Info.CropW);
        y_dst += surface_out->Data.Pitch;
        y_src += bmp_width_;
    }

    /* UV plane */
    if (surface_out->Info.FourCC == MFX_FOURCC_NV12) {
        for (row = 0; row < (surface_out->Info.Height / 2); row++) {
            fast_copy(uv_dst, uv_src, surface_out->Info.CropW);
            uv_dst += surface_out->Data.Pitch;
            uv_src += bmp_width_;
        }
    } else {
        printf("Surface Fourcc not supported!\n");
        return MFX_ERR_UNKNOWN;
    }

    return MFX_ERR_NONE;
}
#endif
