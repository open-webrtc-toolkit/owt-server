/* <COPYRIGHT_TAG> */

#include "va_plugin.h"

#ifdef ENABLE_VA

#include <stdio.h>
#include <sys/time.h>
#include "findplate.h"
#include "base/logger.h"

// disable "unreferenced formal parameter" warning -
// not all formal parameters of interface functions will be used by sample plugin
#pragma warning(disable : 4100)

#define SWAP_BYTES(a, b) {mfxU8 tmp; tmp = a; a = b; b = tmp;}

DEFINE_MLOGINSTANCE("VAplugin");
mfxI64 msdk_time_get_tick(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (mfxI64)tv.tv_sec * (mfxI64)1000000 + (mfxI64)tv.tv_usec;
}

/* Analyze class implementation */
Analyze::Analyze() :
m_pTasks(NULL),
m_bInited(false),
m_bIsInOpaque(false),
m_bIsOutOpaque(false),
m_device(NULL)
{
    m_MaxNumTasks = 0;

    memset(&m_VideoParam, 0, sizeof(m_VideoParam));
    memset(&m_Param, 0, sizeof(m_Param));

    memset(&m_PluginParam, 0, sizeof(m_PluginParam));
    m_PluginParam.MaxThreadNum = 1;
    m_PluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    Opencvanalyzer = NULL;
    memset(&m_Stat, 0, sizeof(m_Stat));
    m_Stat.StartSnap = msdk_time_get_tick()/1000;
}

Analyze::~Analyze()
{
    PluginClose();
    Close();
}

/* Methods required for integration with Media SDK */
mfxStatus Analyze::PluginInit(mfxCoreInterface *core)
{
    if (core == NULL) {
        MLOG_ERROR("invalid parameters for video analytics\n");
        return MFX_ERR_NULL_PTR;
    }
    mfxCoreParam core_param;
    mfxStatus sts = core->GetCoreParam(core->pthis, &core_param);
    if (sts != MFX_ERR_NONE) {
        MLOG_ERROR("GetCoreParam failed\n");
        return sts;
    }
    //only 1.8 or above version is supported
    if (core_param.Version.Version < 0x00010008) {
        MLOG_ERROR("\nERROR: MSDK v1.8 or above is required to use this plugin \n");
        return MFX_ERR_UNSUPPORTED;
    }

    sts = core->GetHandle(core->pthis, MFX_HANDLE_VA_DISPLAY, &m_device);
    if(sts != MFX_ERR_NONE) {
        MLOG_ERROR("Get the core handle failed\n");
    }
    m_mfxCore = MFXCoreInterface(*core);

    return MFX_ERR_NONE;
}

mfxStatus Analyze::PluginClose()
{
    if (Opencvanalyzer) {
        delete Opencvanalyzer;
        Opencvanalyzer = NULL;
    }
    return MFX_ERR_NONE;
}

mfxStatus Analyze::GetPluginParam(mfxPluginParam *par)
{
    if (par == NULL) {
        MLOG_ERROR("GetPluginParam get invalid params\n");
        return MFX_ERR_NULL_PTR;
    }

    *par = m_PluginParam;

    return MFX_ERR_NONE;
}

