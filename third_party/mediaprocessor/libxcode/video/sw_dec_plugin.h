/* <COPYRIGHT_TAG> */

#ifndef SW_DEC_PLUGIN_H_
#define SW_DEC_PLUGIN_H_
#ifdef ENABLE_SW_CODEC

extern "C"
{
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

#include "base/logger.h"
#include "mfxvideo++.h"
#include "mfxplugin++.h"
#include "mfxjpeg.h"
#include "base_allocator.h"

typedef struct {
    mfxFrameSurface1 *surfaceOut; // Pointer to output surface
    mfxBitstream *bitstreamIn; // Pointer to input stream
    bool bBusy; // Task is in use or not
} MFXTask_SW_DEC;

class SWDecPlugin: public MFXGenericPlugin
{
public:
	DECLARE_MLOGINSTANCE();
    SWDecPlugin();
    void SetAllocPoint(MFXFrameAllocator *pMFXAllocator);
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus Init(mfxVideoParam *par);
    virtual mfxStatus DecodeHeader(mfxBitstream *bs, mfxVideoParam *par);
    virtual mfxStatus Close();
    virtual ~SWDecPlugin();
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


    // Interface for software decoder
    mfxStatus ParserStreamInfo(mfxBitstream *bs, unsigned int codec_id);
    mfxStatus InitSWDec();
    void UploadToSurface(AVFrame *picture, mfxFrameSurface1 *surf_out);

private:
    mfxCoreInterface *m_pmfxCore_; // Pointer to mfxCoreInterface. used to increase-decrease reference for in-out frames
    mfxPluginParam m_mfxPluginParam_; // Plug-in parameters
    mfxU16 m_MaxNumTasks_; // Max number of concurrent tasks
    MFXTask_SW_DEC *m_pTasks_; // Array of tasks
    mfxVideoParam *m_VideoParam_;
    mfxFrameAllocator *m_pAlloc_;
    bool no_data_;
    bool m_bIsOpaque;

    //software decoder
    bool init_flag_;
    AVFormatContext *av_fmt_ctx_;
    AVIOContext *av_io_ctx_;
    AVStream *av_stream_;
    AVCodecContext *av_codec_ctx_;
    AVCodec *av_codec_;
    AVFrame *av_frame_;
    AVPacket avpkt_;
    unsigned char *avio_ctx_buffer_;

    unsigned int m_dec_num_;
};

#endif
#endif /* SW_DEC_PLUGIN_H_ */

