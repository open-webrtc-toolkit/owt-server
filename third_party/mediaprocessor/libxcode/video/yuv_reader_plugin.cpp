/* <COPYRIGHT_TAG> */

#if defined ENABLE_RAW_DECODE

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "base/fast_copy.h"
#include "base/trace.h"
#include "general_allocator.h"
#include "yuv_reader_plugin.h"

#define MSDK_PRINT_RET_MSG(ERR) { \
    MLOG_ERROR("\nReturn on error: error code %d,\t%s\t%d\n\n", ERR, __FILE__, __LINE__); \
}
#define MSDK_CHECK_ERROR(P, X, ERR)              {if ((X) == (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_POINTER(P, ERR)               {if (!(P)) {return ERR;}}
#define MSDK_MIN(A, B)                           (((A) < (B)) ? (A) : (B))

DEFINE_MLOGINSTANCE_CLASS(YUVReaderPlugin, "YUVReaderPlugin");
YUVReaderPlugin::YUVReaderPlugin()
{
    memset(&m_mfxPluginParam, 0, sizeof(m_mfxPluginParam));
    m_mfxPluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_mfxPluginParam.MaxThreadNum = 1;

    m_bInited = false;
    m_bIsOpaque = false;
    m_bIsMultiView = false;
    m_fSource = NULL;
    m_fSourceMVC = NULL;
    m_numLoadedFiles = 0;
    m_ColorFormat = MFX_FOURCC_YV12;
    raw_width_ = 0;
    raw_height_ = 0;
}

YUVReaderPlugin::~YUVReaderPlugin()
{
    Close();
}

mfxStatus YUVReaderPlugin::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    if (out && par) {
        out->Info = par->mfx.FrameInfo;
        out->NumFrameMin = 1;
            out->Type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET + MFX_MEMTYPE_OPAQUE_FRAME + MFX_MEMTYPE_FROM_DECODE;
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

mfxStatus YUVReaderPlugin::Init(const char *strFileName, const mfxU32 ColorFormat, const mfxU32 numViews, std::vector<char*> srcFileBuff)
{
    MSDK_CHECK_POINTER(strFileName, MFX_ERR_NULL_PTR);
    MSDK_CHECK_ERROR(strlen(strFileName), 0, MFX_ERR_NULL_PTR);

    Close();

    //open source YUV file
    if (!m_bIsMultiView) {
#ifdef _WIN32
        _tfopen_s(&m_fSource, strFileName, _T("rb"));
#else
        m_fSource = fopen(strFileName, "rb");
#endif
        if(NULL == m_fSource) {
            MLOG_ERROR(" open file %s error\n", strFileName);
            return MFX_ERR_NULL_PTR;
        }
        ++m_numLoadedFiles;
    } else if (m_bIsMultiView) {
        if (srcFileBuff.size() > 1) {
            if (srcFileBuff.size() != numViews)
                return MFX_ERR_UNSUPPORTED;

            mfxU32 i;
            m_fSourceMVC = new FILE*[numViews];
            for (i = 0; i < numViews; ++i) {
#ifdef _WIN32
                _tfopen_s(&m_fSourceMVC[i], srcFileBuff.at(i), _T("rb"));
#else
                m_fSourceMVC[i] = fopen(srcFileBuff.at(i), "rb");
#endif
                MSDK_CHECK_POINTER(m_fSourceMVC[i], MFX_ERR_NULL_PTR);
                ++m_numLoadedFiles;
            }
        } else {
            return MFX_ERR_UNSUPPORTED;
        }
    }

    //set init state to true in case of success
    m_bInited = true;

    if (ColorFormat == MFX_FOURCC_NV12 || 
            ColorFormat == MFX_FOURCC_YV12 || 
            ColorFormat == MFX_FOURCC_RGB4) {
        m_ColorFormat = ColorFormat;
    } else {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}
mfxStatus YUVReaderPlugin::Init(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    assert(par != NULL);
    m_VideoParam = par;

    if (m_bIsOpaque) {
        m_ExtParam = *m_VideoParam->ExtParam;
        mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(&m_ExtParam, m_VideoParam->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

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

    m_MaxNumTasks = m_VideoParam->AsyncDepth + 1;
    
    // Create internal task pool
    m_pTasks = new MFXTask[m_MaxNumTasks];
    memset(m_pTasks, 0, sizeof(MFXTask) * m_MaxNumTasks);

    return sts;
}

mfxStatus YUVReaderPlugin::Close()
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_fSource) {
        fclose(m_fSource);
        m_fSource = NULL;
    }

    if (m_fSourceMVC) {
        mfxU32 i = 0;
        for (i = 0; i < m_numLoadedFiles; ++i) {
            if  (m_fSourceMVC[i] != NULL) {
                fclose(m_fSourceMVC[i]);
                m_fSourceMVC[i] = NULL;
            }
        }
    }

    m_numLoadedFiles = 0;
    m_bInited = false;
    return sts;
}

mfxStatus YUVReaderPlugin::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}


mfxExtBuffer *YUVReaderPlugin::GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
{
    return 0;
}

mfxStatus YUVReaderPlugin::PluginInit(mfxCoreInterface *core)
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

mfxStatus YUVReaderPlugin::PluginClose()
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_pmfxCore) {
        delete m_pmfxCore;
        m_pmfxCore = NULL;
    }
    return sts;
}

