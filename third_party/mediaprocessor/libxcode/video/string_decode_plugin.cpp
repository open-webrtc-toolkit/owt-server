/* <COPYRIGHT_TAG> */

#ifdef ENABLE_STRING_CODEC
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "base/fast_copy.h"
#include "general_allocator.h"
#include "string_decode_plugin.h"

#define ALIGN16(value) (((value + 15) >> 4) << 4)

mfxExtBuffer *StringDecPlugin::GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
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

StringDecPlugin::StringDecPlugin()
{
    memset(&m_mfxPluginParam, 0, sizeof(m_mfxPluginParam));
    m_MaxNumTasks = 0;
    m_mfxPluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_mfxPluginParam.MaxThreadNum = 1;
    //Init parameters
    freetype_init_ = false;
    text_data_ = NULL;
    string_info_ = NULL;
}

mfxStatus StringDecPlugin::Init(mfxVideoParam *param)
{
    mfxStatus sts = MFX_ERR_NONE;
    m_VideoParam = *param;

    if (m_bIsOpaque) {
        m_ExtParam = *m_VideoParam.ExtParam;
        mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(&m_ExtParam, m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

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
    m_pTasks = new MFXTask2[m_MaxNumTasks];
    memset(m_pTasks, 0, sizeof(MFXTask2) * m_MaxNumTasks);
    yuv_buf_ = NULL;
    return MFX_ERR_NONE;
}

mfxStatus StringDecPlugin::DecodeHeader(StringInfo *strinfo, mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    string_info_ = strinfo;

    // Init freetype
    if (!freetype_init_) {
        sts = InitFreeType(string_info_);
        assert(MFX_ERR_NONE == sts);
        freetype_init_ = true;
    }

    sts = RenderString(string_info_);

    if (sts != MFX_ERR_NONE) {
        printf("Failed to render the string to bmp.\n");

        if (text_data_) {
            for (int i = 0; i < bmp_height_; i++) {
                if (text_data_[i]) {
                    free(text_data_[i]);
                    text_data_[i] = NULL;
                }
            }

            free(text_data_);
            text_data_ = NULL;
        }
        return sts;
    }

    par->mfx.FrameInfo.Width = ALIGN16(bmp_width_ / 3);
    printf("bmp_height_ is %d\n", bmp_height_);
    par->mfx.FrameInfo.Height = ALIGN16(bmp_height_);
    par->mfx.FrameInfo.CropW = bmp_width_ / 3;
    par->mfx.FrameInfo.CropH = bmp_height_;
    par->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    par->mfx.FrameInfo.FrameRateExtN = 120;
    par->mfx.FrameInfo.FrameRateExtD = 4;
    //par->mfx.FrameInfo.AspectRatioW = 1;
    //par->mfx.FrameInfo.AspectRatioH = 1;
    par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    m_bIsOpaque = par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY ? true : false;
    return sts;
}

mfxStatus StringDecPlugin::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
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

mfxStatus StringDecPlugin::Close()
{
    mfxStatus sts = MFX_ERR_NONE;
    // Release FreeType object
    sts = RelFreeType();

    if (m_bIsOpaque) {
        mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(&m_ExtParam, m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (pluginOpaqueAlloc == NULL) {
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

    if (text_data_) {
        for (int i = 0; i < bmp_height_; i++) {
            if (text_data_[i]) {
                free(text_data_[i]);
                text_data_[i] = NULL;
            }
        }

        free(text_data_);
        text_data_ = NULL;
    }

    if (yuv_buf_) {
        free(yuv_buf_);
        yuv_buf_ = NULL;
    }

    return MFX_ERR_NONE;
}

StringDecPlugin::~StringDecPlugin()
{
}

// Media SDK plugin interface
void StringDecPlugin::SetAllocPoint(MFXFrameAllocator *pMFXAllocator)
{
    if (m_bIsOpaque) {
        m_pAlloc = &m_pmfxCore->FrameAllocator;
    } else {
        m_pAlloc = pMFXAllocator;
    }
}

mfxStatus StringDecPlugin::PluginInit(mfxCoreInterface *core)
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

mfxStatus StringDecPlugin::PluginClose()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_pmfxCore) {
        delete m_pmfxCore;
        m_pmfxCore = NULL;
    }

    return sts;
}

mfxStatus StringDecPlugin::GetPluginParam(mfxPluginParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    *par = m_mfxPluginParam;
    return sts;
}

mfxStatus StringDecPlugin::Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task)
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

    MFXTask2 *pTask = &m_pTasks[taskIdx];
    StringInfo *string_in = string_info_;
    mfxFrameSurface1 *opaque_surface_out = (mfxFrameSurface1 *)out[0];
    mfxFrameSurface1 *real_surf_out = NULL;

    if (m_bIsOpaque) {
        sts = m_pmfxCore->GetRealSurface(m_pmfxCore->pthis, opaque_surface_out, &real_surf_out);
        assert(sts == MFX_ERR_NONE);
    } else {
        real_surf_out = opaque_surface_out;
    }

    pTask->stringInfo = string_in;
    pTask->surfaceOut = real_surf_out;
    pTask->bBusy      = true;
    *task = (mfxThreadTask)pTask;
    // To make sure up- or downstream pipeline component does not modify data until we are done.
    m_pmfxCore->IncreaseReference(m_pmfxCore->pthis, &pTask->surfaceOut->Data);
    return sts;
}

mfxStatus StringDecPlugin::Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a)
{
    MFXTask2 *pTask = (MFXTask2 *)task;
    mfxStatus sts = MFX_ERR_NONE;
    StringInfo *string_in = pTask->stringInfo;
    mfxFrameSurface1 *surf_out = pTask->surfaceOut;
    m_pAlloc->Lock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
    sts = Bmp2Yuv(text_data_, bmp_width_, bmp_height_, string_in);

    if (sts != MFX_ERR_NONE) {
        printf("Failed to convert the bmp to NV12 frame.\n");
        m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);

        for (int i = 0; i < bmp_height_; i++) {
            free(text_data_[i]);
            text_data_[i] = NULL;
        }

        free(text_data_);
        text_data_ = NULL;
        if (yuv_buf_) {
            free(yuv_buf_);
            yuv_buf_ = NULL;
        }
        return sts;
    }

    sts = UploadToSurface(yuv_buf_, surf_out);

    if (sts != MFX_ERR_NONE) {
        printf("Failed to upload the string picture to surface.\n");
        m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);

        for (int i = 0; i < bmp_height_; i++) {
            free(text_data_[i]);
            text_data_[i] = NULL;
        }

        free(text_data_);
        text_data_ = NULL;
        if (yuv_buf_) {
            free(yuv_buf_);
            yuv_buf_ = NULL;
        }
        return sts;
    }

    m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
    return MFX_TASK_DONE;
}