mfxStatus Analyze::Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task)
{
    if ((in == NULL) || (*in ==NULL) || (task == NULL)) {
        MLOG_ERROR("plugin submit invalid params\n");
        return MFX_ERR_NULL_PTR;
    }

    if (in_num != 1) {
        MLOG_ERROR("plugin in/out num is unsupported\n");
        return MFX_ERR_UNSUPPORTED;
    }

    if (m_bInited == false) {
        MLOG_ERROR("plugin was not initialized\n");
        return MFX_ERR_NOT_INITIALIZED;
    }

    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *surface_in = (mfxFrameSurface1 *)in[0];
    mfxFrameSurface1 *real_surface_in = surface_in;
    if (m_bIsInOpaque)
    {
        sts = m_mfxCore.GetRealSurface(surface_in, &real_surface_in);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("plugin memory allocate failed\n");
            return MFX_ERR_MEMORY_ALLOC;
        }
    }

    mfxFrameSurface1 *surface_out = NULL;
    mfxFrameSurface1 *real_surface_out = NULL;
    if (out) {
        surface_out = (mfxFrameSurface1 *)out[0];
        real_surface_out = surface_out;
        if (m_bIsOutOpaque)
        {
            sts = m_mfxCore.GetRealSurface(surface_out, &real_surface_out);
            if (sts != MFX_ERR_NONE) {
                MLOG_ERROR("plugin memory allocate failed\n");
                return MFX_ERR_MEMORY_ALLOC;
            }
        }
    }

    // check validity of parameters
    sts = CheckInOutFrameInfo(&real_surface_in->Info, &real_surface_out->Info);
    if (sts != MFX_ERR_NONE) {
        MLOG_ERROR("plugin get in/out frame information failed\n");
        return sts;
    }

    mfxU32 ind = FindFreeTaskIdx();

    if (ind >= m_MaxNumTasks)
    {
        MLOG_WARNING("no free task available here\n");
        return MFX_WRN_DEVICE_BUSY; // currently there are no free tasks available
    }

    m_mfxCore.IncreaseReference(&(real_surface_in->Data));
    if (real_surface_out) {
        m_mfxCore.IncreaseReference(&(real_surface_out->Data));
    }

    m_pTasks[ind].In = real_surface_in;
    if (real_surface_out) {
        m_pTasks[ind].Out = real_surface_out;
    }
    m_pTasks[ind].bBusy = true;

    mfxExtBuffer **ext_buf = surface_in->Data.ExtParam;
    VAPluginData* plugin_data = (VAPluginData*)(*ext_buf);
    m_pTasks[ind].OutMeta = plugin_data;
    if (m_Param.bOpenCLEnable) {
        m_pTasks[ind].pProcessor = new Analyzer(m_Param.bDump, m_Param.VaInterval, m_Param.VaType, &m_Stat, &m_OpenCLFilter);
    } else {
        m_pTasks[ind].pProcessor = new Analyzer(m_Param.bDump, m_Param.VaInterval, m_Param.VaType, &m_Stat, NULL);
    }
    if (m_pTasks[ind].pProcessor == NULL) {
        MLOG_ERROR("processor allocated failed\n");
        return MFX_ERR_MEMORY_ALLOC;
    }

    if (Opencvanalyzer == NULL) {
        Opencvanalyzer = new OpencvVideoAnalyzer(m_Param.bDump);
    }

    m_pTasks[ind].pProcessor->SetAnalyzer(Opencvanalyzer);

    m_pTasks[ind].pProcessor->SetAllocator(&m_mfxCore.FrameAllocator());
    m_pTasks[ind].pProcessor->Init(real_surface_in, real_surface_out);

    *task = (mfxThreadTask)&m_pTasks[ind];

    return MFX_ERR_NONE;
}

mfxStatus Analyze::Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a)
{
    if (m_bInited == false) {
        return MFX_ERR_NOT_INITIALIZED;
    }

    mfxStatus sts = MFX_ERR_NONE;
    AnalyzeTask *current_task = (AnalyzeTask *)task;

    // 0,...,NumChunks - 2 calls return TASK_WORKING, NumChunks - 1,.. return TASK_DONE
    if (uid_a < m_NumChunks)
    {
        // there's data to process
        sts = current_task->pProcessor->Process(current_task->OutMeta);
        sts = ((m_NumChunks - 1) == uid_a) ? MFX_TASK_DONE : MFX_TASK_WORKING;
        return sts;
        // last call?
    }
    else
    {
        // no data to process
        sts =  MFX_TASK_DONE;
    }
    return sts;
}

mfxStatus Analyze::FreeResources(mfxThreadTask task, mfxStatus sts)
{
    AnalyzeTask *current_task = (AnalyzeTask *)task;

    m_mfxCore.DecreaseReference(&(current_task->In->Data));
    if (current_task->Out) {
        m_mfxCore.DecreaseReference(&(current_task->Out->Data));
    }

    if (current_task->pProcessor) {
        delete current_task->pProcessor;
        current_task->pProcessor = NULL;
    }
    current_task->bBusy = false;

    return MFX_ERR_NONE;
}

