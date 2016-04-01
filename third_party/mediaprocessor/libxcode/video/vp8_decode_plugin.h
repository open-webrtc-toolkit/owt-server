/* <COPYRIGHT_TAG> */
#pragma once

#ifdef ENABLE_VPX_CODEC
#include "mfxvideo++.h"
#include "mfxplugin++.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"
#include "base/media_common.h"

typedef struct VP8InputCtx {
    enum VP8FILETYPE kind;
    unsigned int     fourcc;
    unsigned int     width;
    unsigned int     height;
    unsigned int     fps_den;
    unsigned int     fps_num;

    VP8InputCtx():
        kind(RAW_FILE),
        fourcc(0),
        width(0),
        height(0),
        fps_den(0),
        fps_num(0) {
        };
} VP8InputCtx;

// Task structure for OCL plugin
typedef struct {
    mfxBitstream*       bitstreamIn;    // Pointer to input stream
    mfxFrameSurface1*   surfaceOut;     // Pointer to output surface
    bool                bBusy;          // Task is in use or not
} MFXTask;

//
// This class uses the generic OpenCL processing class and the Media SDK plug-in interface
// to realize an asynchronous OpenCL frame processing user plug-in which can be used in a transcode pipeline
//
class VP8DecPlugin : public MFXGenericPlugin
{
public:
    VP8DecPlugin();
    void SetAllocPoint(MFXFrameAllocator *pMFXAllocator);
    virtual mfxStatus Init(mfxVideoParam *param);
    virtual mfxStatus DecodeHeader(mfxBitstream *bs, mfxVideoParam *par);
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus Close();
    virtual ~VP8DecPlugin();
    mfxExtBuffer* GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId);

    // Interface called by Media SDK framework
    mfxStatus PluginInit(mfxCoreInterface *core);
    mfxStatus PluginClose();
    mfxStatus GetPluginParam(mfxPluginParam *par);
    mfxStatus Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task);  // called to submit task but not execute it
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a); // function that called to start task execution and check status of execution
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts); // free task and releated resources
    mfxStatus SetAuxParams(void* auxParam, int auxParamSize);
    void      Release();

    // Interface for vpx decoder
    mfxStatus InitVpxDec();
    mfxStatus ParseInputType(mfxBitstream *bs);
    void UploadToSurface(vpx_image_t *yuv_image, mfxFrameSurface1 *surface_out);
    bool IsIvfFile( unsigned char *raw_hdr,
                    unsigned int  *fourcc,
                    unsigned int  *width,
                    unsigned int  *height,
                    unsigned int  *fps_den,
                    unsigned int  *fps_num,
                    unsigned int  *bytes_read);
    bool IsRawFile( unsigned char *raw_hdr,
                    unsigned int  *fourcc,
                    unsigned int  *width,
                    unsigned int  *height,
                    unsigned int  *fps_den,
                    unsigned int  *fps_num,
                    unsigned int  *bytes_read);
    bool IsWebMFile(unsigned char *raw_hdr,
                    unsigned int  *fourcc,
                    unsigned int  *width,
                    unsigned int  *height,
                    unsigned int  *fps_den,
                    unsigned int  *fps_num,
                    unsigned int  *bytes_read);
   
protected:
    mfxCoreInterface*   m_pmfxCore;         // Pointer to mfxCoreInterface. used to increase-decrease reference for in-out frames
    mfxPluginParam      m_mfxPluginParam;   // Plug-in parameters
    mfxU16              m_MaxNumTasks;      // Max number of concurrent tasks
    MFXTask*            m_pTasks;           // Array of tasks
    mfxVideoParam       m_VideoParam;
    mfxFrameAllocator*  m_pAlloc;
    mfxExtBuffer*       m_ExtParam;
    bool m_bIsOpaque;
    // VPX Decoder
    vpx_codec_ctx_t  vpx_codec_;
    bool             vpx_init_flag_;
    VP8InputCtx      input_ctx_;
    
};
#endif