mfxStatus StringDecPlugin::FreeResources(mfxThreadTask task, mfxStatus sts)
{
    MFXTask2 *pTask = (MFXTask2 *)task;
    m_pmfxCore->DecreaseReference(m_pmfxCore->pthis, &pTask->surfaceOut->Data);
    pTask->bBusy = false;
    return MFX_ERR_NONE;
}

mfxStatus StringDecPlugin::SetAuxParams(void *auxParam, int auxParamSize)
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}

void StringDecPlugin::Release()
{
}

mfxStatus StringDecPlugin::InitFreeType(StringInfo *strinfo)
{
    FT_Error error;
    char *font_file = strinfo->font;

    error = FT_Init_FreeType(&freetype_library_);
    if (error) {
        printf("Error in initializing FreeType library.\n");
        return MFX_ERR_UNKNOWN;
    }

    printf("font_file %s\n", font_file);
    error = FT_New_Face(freetype_library_, font_file, 0, &font_face_);

    if (error == FT_Err_Unknown_File_Format) {
        printf("Font file could be opened and read, but it appears its font is unsupported.\n");
        return MFX_ERR_UNKNOWN;
    } else if (error) {
        printf("Error in opening or reading font file.\n");
        return MFX_ERR_UNKNOWN;
    }

    return MFX_ERR_NONE;
}