mfxStatus Analyze::Init(mfxVideoParam *mfxParam)
{
    if (mfxParam == NULL) {
        MLOG_ERROR("plugin init invalid parameters\n");
        return MFX_ERR_NULL_PTR;
    }

    mfxStatus sts = MFX_ERR_NONE;
    memcpy(&m_VideoParam, mfxParam, sizeof(mfxVideoParam));

    // map opaque surfaces array in case of opaque surfaces
    m_bIsInOpaque = (m_VideoParam.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY) ? true : false;
    mfxExtOpaqueSurfaceAlloc* pluginOpaqueAlloc = NULL;

    if (m_bIsInOpaque)
    {
        pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer(m_VideoParam.ExtParam,
            m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (pluginOpaqueAlloc == NULL) {
            MLOG_ERROR("init the opaque allocator invalid, sts is %d\n", sts);
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }
    }

    // check existence of corresponding allocs
    if (m_bIsInOpaque && ! pluginOpaqueAlloc->In.Surfaces)
       return MFX_ERR_INVALID_VIDEO_PARAM;

    if (m_bIsInOpaque)
    {
        sts = m_mfxCore.MapOpaqueSurface(pluginOpaqueAlloc->In.NumSurface,
            pluginOpaqueAlloc->In.Type, pluginOpaqueAlloc->In.Surfaces);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("init map input opaque surface failed, sts is %d\n", sts);
            return MFX_ERR_MEMORY_ALLOC;
        }
    }

    m_MaxNumTasks = m_VideoParam.AsyncDepth;
    if (m_MaxNumTasks < 2) m_MaxNumTasks = 2;

    m_pTasks = new AnalyzeTask [m_MaxNumTasks];
    if (m_pTasks == NULL) {
        MLOG_ERROR("plugin task allocated failed\n");
        return MFX_ERR_MEMORY_ALLOC;
    }

    memset(m_pTasks, 0, sizeof(AnalyzeTask) * m_MaxNumTasks);

    m_NumChunks = m_PluginParam.MaxThreadNum;
    m_pChunks = new DataChunk [m_NumChunks];
    if (m_pChunks == NULL) {
        MLOG_ERROR("memory allocated failed\n");
        return MFX_ERR_MEMORY_ALLOC;
    }

    memset(m_pChunks, 0, sizeof(DataChunk) * m_NumChunks);

    // divide frame into data chunks
    mfxU32 num_lines_in_chunk = mfxParam->vpp.In.CropH / m_NumChunks; // integer division
    mfxU32 remainder_lines = mfxParam->vpp.In.CropH % m_NumChunks; // get remainder
    // remaining lines are distributed among first chunks (+ extra 1 line each)
    for (mfxU32 i = 0; i < m_NumChunks; i++)
    {
        m_pChunks[i].StartLine = (i == 0) ? 0 : m_pChunks[i-1].EndLine + 1;
        m_pChunks[i].EndLine = (i < remainder_lines) ? (i + 1) * num_lines_in_chunk : (i + 1) * num_lines_in_chunk - 1;
    }

    m_bInited = true;

    return MFX_ERR_NONE;
}