mfxStatus YUVReaderPlugin::GetPluginParam(mfxPluginParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    *par = m_mfxPluginParam;
    return sts;
}

mfxStatus YUVReaderPlugin::Submit(const mfxHDL *in, mfxU32 , const mfxHDL *out, mfxU32 , mfxThreadTask *task)
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

    MFXTask *pTask = &m_pTasks[taskIdx];
    mfxFrameSurface1 *opaque_surface_out = (mfxFrameSurface1 *)out[0];
    mfxFrameSurface1 *real_surf_out = NULL;

    if (m_bIsOpaque) {
        sts = m_pmfxCore->GetRealSurface(m_pmfxCore->pthis, opaque_surface_out, &real_surf_out);
        assert(sts == MFX_ERR_NONE);
    } else {
        real_surf_out = opaque_surface_out;
    }

    pTask->rawInfo    = raw_info_;
    pTask->surfaceOut = real_surf_out;
    pTask->bBusy      = true;
    *task = (mfxThreadTask)pTask;
    m_pmfxCore->IncreaseReference(m_pmfxCore->pthis, &pTask->surfaceOut->Data);
    return sts;
}

mfxStatus YUVReaderPlugin::Execute(mfxThreadTask task, mfxU32 , mfxU32 uid_a)
{
    MFXTask *pTask = (MFXTask *)task;
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *surf_out = pTask->surfaceOut;

    m_pAlloc->Lock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
    sts = LoadNextFrame(surf_out);
    if (sts != MFX_ERR_NONE && MFX_ERR_MORE_DATA != sts) {
        MLOG_ERROR("Failed to convert the bmp to NV12 frame. %d\n", sts);
        m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
        return sts;
    }

    m_pAlloc->Unlock(m_pAlloc->pthis, surf_out->Data.MemId, &surf_out->Data);
    return MFX_TASK_DONE;
}

mfxStatus YUVReaderPlugin::FreeResources(mfxThreadTask task, mfxStatus )
{
    MFXTask *pTask = (MFXTask *)task;
    m_pmfxCore->DecreaseReference(m_pmfxCore->pthis, &pTask->surfaceOut->Data);
    pTask->bBusy = false;
    return MFX_ERR_NONE;
}

mfxStatus YUVReaderPlugin::SetAuxParams(void *auxParam, int auxParamSize)
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}

void YUVReaderPlugin::Release()
{
    return;
}

void YUVReaderPlugin::SetAllocPoint(MFXFrameAllocator *pMFXAllocator)
{
    if(pMFXAllocator && m_bIsOpaque) {
        m_pAlloc = pMFXAllocator;
    } else {
        m_pAlloc = &m_pmfxCore->FrameAllocator;
    }
}

