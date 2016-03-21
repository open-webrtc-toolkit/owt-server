/* <COPYRIGHT_TAG> */

#include "yuv_writer_plugin.h"

#ifdef ENABLE_RAW_CODEC
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "base/fast_copy.h"
#include "base/trace.h"
#include "general_allocator.h"

DEFINE_MLOGINSTANCE_CLASS(YUVWriterPlugin, "YUVWriterPlugin");
YUVWriterPlugin::YUVWriterPlugin()
{
    memset(&m_mfxPluginParam, 0, sizeof(m_mfxPluginParam));
    m_mfxPluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_mfxPluginParam.MaxThreadNum = 1;

    m_MaxNumTasks = 0;

    frame_buf_ = NULL;
    swap_buf_ = NULL;
}

mfxStatus YUVWriterPlugin::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    if (in && par) {
        in->Info = par->mfx.FrameInfo;
        in->NumFrameMin = 1;
        if (m_bIsOpaque) {
            in->Type = MFX_MEMTYPE_OPAQUE_FRAME + MFX_MEMTYPE_FROM_ENCODE;
        } else {
            in->Type = MFX_MEMTYPE_EXTERNAL_FRAME + MFX_MEMTYPE_FROM_ENCODE;
        }
        in->NumFrameSuggested = in->NumFrameMin + par->AsyncDepth;
    } else {
        FRMW_TRACE_ERROR("Alloc request parameter is NULL");
        return MFX_ERR_NULL_PTR;
    }
    return MFX_ERR_NONE;
}

mfxStatus YUVWriterPlugin::Init(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = NULL;
    int i = 0;

    if (!par) {
        FRMW_TRACE_ERROR("Input parameter is NULL");
        return MFX_ERR_NULL_PTR;
    } else {
        m_VideoParam = par;
    }

    m_bIsOpaque = m_VideoParam->IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY ? true : false;

    m_MaxNumTasks = m_VideoParam->AsyncDepth + 1;
    FRMW_TRACE_DEBUG("Max num task is %d", m_MaxNumTasks);

    if (m_bIsOpaque) {
        while (!pluginOpaqueAlloc && (i < m_VideoParam->NumExtParam)) {
            pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(m_VideoParam->ExtParam,\
                    m_VideoParam->NumExtParam,\
                    MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
            i++;
        }
        if (pluginOpaqueAlloc != NULL) {
            if (pluginOpaqueAlloc->In.Surfaces == NULL) {
                assert(0);
                return MFX_ERR_NULL_PTR;
            } else {
                sts = m_pmfxCore->MapOpaqueSurface(m_pmfxCore->pthis,
                        pluginOpaqueAlloc->In.NumSurface,
                        pluginOpaqueAlloc->In.Type,
                        pluginOpaqueAlloc->In.Surfaces);
                assert(MFX_ERR_NONE == sts);
            }
        } else {
            FRMW_TRACE_ERROR("Opaque alloc is NULL");
            return MFX_ERR_NULL_PTR;
        }
    }

    // Create internal task pool
    m_pTasks = new MFXTask_YUV_WRITER[m_MaxNumTasks];
    memset(m_pTasks, 0, sizeof(MFXTask_YUV_WRITER) * m_MaxNumTasks);

    // check input parameter limits
    if (MFX_ERR_NONE != CheckParameters(par)) {
        FRMW_TRACE_ERROR("Input parameters have error");
        return MFX_ERR_UNKNOWN;
    }

    return MFX_ERR_NONE;
}

mfxStatus YUVWriterPlugin::Close()
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxExtOpaqueSurfaceAlloc *pluginOpaqueAlloc = NULL;
    int i = 0;

    if (m_bIsOpaque) {
        while (!pluginOpaqueAlloc && (i < m_VideoParam->NumExtParam)) {
            pluginOpaqueAlloc =
                (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(m_VideoParam->ExtParam,
                        m_VideoParam->NumExtParam,
                        MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
            i++;
        }
        if (pluginOpaqueAlloc != NULL) {
            sts = m_pmfxCore->UnmapOpaqueSurface(m_pmfxCore->pthis,
                    pluginOpaqueAlloc->In.NumSurface,
                    pluginOpaqueAlloc->In.Type,
                    pluginOpaqueAlloc->In.Surfaces);
            if (MFX_ERR_NONE != sts) {
                FRMW_TRACE_ERROR("Failed to umap opaque mem");
            }
            return sts;
        } else {
            FRMW_TRACE_WARNI("PTR is NULL");
            return MFX_ERR_NULL_PTR;
        }
    }

    if (m_pTasks) {
        delete [] m_pTasks;
        m_pTasks = NULL;
    }
    return sts;
}

mfxStatus YUVWriterPlugin::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_VideoParam->mfx.FrameInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV420) {
        par->mfx.BufferSizeInKB = ((m_VideoParam->mfx.FrameInfo.Width) *\
                                   (m_VideoParam->mfx.FrameInfo.Height) * 3 / 2 +\
                                   (m_VideoParam->mfx.FrameInfo.Width) *\
                                   (m_VideoParam->mfx.FrameInfo.Height) * 3 % 2\
                                  ) / 1000 + 1;
    }

    return sts;
}