mfxStatus Analyze::SetAuxParams(void* auxParam, int auxParamSize)
{
    VAPluginParam *pAnalyzePar = (VAPluginParam *)auxParam;
    if (pAnalyzePar == NULL) {
        MLOG_ERROR("SetAuxParams invalid parameters\n");
        return MFX_ERR_NULL_PTR;
    }

    // check validity of parameters
    mfxStatus sts = CheckParam(&m_VideoParam, pAnalyzePar);
    if (sts != MFX_ERR_NONE) {
        MLOG_ERROR("parameters check failed\n");
        return sts;
    }

    m_Param = *pAnalyzePar;

    m_Stat.PerfTrace = m_Param.PerfTrace;

    if (m_Param.bOpenCLEnable == true) {
        // init OpenCLFilter
        cl_int error = CL_SUCCESS;
        char* snva_home;
        snva_home = getenv("SNVA_HOME");
        if ((snva_home == NULL) && (m_Param.KernelPath == NULL)){
            error = m_OpenCLFilter.AddKernel(readFile("ocl_cvtcolor.cl").c_str(),
                                         "yuv2rgb", "yuv2gray", MFX_FOURCC_NV12);
            if (error) {
                return MFX_ERR_DEVICE_FAILED;
            }
            error = m_OpenCLFilter.AddKernel(readFile("ocl_bgfg.cl").c_str(),
                                         "backgroundsubtract_abs_diff", "backgroundsubtract_gmm", MFX_FOURCC_NV12);
            if (error) {
                return MFX_ERR_DEVICE_FAILED;
            }
       } else {
            std::string cvt_path;
            std::string bgfg_path;
            if (snva_home) {
                cvt_path = snva_home;
                bgfg_path = snva_home;
            } else if (m_Param.KernelPath){
                cvt_path = m_Param.KernelPath;
                bgfg_path = m_Param.KernelPath;
            }
            cvt_path.append("/ocl_cvtcolor.cl");
            bgfg_path.append("/ocl_bgfg.cl");
            error = m_OpenCLFilter.AddKernel(readFile(cvt_path.c_str()).c_str(),
                                                 "yuv2rgb", "yuv2gray", MFX_FOURCC_NV12);
            if (error) {
                return MFX_ERR_DEVICE_FAILED;
            }
            error = m_OpenCLFilter.AddKernel(readFile(bgfg_path.c_str()).c_str(),
                                             "backgroundsubtract_abs_diff", "backgroundsubtract_gmm", MFX_FOURCC_NV12);
            if (error) return MFX_ERR_DEVICE_FAILED;
        }

        error = m_OpenCLFilter.OCLInit(m_device);
        if (error) {
            error = CL_SUCCESS;
            MLOG_WARNING("Initializing plugin with media sharing failed\n");
        } else {
            error = m_OpenCLFilter.SelectKernel(0);
            if (error) {
                return MFX_ERR_DEVICE_FAILED;
            }
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus Analyze::Close()
{
    if (!m_bInited)
        return MFX_ERR_NONE;
    memset(&m_Param, 0, sizeof(VAPluginParam));

    delete [] m_pTasks;
    m_pTasks = NULL;

    delete [] m_pChunks;
    m_pChunks = NULL;

    mfxStatus sts = MFX_ERR_NONE;

    mfxExtOpaqueSurfaceAlloc* pluginOpaqueAlloc = NULL;

    if ((m_bIsInOpaque) || (m_bIsOutOpaque))
    {
        pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)
            GetExtBuffer(m_VideoParam.ExtParam, m_VideoParam.NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (pluginOpaqueAlloc == NULL) {
            MLOG_ERROR("invalid opaque allocator\n");
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }
    }

    // check existence of corresponding allocs
    if (m_bIsInOpaque && ! pluginOpaqueAlloc->In.Surfaces)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (m_bIsOutOpaque && !pluginOpaqueAlloc->Out.Surfaces)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (m_bIsInOpaque)
    {
        sts = m_mfxCore.UnmapOpaqueSurface(pluginOpaqueAlloc->In.NumSurface,
            pluginOpaqueAlloc->In.Type, pluginOpaqueAlloc->In.Surfaces);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("close unmap input opaque surface failed\n");
            return MFX_ERR_MEMORY_ALLOC;
        }
    }

    if (m_bIsOutOpaque)
    {
        sts = m_mfxCore.UnmapOpaqueSurface(pluginOpaqueAlloc->Out.NumSurface,
            pluginOpaqueAlloc->Out.Type, pluginOpaqueAlloc->Out.Surfaces);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("close unmap input opaque surface failed\n");
            return MFX_ERR_MEMORY_ALLOC;
        }
    }
    m_bInited = false;

    return MFX_ERR_NONE;
}

mfxStatus Analyze::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    if ((par == NULL) || (in == NULL) || (out == NULL)) {
        MLOG_ERROR("QueryIOSurface invalid parameters\n");
        return MFX_ERR_NULL_PTR;
    }

    memcpy(&(in->Info), &(par->vpp.In), sizeof(par->vpp.In));
    in->NumFrameMin = 1;
    in->NumFrameSuggested = in->NumFrameMin + par->AsyncDepth;

    memcpy(&(out->Info), &(par->vpp.Out), sizeof(par->vpp.Out));
    out->NumFrameMin = 1;
    out->NumFrameSuggested = out->NumFrameMin + par->AsyncDepth;
    if (par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) {
        out->Type = MFX_MEMTYPE_OPAQUE_FRAME + MFX_MEMTYPE_FROM_VPPOUT;
    } else {
        out->Type = MFX_MEMTYPE_EXTERNAL_FRAME + MFX_MEMTYPE_FROM_VPPOUT + MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
    }
    return MFX_ERR_NONE;
}