mfxStatus YUVReaderPlugin::LoadNextFrame(mfxFrameSurface1* pSurface)
{
    // check if reader is initialized
    MSDK_CHECK_ERROR(m_bInited, false, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(pSurface, MFX_ERR_NULL_PTR);
    mfxU32 nBytesRead;
    mfxU16 w, h, i, pitch;
    mfxU8 *ptr, *ptr2;
    mfxFrameInfo* pInfo = &pSurface->Info;
    mfxFrameData* pData = &pSurface->Data;

    mfxU32 vid = pSurface->Info.FrameId.ViewId;

    // this reader supports only NV12 mfx surfaces for code transparency,
    // other formats may be added if application requires such functionality
    if (MFX_FOURCC_NV12 != pInfo->FourCC && 
            MFX_FOURCC_YV12 != pInfo->FourCC && 
            MFX_FOURCC_RGB4 != pInfo->FourCC) {
        return MFX_ERR_UNSUPPORTED;
    }

    if (pInfo->CropH > 0 && pInfo->CropW > 0) {
        w = pInfo->CropW;
        h = pInfo->CropH;
    } else {
        w = pInfo->Width;
        h = pInfo->Height;
    }

    pitch = pData->Pitch;

    if (MFX_FOURCC_NV12 == pInfo->FourCC || MFX_FOURCC_YV12 == pInfo->FourCC) {
        ptr = pData->Y + pInfo->CropX + pInfo->CropY * pData->Pitch;

        // read luminance plane
        for(i = 0; i < h; i++) {
            if (!m_bIsMultiView) {
                nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSource);
            } else {
                nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSourceMVC[vid]);
            }
            if (w != nBytesRead) {
                return MFX_ERR_MORE_DATA;
            }
        }

        // read chroma planes
        switch (m_ColorFormat) {// color format of data in the input file
        case MFX_FOURCC_YV12: // YUV420 is implied
            switch (pInfo->FourCC) {
            case MFX_FOURCC_NV12:
                mfxU8 buf[2048]; // maximum supported chroma width for nv12
                mfxU32 j;
                w /= 2;
                h /= 2;
                ptr = pData->UV + pInfo->CropX + (pInfo->CropY / 2) * pitch;
                if (w > 2048) {
                    return MFX_ERR_UNSUPPORTED;
                }
                // load U
                for (i = 0; i < h; i++) {
                    if (!m_bIsMultiView) {
                        nBytesRead = (mfxU32)fread(buf, 1, w, m_fSource);
                    } else {
                        nBytesRead = (mfxU32)fread(buf, 1, w, m_fSourceMVC[vid]);
                    }
                    if (w != nBytesRead) {
                        return MFX_ERR_MORE_DATA;
                    }
                    for (j = 0; j < w; j++) {
                        ptr[i * pitch + j * 2] = buf[j];
                    }
                }
                // load V
                for (i = 0; i < h; i++) {
                    if (!m_bIsMultiView) {
                        nBytesRead = (mfxU32)fread(buf, 1, w, m_fSource);
                    } else {
                        nBytesRead = (mfxU32)fread(buf, 1, w, m_fSourceMVC[vid]);
                    }
                    if (w != nBytesRead) {
                        return MFX_ERR_MORE_DATA;
                    }
                    for (j = 0; j < w; j++) {
                        ptr[i * pitch + j * 2 + 1] = buf[j];
                    }
                }

                break;
            case MFX_FOURCC_YV12:
                w /= 2;
                h /= 2;
                pitch /= 2;

                ptr  = pData->U + (pInfo->CropX / 2) + (pInfo->CropY / 2) * pitch;
                ptr2 = pData->V + (pInfo->CropX / 2) + (pInfo->CropY / 2) * pitch;

                for(i = 0; i < h; i++) {
                    if (!m_bIsMultiView) {
                        nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSource);
                    } else {
                        nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSourceMVC[vid]);
                    }
                    if (w != nBytesRead) {
                        return MFX_ERR_MORE_DATA;
                    }
                }
                for(i = 0; i < h; i++) {
                    if (!m_bIsMultiView) {
                        nBytesRead = (mfxU32)fread(ptr2 + i * pitch, 1, w, m_fSource);
                    } else {
                        nBytesRead = (mfxU32)fread(ptr2 + i * pitch, 1, w, m_fSourceMVC[vid]);
                    }
                    if (w != nBytesRead) {
                        return MFX_ERR_MORE_DATA;
                    }
                }
                break;
            default:
                return MFX_ERR_UNSUPPORTED;
                }
            break;
        case MFX_FOURCC_NV12:
            h /= 2;
            ptr  = pData->UV + pInfo->CropX + (pInfo->CropY / 2) * pitch;
            for(i = 0; i < h; i++) {
                if (!m_bIsMultiView) {
                    nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSource);
                } else {
                    nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, w, m_fSourceMVC[vid]);
                }
                if (w != nBytesRead) {
                    return MFX_ERR_MORE_DATA;
                }
            }

            break;
        default:
            return MFX_ERR_UNSUPPORTED;
        }
    }
    else if (pInfo->FourCC == MFX_FOURCC_RGB4) {
        MSDK_CHECK_POINTER(pData->R, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->G, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_POINTER(pData->B, MFX_ERR_NOT_INITIALIZED);
        // there is issue with A channel in case of d3d, so A-ch is ignored
        //MSDK_CHECK_POINTER(pData->A, MFX_ERR_NOT_INITIALIZED);

        ptr = MSDK_MIN( MSDK_MIN(pData->R, pData->G), pData->B );
        ptr = ptr + pInfo->CropX + pInfo->CropY * pitch;

        for (i = 0; i < h; i++) {
            nBytesRead = (mfxU32)fread(ptr + i * pitch, 1, 4*w, m_fSource);
            if (4*w != nBytesRead) {
                return MFX_ERR_MORE_DATA;
            }
        }
     }

    return MFX_ERR_NONE;
}

mfxStatus YUVReaderPlugin::DecodeHeader(RawInfo *rawInfo, mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    assert(NULL != rawInfo);

    if (!m_bInited) {
        std::vector<char *> v1;
        sts = Init(rawInfo->raw_file, rawInfo->fourcc, 0, v1);
        if (MFX_ERR_NONE != sts) {
            MLOG_ERROR(" Init error %d\n", sts);
            return sts;
        }
    }

    raw_width_ = rawInfo->width;
    raw_height_ = rawInfo->height;

    par->mfx.FrameInfo.Width = raw_width_;
    par->mfx.FrameInfo.Height = raw_height_;
    par->mfx.FrameInfo.CropW = rawInfo->cropw;
    par->mfx.FrameInfo.CropH = rawInfo->croph;
    par->mfx.FrameInfo.FourCC = m_ColorFormat ;
    par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    
    m_bIsOpaque = par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY ? true : false;

    return sts;
}

#endif //#if defined ENABLE_RAW_DECODE