mfxStatus StringDecPlugin::Bmp2Yuv(char **text_data, int bmp_width, int bmp_height, StringInfo *strinfo)
{
    char **img_data = text_data;
    unsigned char rgb_r = strinfo->rgb_r;
    unsigned char rgb_g = strinfo->rgb_g;
    unsigned char rgb_b = strinfo->rgb_b;
    if (NULL == yuv_buf_) {
        yuv_buf_ = (unsigned char *)malloc(((bmp_width / 3) * bmp_height * 3) / 2);
        if (yuv_buf_ == NULL) {
            printf("Error in mallocing yuv_buf_.\n");
            return MFX_ERR_UNKNOWN;
        }
    } else {
        memset(yuv_buf_, 0, (((bmp_width / 3) * bmp_height * 3) / 2) * sizeof(unsigned char));
    }

    unsigned char *yuv_buf = yuv_buf_;
    int width = bmp_width / 3;
    int height = bmp_height;
    unsigned char *y_ptr = yuv_buf;
    unsigned char *uv_ptr = yuv_buf + width * height;
    unsigned char *u_ptr = (unsigned char *)malloc(width * height);
    unsigned char *v_ptr = (unsigned char *)malloc(width * height);

    if (!u_ptr || !v_ptr) {
        printf("Error in mallocing u_ptr or v_ptr.\n");
        if (u_ptr) {
            free(u_ptr);
        }
        if (v_ptr) {
            free(v_ptr);
        }
        return MFX_ERR_UNKNOWN;
    }

    memset(u_ptr, 0, width * height * sizeof(unsigned char));
    memset(v_ptr, 0, width * height * sizeof(unsigned char));

    int rgb_idx = 0;
    unsigned char R, G, B;
    unsigned char Y, U, V;
    int i, j;

    for (i = 0; i < height; i++) {
        rgb_idx = 0;

        for (j = 0; j < width; j++) {
            R = (unsigned char)img_data[i][rgb_idx++];
            G = (unsigned char)img_data[i][rgb_idx++];
            B = (unsigned char)img_data[i][rgb_idx++];

            if (R != 0) {
                if ((rgb_r != 255) || (rgb_g != 255) || (rgb_b != 255)) {
                    if ((rgb_r == 0) && (rgb_g == 0) && (rgb_b == 0)) {
                        printf("The color of character can't be black.\n");
                        if (u_ptr) {
                            free(u_ptr);
                        }
                        if (v_ptr) {
                            free(v_ptr);
                        }
                        return MFX_ERR_UNKNOWN;
                    }

                    R = rgb_r;
                    G = rgb_g;
                    B = rgb_b;
                }
            }

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
    return MFX_ERR_NONE;
}

mfxStatus StringDecPlugin::RenderString(StringInfo *stringInfo)
{
    FT_Error   error;
    FT_GlyphSlot slot;
    FT_UInt      glyph_idx;
    int          pen_x, pen_y, n, k;
    int          i, j;
    int          start_y;
    char *string = stringInfo->text;
    unsigned int num_chars = strlen(string);
    unsigned int plsize = stringInfo->plsize;
    error = FT_Set_Pixel_Sizes(font_face_, 0, plsize);

    if (error) {
        printf("Error in setting character pixel size.\n");
        return MFX_ERR_UNKNOWN;
    }

    slot = font_face_->glyph;
    pen_x = plsize >> 3;
    pen_y = plsize >> 2;
    bmp_width_ = 0;
    bmp_height_ = plsize + pen_y;
    text_data_ = (char **)malloc(sizeof(char *) * bmp_height_);

    if (text_data_ == NULL) {
        printf("Error in malloc bmp.\n");
        return MFX_ERR_UNKNOWN;
    }

    for (n = 0; n < num_chars; n++) {
        if (string[n] == ' ') {
            if (n == 0) {
                for (k = 0; k < bmp_height_; k++) {
                    text_data_[k] = (char *)malloc(sizeof(char) * ((plsize / 2) * 3));

                    if (text_data_[k] == NULL) {
                        printf("Error in malloc bmp each line.\n");
                        return MFX_ERR_UNKNOWN;
                    }

                    for (i = 0; i < ((plsize / 2) * 3); i++) {
                        text_data_[k][i] = 0;
                    }
                }
            } else {
                for (k = 0; k < bmp_height_; k++) {
                    text_data_[k] = (char *)realloc(text_data_[k], sizeof(char) * (bmp_width_ + (plsize / 2) * 3));

                    if (text_data_[k] == NULL) {
                        printf("Error in realloc bmp each line.\n");
                        return MFX_ERR_UNKNOWN;
                    }

                    for (i = bmp_width_; i < (bmp_width_ + (plsize / 2) * 3); i++) {
                        text_data_[k][i] = 0;
                    }
                }
            }

            bmp_width_ += (plsize / 2) * 3;
        } else {
            //convert character code to glyph index
            glyph_idx = FT_Get_Char_Index(font_face_, string[n]);
            //load glyph image into the slot (erase previous one)
            error = FT_Load_Glyph(font_face_, glyph_idx, FT_LOAD_DEFAULT);

            if (error) {
                continue; // ignore errors
            }

            FT_Glyph glyph;
            error = FT_Get_Glyph(font_face_->glyph, &glyph);

            if (error) {
                printf("Error in getting glyph.\n");

                for (k = 0; k < bmp_height_; k++) {
                    if (text_data_[k]) {
                        free(text_data_[k]);
                        text_data_[k] = NULL;
                    }
                }

                free(text_data_);
                text_data_ = NULL;
                return MFX_ERR_UNKNOWN;
            }

            //convert to an anti-aliased bitmap
            error = FT_Render_Glyph(font_face_->glyph, FT_RENDER_MODE_NORMAL);

            if (error) {
                printf("Error in rendering glyph.\n");

                for (k = 0; k < bmp_height_; k++) {
                    if (text_data_[k]) {
                        free(text_data_[k]);
                        text_data_[k] = NULL;
                    }
                }

                free(text_data_);
                text_data_ = NULL;
                return MFX_ERR_UNKNOWN;
            }

            error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);

            if (error) {
                printf("Error in converting glyph to bitmap.\n");

                for (k = 0; k < bmp_height_; k++) {
                    if (text_data_[k]) {
                        free(text_data_[k]);
                        text_data_[k] = NULL;
                    }
                }

                free(text_data_);
                text_data_ = NULL;
                return MFX_ERR_UNKNOWN;
            }

            FT_BitmapGlyph bmp_glyph = (FT_BitmapGlyph)glyph;
            FT_Bitmap &bitmap = bmp_glyph->bitmap;
            int width = bitmap.width;
            int height = bitmap.rows;

            if ((width & 0x3) != 0) {
                width += 4 - (width & 0x3);
            }

            if (n == 0) {
                for (k = 0; k < bmp_height_; k++) {
                    text_data_[k] = (char *)malloc(sizeof(char) * ((width + pen_x) * 3));

                    if (text_data_[k] == NULL) {
                        printf("Error in malloc bmp each line.\n");
                        return MFX_ERR_UNKNOWN;
                    }

                    for (i = 0; i < ((width + pen_x) * 3); i++) {
                        text_data_[k][i] = 0;
                    }
                }
            } else {
                for (k = 0; k < bmp_height_; k++) {
                    text_data_[k] = (char *)realloc(text_data_[k], sizeof(char) * (bmp_width_ + (width + pen_x) * 3));

                    if (text_data_[k] == NULL) {
                        printf("Error in realloc bmp each line.\n");
                        return MFX_ERR_UNKNOWN;
                    }

                    for (i = bmp_width_; i < (bmp_width_ + (width + pen_x) * 3); i++) {
                        text_data_[k][i] = 0;
                    }
                }
            }

            start_y = plsize - slot->bitmap_top;

            for (i = 0; i < bmp_height_; i++) {
                for (j = 0; j < width; j++) {
                    if (i >= start_y && i < start_y + height) {
                        text_data_[i][bmp_width_ + 3 * j] =
                            text_data_[i][bmp_width_ + 3 * j + 1] =
                                text_data_[i][bmp_width_ + 3 * j + 2] = (j >= bitmap.width || i >= start_y + bitmap.rows) ? 0 : bitmap.buffer[j + bitmap.width * (i - start_y)];
                    } else {
                        text_data_[i][bmp_width_ + 3 * j] =
                            text_data_[i][bmp_width_ + 3 * j + 1] =
                                text_data_[i][bmp_width_ + 3 * j + 2] = 0;
                    }
                }
            }

            bmp_width_ += (width + pen_x) * 3;
            FT_Done_Glyph(glyph);
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus StringDecPlugin::UploadToSurface(unsigned char *yuv_buf, mfxFrameSurface1 *surface_out)
{
    int row;
    unsigned char *y_dst, *uv_dst;
    unsigned char *y_src, *uv_src;
    y_src = yuv_buf;
    uv_src = yuv_buf + (bmp_width_ / 3) * bmp_height_;
    y_dst = surface_out->Data.Y;
    uv_dst = surface_out->Data.VU;

    /* Y plane */
    for (row = 0; row < surface_out->Info.Height; row++) {
        fast_copy(y_dst, y_src, surface_out->Info.CropW);
        y_dst += surface_out->Data.Pitch;
        y_src += bmp_width_ / 3;
    }

    if (surface_out->Info.FourCC == MFX_FOURCC_NV12) { /* UV plane */
        for (row = 0; row < (surface_out->Info.Height / 2); row++) {
            fast_copy(uv_dst, uv_src, surface_out->Info.CropW);
            uv_dst += surface_out->Data.Pitch;
            uv_src += bmp_width_ / 3;
        }
    } else {
        printf("Surface Fourcc not supported!\n");
        return MFX_ERR_UNKNOWN;
    }
    return MFX_ERR_NONE;
}

mfxStatus StringDecPlugin::RelFreeType()
{
    FT_Done_Face(font_face_);
    font_face_ = NULL;
    FT_Done_FreeType(freetype_library_);
    freetype_library_ = NULL;
    return MFX_ERR_NONE;
}
#endif