/* Internal methods */
mfxU32 Analyze::FindFreeTaskIdx()
{
    mfxU32 i;
    for (i = 0; i < m_MaxNumTasks; i++)
    {
        if (false == m_pTasks[i].bBusy)
        {
            break;
        }
    }

    return i;
}

mfxStatus Analyze::CheckParam(mfxVideoParam *mfxParam, VAPluginParam *pAnalyzePar)
{
    if (mfxParam == NULL) {
        MLOG_ERROR("CheckParam invalid parameters\n");
        return MFX_ERR_NULL_PTR;
    }

    mfxInfoVPP *pParam = &mfxParam->vpp;


    // only NV12 color format is supported
    if (MFX_FOURCC_RGB4 != pParam->In.FourCC)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

mfxStatus Analyze::CheckInOutFrameInfo(mfxFrameInfo *pIn, mfxFrameInfo *pOut)
{
    //TODO: check the video analytic parameters
    return MFX_ERR_NONE;
}


// function for getting a pointer to a specific external buffer from the array
mfxExtBuffer* Analyze::GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
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

void Analyze::SetAllocPoint(MFXFrameAllocator *pMFXAllocator)
{
    if((m_bIsInOpaque)||(m_bIsOutOpaque)) {
        m_pAlloc = &(m_mfxCore.FrameAllocator());
    } else {
        m_pAlloc = pMFXAllocator;
    }
}

/* Processor class implementation */
Processor::Processor()
    : m_pIn(NULL)
    , m_pOut(NULL)
    , m_pAlloc(NULL)
{
}

Processor::~Processor()
{
}

bool Processor::SetAnalyzer(OpencvVideoAnalyzer* video_analyzer)
{
    videoanalyzer = video_analyzer;
    return true;
}

mfxStatus Processor::SetAllocator(mfxFrameAllocator *pAlloc)
{
    m_pAlloc = pAlloc;
    return MFX_ERR_NONE;
}

mfxStatus Processor::Init(mfxFrameSurface1 *frame_in, mfxFrameSurface1 *frame_out)
{
    if (frame_in == NULL) {
        MLOG_ERROR("Processor Init invalid parameters\n");
        return MFX_ERR_NULL_PTR;
    }

    m_pIn = frame_in;
    m_pOut = frame_out;

    return MFX_ERR_NONE;
}

mfxStatus Processor::LockFrame(mfxFrameSurface1 *frame)
{
    if (frame == NULL) {
        MLOG_ERROR("process lock frame invalid parameters\n");
        return MFX_ERR_NULL_PTR;
    }
    //double lock impossible
    if (frame->Data.Y != 0 && frame->Data.MemId !=0)
        return MFX_ERR_UNSUPPORTED;
    //no allocator used, no need to do lock
    if (frame->Data.Y != 0)
        return MFX_ERR_NONE;
    //lock required
    if (m_pAlloc == NULL) {
        MLOG_ERROR("process lock frame invalid alloc\n");
        return MFX_ERR_NULL_PTR;
    }

    return m_pAlloc->Lock(m_pAlloc->pthis, frame->Data.MemId, &frame->Data);
}

mfxStatus Processor::UnlockFrame(mfxFrameSurface1 *frame)
{
    if (frame == NULL) {
        MLOG_ERROR("process unlock frame invalid parameters\n");
        return MFX_ERR_NULL_PTR;
    }
    //unlock not possible, no allocator used
    if (frame->Data.Y != 0 && frame->Data.MemId ==0)
        return MFX_ERR_NONE;
    //already unlocked
    if (frame->Data.Y == 0)
        return MFX_ERR_NONE;
    //unlock required
    if (m_pAlloc == NULL) {
        MLOG_ERROR("process unlock frame invalid alloc\n");
        return MFX_ERR_NULL_PTR;
    }
    return m_pAlloc->Unlock(m_pAlloc->pthis, frame->Data.MemId, &frame->Data);
}


/* video analytics class implementation */
Analyzer::Analyzer() : Processor()
{
    bDump = false;
    VaInterval = 1;
    VaType = VA_ROI;
}


Analyzer::Analyzer(bool dump, int interval, VAType type, Statistics* stat, OpenCLFilter *pOpenCLFilter) : Processor()
{
    bDump = dump;
    VaInterval = interval;
    VaType = type;
    m_pOpenCLFilter = pOpenCLFilter;
    Stat = stat;
    Stat->nFrameIndex++;
    FrameCnt = Stat->nFrameIndex;
}

mfxStatus Analyzer::SetAllocator(mfxFrameAllocator* pAlloc)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_pOpenCLFilter) {
        sts = m_pOpenCLFilter->SetAllocator(pAlloc);
    }
    if (MFX_ERR_NONE == sts) {
        sts = Processor::SetAllocator(pAlloc);
    }
    return sts;
}

