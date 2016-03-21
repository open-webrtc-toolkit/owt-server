/* <COPYRIGHT_TAG> */

#ifdef ENABLE_VA

#ifndef __VA_PLUGIN_H__
#define __VA_PLUGIN_H__

#include <stdlib.h>
#include <memory.h>
#include <vector>
#include "semaphore.h"

#include "mfxdefs.h"
#include "mfxvideo.h"
#include "mfxvideo++.h"
#include "mfxplugin.h"
#include "mfxplugin++.h"

#include "base/media_common.h"
#include "base/frame_meta.h"
#include "base_allocator.h"
#include "video_analyzer.h"
#include "opencl_filter.h"
#include "opencl_filter_va.h"

#define MAX_THREAD 8
#define MAX_VECTOR_SIZE 256

enum {
    MFX_EXTBUFF_VA_PLUGIN_PARAM = MFX_MAKEFOURCC('V','A','V','E')
};

struct VAPluginData
{
    mfxExtBuffer Header;
    FrameMeta* In;
    FrameMeta* Out;
};

struct VAPluginParam
{
    bool   bOpenCLEnable;
    char*  KernelPath;
    bool   PerfTrace;
    bool   bDump;
    mfxI32 VaInterval;
    VAType VaType;
};

typedef struct {
    mfxU32 StartLine;
    mfxU32 EndLine;
} DataChunk;

typedef struct {
    bool   PerfTrace;
    int    nFrameIndex;
    mfxU32 PrepTime;
    mfxU32 VATime;
    mfxU64 StartSnap;
    mfxU64 EndSnap;
} Statistics;

class Processor
{
public:
    Processor();
    virtual ~Processor();
    virtual mfxStatus SetAllocator(mfxFrameAllocator *pAlloc);
    virtual mfxStatus Init(mfxFrameSurface1 *frame_in, mfxFrameSurface1 *frame_out);
    bool SetAnalyzer(OpencvVideoAnalyzer* video_analyzer);
    virtual mfxStatus Process(VAPluginData* data) = 0;

protected:
    //locks frame or report of an error
    mfxStatus LockFrame(mfxFrameSurface1 *frame);
    mfxStatus UnlockFrame(mfxFrameSurface1 *frame);

    mfxFrameSurface1  *m_pIn;
    mfxFrameSurface1  *m_pOut;
    mfxFrameAllocator *m_pAlloc;

    std::vector<mfxU8> m_YIn, m_UVIn;
    std::vector<mfxU8> m_YOut, m_UVOut;
    OpencvVideoAnalyzer* videoanalyzer;
};

class Analyzer : public Processor
{
public:
    Analyzer();
    Analyzer(bool dump, int interval, VAType type, Statistics *stat, OpenCLFilter *pOpenCLFilter);

    mfxStatus SetAllocator(mfxFrameAllocator* pAlloc);
    virtual ~Analyzer();
    virtual mfxStatus Process(VAPluginData* data);

private:
    OpenCLFilter *m_pOpenCLFilter;
    void ProcessFrame(cv::Mat &frame, cv::Mat &showframe, VAPluginData* data);
    bool bDump;
    int VaInterval;
    VAType VaType;
    int FrameCnt;
    Statistics* Stat;
};

typedef struct {
    mfxFrameSurface1 *In;
    mfxFrameSurface1 *Out;
    bool bBusy;
    VAPluginData* OutMeta;
    Processor *pProcessor;
} AnalyzeTask;

class Analyze : public MFXGenericPlugin
{
public:
    Analyze();
    virtual ~Analyze();

    // methods to be called by Media SDK
    virtual mfxStatus PluginInit(mfxCoreInterface *core);
    virtual mfxStatus Init(mfxVideoParam *mfxParam);
    virtual mfxStatus SetAuxParams(void* auxParam, int auxParamSize);
    virtual mfxStatus PluginClose();
    virtual mfxStatus GetPluginParam(mfxPluginParam *par);
    virtual mfxStatus Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task);
    virtual mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a);
    virtual mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts);
    virtual void Release(){}
    // methods to be called by application
    void SetAllocPoint(MFXFrameAllocator *pMFXAllocator);
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    static MFXGenericPlugin* CreatePlugin() {
        return new Analyze();
    }
    virtual mfxStatus Close();

protected:
    bool m_bInited;

    MFXCoreInterface m_mfxCore;

    mfxVideoParam   m_VideoParam;
    mfxPluginParam  m_PluginParam;
    VAPluginParam   m_Param;

    AnalyzeTask     *m_pTasks;
    mfxU32          m_MaxNumTasks;

    DataChunk       *m_pChunks;

    mfxU32          m_NumChunks;

    OpenCLFilterVA  m_OpenCLFilter;
    mfxHDL          m_device;
    mfxStatus CheckParam(mfxVideoParam *mfxParam, VAPluginParam *pPar);
    mfxStatus CheckInOutFrameInfo(mfxFrameInfo *pIn, mfxFrameInfo *pOut);
    mfxU32 FindFreeTaskIdx();
    mfxExtBuffer* GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId);

    bool m_bIsInOpaque;
    bool m_bIsOutOpaque;
    mfxFrameAllocator*  m_pAlloc;
    OpencvVideoAnalyzer* Opencvanalyzer;

    //statistics
    Statistics m_Stat;
};

#endif

#endif
