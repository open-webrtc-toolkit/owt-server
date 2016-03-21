/* <COPYRIGHT_TAG> */

#ifndef SW_ENC_PLUGIN_H_
#define SW_ENC_PLUGIN_H_
#ifdef ENABLE_SW_CODEC

extern "C"
{
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "base/logger.h"
#include "mfxvideo++.h"
#include "mfxplugin++.h"

#define QP_MIN (1)
#define QP_MAX (51)
#define BIT_RATE_MIN (100000)
#define BIT_RATE_MAX (10000000)

typedef enum {
    SW_CBR,
    SW_VBR,
    SW_CQP
}RateCtrl;

typedef struct EncodeOptions{
    AVCodecID codec_id;
    int bit_rate;
    RateCtrl rate_ctrl;
    unsigned int time_base_num;
    unsigned int time_base_den;
    unsigned int profile;
    unsigned int level;
    unsigned int gop_size;
    unsigned int target_usage;
    unsigned int qp_min;
    unsigned int qp_max;
    unsigned int num_slice;
    unsigned int refs;
    unsigned int de_interlace;
    int thread_count;
    EncodeOptions():
        codec_id(AV_CODEC_ID_H264),
        bit_rate(0),
        rate_ctrl(SW_CBR),
        time_base_num(1000),
        time_base_den(30000),
        profile(FF_PROFILE_H264_MAIN),
        level(41),
        gop_size(25),
        target_usage(0),
        qp_min(QP_MIN),
        qp_max(QP_MAX),
        num_slice(1),
        refs(1),
        de_interlace(0),
        thread_count(1)
    {};
}EncodeOptions;

typedef struct {
    mfxFrameSurface1 *surfaceIn; // Pointer to input surface
    mfxBitstream *bitstreamOut; // Pointer to output stream
    bool bBusy; // Task is in use or not
} MFXTask_SW_ENC;

class SWEncPlugin: public MFXGenericPlugin
{
public:
	DECLARE_MLOGINSTANCE();
    SWEncPlugin();
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus Init(mfxVideoParam *par);
    virtual mfxStatus Close();
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);
    virtual ~SWEncPlugin();
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


    // Interface for software encoder
    mfxStatus InitSWEnc();
    mfxStatus GetInputYUV(mfxFrameSurface1* surf);
    mfxStatus CheckParameters(mfxVideoParam *par);

private:
    mfxCoreInterface *m_pmfxCore_; // Pointer to mfxCoreInterface. used to increase-decrease reference for in-out frames
    mfxPluginParam m_mfxPluginParam_; // Plug-in parameters
    mfxU16 m_MaxNumTasks_; // Max number of concurrent tasks
    MFXTask_SW_ENC *m_pTasks_; // Array of tasks
    mfxVideoParam *m_VideoParam_;
    mfxFrameAllocator *m_pAlloc_;
    mfxBitstream *m_BS_;
    EncodeOptions enc_opt_;

    //software encoder
    bool init_flag_;
    AVCodecContext *c_;
    AVCodec *pCodec_;
    AVFrame *m_pYUVFrame_;

    unsigned char *outbuf_;
    unsigned int outbuf_size_;
    unsigned char *frame_buf_;
    unsigned char *swap_buf_;
    unsigned int frame_size_;
    unsigned int m_height_;
    unsigned int m_width_;

    unsigned int m_enc_num_;
};

#endif
#endif /* SW_ENC_PLUGIN_H_ */