Analyzer::~Analyzer()
{
}

mfxStatus Analyzer::Process(VAPluginData* data)
{
    mfxI64 c1, c2;
    float average;

    if (data == NULL) {
        MLOG_ERROR("processor prcoess invalid parameter\n");
        return MFX_ERR_NULL_PTR;
    }

    mfxStatus sts = MFX_ERR_NONE;
    if (MFX_ERR_NONE != (sts = LockFrame(m_pIn)))
    {
        return sts;
    }

    if (m_pOut) {
        if (MFX_ERR_NONE != (sts = LockFrame(m_pOut)))
        {
            return sts;
        }
    }

    cv::Mat BgFrame(m_pIn->Info.Height, m_pIn->Info.Width, CV_8UC1, cv::Scalar(255));
    if (m_pOpenCLFilter == NULL) {
        cv::ocl::setUseOpenCL(false);
        cv::Mat* pFrame = new cv::Mat(m_pIn->Info.Height, m_pIn->Info.Width, CV_8UC3, cv::Scalar(255,255,255));
        cv::Mat* pGrayFrame = new cv::Mat(m_pIn->Info.Height, m_pIn->Info.Width, CV_8UC1, cv::Scalar(255));
        cv::Mat* pForeFrame = new cv::Mat(m_pIn->Info.Height, m_pIn->Info.Width, CV_8UC1, cv::Scalar(255));

        c1 = msdk_time_get_tick();
        videoanalyzer->YUV2RGB(m_pIn, *pFrame);
        videoanalyzer->YUV2Gray(m_pIn, *pGrayFrame);
        c2 = msdk_time_get_tick();

        Stat->PrepTime = (c2-c1)/1000 + Stat->PrepTime;
        average = ((float)(Stat->PrepTime))/FrameCnt;
        if (Stat->PerfTrace)
            MLOG_INFO("Pre-analyzing(CPU) for %d frame consume %0.1f ms per frame\n", FrameCnt, average);
        sts = UnlockFrame(m_pIn);
        if (sts != MFX_ERR_NONE) {
            delete pFrame;
            delete pGrayFrame;
            delete pForeFrame;
            MLOG_ERROR("processor lock input frame failed\n");
            return sts;
        }
        if (FrameCnt == 0) {
            BgFrame = *pGrayFrame;
            *pForeFrame = *pGrayFrame;
        } else {
            videoanalyzer->GrayBackgroundSubstraction(*pGrayFrame, BgFrame, *pForeFrame);
        }

        ProcessFrame(*pForeFrame, *pFrame, data);
        delete pFrame;
        delete pGrayFrame;
        delete pForeFrame;

        if (m_pOut) {
            sts = UnlockFrame(m_pOut);
            if (sts != MFX_ERR_NONE) {
                MLOG_ERROR("processor lock output frame failed\n");
                return sts;
            }
       }
    } else {
        unsigned char* pImage;
        unsigned char* pGrayImage;

        cv::ocl::setUseOpenCL(true);
        c1 = msdk_time_get_tick();
        m_pOpenCLFilter->ProcessSurface(m_pIn->Info.Width, m_pIn->Info.Height, m_pIn->Data.MemId, NULL);
        c2 = msdk_time_get_tick();

        Stat->PrepTime = (c2-c1)/1000 + Stat->PrepTime;
        average = ((float)(Stat->PrepTime))/FrameCnt;
        if (Stat->PerfTrace)
            MLOG_INFO("Pre-analyzing(GPU) for %d frame consume %0.1f ms per frame\n", FrameCnt, average);
        sts = UnlockFrame(m_pIn);
        m_pOpenCLFilter->MapImage();
        pImage = m_pOpenCLFilter->GetImageData();
        pGrayImage = m_pOpenCLFilter->GetGrayImageData();

        //process the image here.
        cv::Mat frame(m_pIn->Info.Height, m_pIn->Info.Width, CV_8UC3, pImage);
        cv::Mat grayframe(m_pIn->Info.Height, m_pIn->Info.Width, CV_8UC1, pGrayImage);

        ProcessFrame(grayframe, frame, data);
        m_pOpenCLFilter->UnmapImage();
        if (m_pOut) {
            sts = UnlockFrame(m_pOut);
        }
    }

    Stat->EndSnap = msdk_time_get_tick()/1000;
    mfxU64 consume = Stat->EndSnap-Stat->StartSnap;
    float fps = ((float)(FrameCnt * 1000))/consume;
    average = ((float)(consume))/FrameCnt;
    if (Stat->PerfTrace)
        MLOG_INFO("Decode&Analyze frame consume %.1f ms per frame, fps:%.2f\n", average, fps);
    return sts;
}


