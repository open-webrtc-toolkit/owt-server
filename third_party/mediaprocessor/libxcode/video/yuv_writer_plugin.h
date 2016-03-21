/* <COPYRIGHT_TAG> */

#ifndef YUV_WRITER_PLUGIN_H_
#define YUV_WRITER_PLUGIN_H_
#ifdef ENABLE_RAW_CODEC

#include <stdio.h>

#include "mfxvideo++.h"
#include "mfxplugin++.h"
#include "base/logger.h"
#include "base_allocator.h"

typedef struct {
    mfxFrameSurface1 *surfaceIn; // Pointer to input surface
    mfxBitstream *bitstreamOut; // Pointer to output stream
    bool bBusy; // Task is in use or not
} MFXTask_YUV_WRITER;

class YUVWriterPlugin: public MFXGenericPlugin
{
public:
	DECLARE_MLOGINSTANCE();
    YUVWriterPlugin();
    void SetAllocPoint(MFXFrameAllocator *pMFXAllocator);
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus Init(mfxVideoParam *par);
    virtual mfxStatus Close();
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);
    virtual ~YUVWriterPlugin();
    mfxExtBuffer *GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId);

    // Interface called by Media SDK framework
    mfxStatus PluginInit(mfxCoreInterface *core);
    mfxStatus PluginClose();
    mfxStatus GetPluginParam(mfxPluginParam *par);
    mfxStatus Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task); // called to submit task but not execute it
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a); // function that called to start task execution and check status of execution
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts); // free task and releated resources
    mfxStatus SetAuxParams(void *auxParam, int auxParamSize);
    void      Release();


    // Interface for yuv writer
    mfxStatus GetInputYUV(mfxFrameSurface1* surf);

    mfxStatus CheckParameters(mfxVideoParam *par);

private:
    mfxCoreInterface *m_pmfxCore; // Pointer to mfxCoreInterface. used to increase-decrease reference for in-out frames
    mfxPluginParam m_mfxPluginParam; // Plug-in parameters
    mfxU16 m_MaxNumTasks; // Max number of concurrent tasks
    MFXTask_YUV_WRITER *m_pTasks; // Array of tasks
    mfxVideoParam *m_VideoParam;
    mfxFrameAllocator *m_pAlloc;
    mfxBitstream *m_BS;
    unsigned char *frame_buf_;
    unsigned char *swap_buf_;
    unsigned int m_height_;
    unsigned int m_width_;
    bool m_bIsOpaque;

};

#endif
#endif /* YUV_WRITER_PLUGIN_H_ */