YUVWriterPlugin::~YUVWriterPlugin()
{
    if (frame_buf_) {
        delete frame_buf_;
        frame_buf_ = NULL;
    }

    if (swap_buf_) {
        free(swap_buf_);
        swap_buf_ = NULL;
    }
}

mfxExtBuffer *YUVWriterPlugin::GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
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

mfxStatus YUVWriterPlugin::PluginInit(mfxCoreInterface *core)
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pmfxCore = new mfxCoreInterface;
    if(!m_pmfxCore) {
        return MFX_ERR_MEMORY_ALLOC;
    }
    *m_pmfxCore = *core;
    m_pAlloc = &m_pmfxCore->FrameAllocator;

    return sts;
}

mfxStatus YUVWriterPlugin::PluginClose()
{
    mfxStatus sts = MFX_ERR_NONE;

    if(m_pmfxCore) {
        delete m_pmfxCore;
        m_pmfxCore = NULL;
    }

    return sts;
}

mfxStatus YUVWriterPlugin::GetPluginParam(mfxPluginParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    *par = m_mfxPluginParam;

    return sts;
}

mfxStatus YUVWriterPlugin::Submit(const mfxHDL *in, mfxU32 , const mfxHDL *out, mfxU32 , mfxThreadTask *task)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxBitstream *bitstream_out = NULL;
    mfxFrameSurface1 *opaque_surface_in = NULL;
    mfxFrameSurface1 *real_surf_in = NULL;

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
    MFXTask_YUV_WRITER *pTask = &m_pTasks[taskIdx];

    bitstream_out = (mfxBitstream *)out;
    opaque_surface_in = (mfxFrameSurface1 *)in[0];
    if (!bitstream_out || !opaque_surface_in) {
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

    m_BS = bitstream_out;
    pTask->bitstreamOut = bitstream_out;
    pTask->surfaceIn = real_surf_in;
    pTask->bBusy        = true;

    *task = (mfxThreadTask)pTask;
    m_pmfxCore->IncreaseReference(m_pmfxCore->pthis, &pTask->surfaceIn->Data);

    return sts;
}

mfxStatus YUVWriterPlugin::Execute(mfxThreadTask task, mfxU32 , mfxU32 uid_a)
{
    MFXTask_YUV_WRITER *pTask = (MFXTask_YUV_WRITER *)task;
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1 *surf_in = pTask->surfaceIn;
    sts = m_pAlloc->Lock(m_pAlloc->pthis, surf_in->Data.MemId, &surf_in->Data);
    if (sts != MFX_ERR_NONE) {
        FRMW_TRACE_ERROR("Lock return %d", sts);
    }

    /* get input frame */
    sts = GetInputYUV(surf_in);
    if (MFX_ERR_NONE != sts) {
        FRMW_TRACE_ERROR("Get yuv failed");
    }

    sts = m_pAlloc->Unlock(m_pAlloc->pthis, surf_in->Data.MemId, &surf_in->Data);
    if (sts != MFX_ERR_NONE) {
        FRMW_TRACE_ERROR("Unlock return %d", sts);
    }

    return MFX_TASK_DONE;
}

mfxStatus YUVWriterPlugin::FreeResources(mfxThreadTask task, mfxStatus )
{
    MFXTask_YUV_WRITER *pTask = (MFXTask_YUV_WRITER *)task;

    m_pmfxCore->DecreaseReference(m_pmfxCore->pthis, &pTask->surfaceIn->Data);
    //m_pmfxCore->DecreaseReference(m_pmfxCore->pthis, &pTask->surfaceOut->Data);

    pTask->bBusy = false;

    return MFX_ERR_NONE;
}

mfxStatus YUVWriterPlugin::SetAuxParams(void *auxParam, int auxParamSize)
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}

void YUVWriterPlugin::Release()
{
    return;
}

mfxStatus YUVWriterPlugin::GetInputYUV(mfxFrameSurface1 *surf)
{
    int buff_size = m_width_ * m_height_ * 3 / 2;

    if (frame_buf_ == NULL) {
        frame_buf_ = new unsigned char[buff_size];
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

    fast_copy(m_BS->Data + m_BS->DataLength, frame_buf_, buff_size);
    m_BS->DataLength += buff_size;

    return MFX_ERR_NONE;
}

mfxStatus YUVWriterPlugin::CheckParameters(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!par) {
        FRMW_TRACE_ERROR("Invalid input parameter");
        sts = MFX_ERR_NULL_PTR;
    }
    mfxInfoMFX *opt = &(par->mfx);
    m_width_ = opt->FrameInfo.CropW;
    m_height_ = opt->FrameInfo.CropH;

    return sts;
}


void YUVWriterPlugin::SetAllocPoint(MFXFrameAllocator *pMFXAllocator)
{
    if(pMFXAllocator && m_bIsOpaque) {
        m_pAlloc = pMFXAllocator;
    } else {
        m_pAlloc = &m_pmfxCore->FrameAllocator;
    }
}
#endif