void Analyzer::ProcessFrame(cv::Mat &frame,cv::Mat &showframe, VAPluginData* data)
{
    mfxI64 c1, c2;

    if (VaInterval < 0) {
        return;
    }

    char plate[6] = {0};
    bool PlateFound = false;
    std::vector<cv::Rect> rect_out;
    int num_objects = 0;
    if (VaInterval == 0 || FrameCnt % VaInterval == 0) {
        FrameMeta* in = data->In;
        if (VaType == VA_ROI) {
            c1 = msdk_time_get_tick();
            num_objects = videoanalyzer->ROIDetection(showframe, rect_out);
            c2 = msdk_time_get_tick();
            videoanalyzer->MarkImage(showframe, rect_out, false);
        } else if (VaType == VA_FACE) {
            c1 = msdk_time_get_tick();
            num_objects = videoanalyzer->FaceDetection(showframe, rect_out);
            c2 = msdk_time_get_tick();
            videoanalyzer->MarkImage(showframe, rect_out, true);
        } else if (VaType == VA_PEOPLE){
            c1 = msdk_time_get_tick();
            num_objects = videoanalyzer->PeopleDetection(frame, rect_out);
            c2 = msdk_time_get_tick();
            videoanalyzer->MarkImage(showframe, rect_out, false);
        } else if (VaType == VA_PLATE) {
            c1 = msdk_time_get_tick();
            num_objects = 0;
            if (in) {
                if (in->NumOfROI > 0) {
                    LIST_ROI::iterator i;
                    for (i = in->ROIList.begin(); i != in->ROIList.end(); i++) {
                        int x = (*i)->tl_x;
                        int y = (*i)->tl_y;
                        int width = (*i)->br_x - (*i)->tl_x;
                        int height = (*i)->br_y - (*i)->tl_y;
                        //FIXME: Change the video ROI information acording to frame resolution.
                        x = (int)(x * 1920 / 640);
                        y = (int)(y * 1080 / 480);
                        width = (int)(width * 1920 /640);
                        height = (int)(height * 1080 / 480);
                        cv::Rect in_rect(x, y, width, height);
                        PlateFound = plate_recog(showframe, in_rect, plate);
                    }
                }
            }else {
                // roi -> car -> plate
                std::vector<cv::Rect> roiRect;
                std::vector<cv::Rect> carRect;
                videoanalyzer->ROIDetection(showframe, roiRect);
                for (int i = 0; i < roiRect.size(); i++) {
                    videoanalyzer->CarDetection(showframe, roiRect[i], carRect);

                    for (int i = 0; i < carRect.size(); i++) {
                        PlateFound = plate_recog(showframe, carRect[i], plate);
                        if(PlateFound) break;
                    }
                    if(PlateFound) break;
                }
                roiRect.clear();
                carRect.clear();
            }
            c2 = msdk_time_get_tick();
        } else if (VaType == VA_CAR) {
            c1 = msdk_time_get_tick();
            if (in) {
                if (in->NumOfROI > 0) {
                    LIST_ROI::iterator i;
                    for (i = in->ROIList.begin(); i != in->ROIList.end(); i++) {
                        int x = (*i)->tl_x;
                        int y = (*i)->tl_y;
                        int width = (*i)->br_x - (*i)->tl_x;
                        int height = (*i)->br_y - (*i)->tl_y;
                        cv::Rect in_rect(x, y, width, height);
                        videoanalyzer->CarDetection(showframe, in_rect, rect_out);
                    }
                    num_objects = rect_out.size();
                }
            }else {
                // roi -> car
                std::vector<cv::Rect> roiRect;
                videoanalyzer->ROIDetection(showframe, roiRect);
                for (int i = 0; i < roiRect.size(); i++) {
                    videoanalyzer->CarDetection(showframe, roiRect[i], rect_out);
                    if(rect_out.size() > 0) break;
                }
                num_objects = rect_out.size();
                roiRect.clear();
                videoanalyzer->MarkImage(showframe, rect_out, false);
            }
            c2 = msdk_time_get_tick();
        }

        float average;
        Stat->VATime = Stat->VATime + (c2-c1)/1000;
        average = ((float)(Stat->VATime))/FrameCnt;
        if (Stat->PerfTrace)
            MLOG_INFO("Analyzer found %d condidates, average consume %.1f ms per frame\n", num_objects, average);
        FrameMeta* out = data->Out;
        if (out) {
            if (in) {
                out->VideoId = in->VideoId;
                out->FrameId = in->FrameId;
            } else {
                out->VideoId = 0;
                out->FrameId = FrameCnt;
            }

            if (VaType == VA_PLATE) {
                out->FeatureType = 1;
                if (PlateFound) {
                    out->DescriptorLen = 6;
                    unsigned char* des = new unsigned char[6];
                    memcpy(des, plate, 6);
                    out->Descriptor = des;
                } else {
                    // If do not set Descriptor to NULL,
                    // it may be crash in msdk_codec.cpp when delete out->Descriptor.
                    out->Descriptor = NULL;
                    out->DescriptorLen = 0;
                }
            } else {
                out->FeatureType = 0;
                out->NumOfROI = rect_out.size();
                for (int i = 0; i < rect_out.size(); i++) {
                    if (Stat->PerfTrace)
                        MLOG_DEBUG("tl %d:%d : br %d:%d\n", rect_out[i].tl().x, rect_out[i].tl().y,
                                                        rect_out[i].br().x, rect_out[i].br().y);
                    ROI* roi = new ROI;
                    if (roi) {
                        roi->tl_x = rect_out[i].tl().x;
                        roi->tl_y = rect_out[i].tl().y;
                        roi->br_x = rect_out[i].br().x;
                        roi->br_y = rect_out[i].br().y;
                        out->ROIList.push_front(roi);
                    }
                }
            }
        }
        rect_out.clear();

        if (m_pOut) {
            videoanalyzer->RGB2YUV(showframe, m_pOut);
        }
        if (bDump == true) {
            videoanalyzer->ShowImage(showframe);
        }
    }
}

#endif

