/* <COPYRIGHT_TAG> */

#ifdef ENABLE_VA

#ifndef VA_XCODER_H_
#define VA_XCODER_H_

#define LIBVA_X11_SUPPORT
#define LIBVA_SUPPORT

#include <fcntl.h>
#include <unistd.h>
#include "va/va.h"
#include "va/va_drm.h"
#include "base/media_types.h"
#include "base/media_common.h"
#include "base/dispatcher.h"
#include "base/mem_pool.h"
#include "base/stream.h"
#include "video/msdk_codec.h"
#include "video/general_allocator.h"
#include "video/vaapi_device.h"
#include "base/logger.h"

/**
 * \ brief VAXcoder class
 * \ details manafes decoder and encoder objects.
 *
 */

typedef struct {
    ElementMode mode;
    MemPool *inputBs;
    unsigned input_codec_type;
    unsigned gop_size;
    unsigned frame_rate_num;
    unsigned frame_rate_den;
}DecParam;

typedef struct {
    unsigned int width;
    unsigned int height;
}VppParam;

typedef struct {
    Stream* bs_output;
    unsigned output_codec_type;
    unsigned bitrate;
    unsigned gop_size;
    unsigned frame_rate_num;
    unsigned frame_rate_den;
}EncParam;

typedef struct {
    Stream* va_output;
    VAType   type;
    unsigned interval;
    bool dump;
    bool OpenCLEnable;
    char* KernelPath;
    bool PerfTrace;
    VA_OUTPUT_FORMAT out_format;
}VaParam;

typedef struct {
    unsigned dis_x;
    unsigned dis_y;
    unsigned dis_w;
    unsigned dis_h;
}DisParam;

class VAXcoder
{
public:
	DECLARE_MLOGINSTANCE();
    VAXcoder();
    virtual ~VAXcoder();
    int InitInput(bool bDis, DecParam *dec_param);
    void AttachVaOutput(VppParam *vpp_param, VaParam *va_param);
    void AttachBsOutput(VppParam *vpp_param, EncParam *enc_param);
    void AttachDisOutput(VppParam *vpp_param, DisParam *dis_param);
    bool Start();
    int Step(FrameMeta* input);
    bool Stop();
    bool Join();
    MemPool* mp_;
    Stream* va_output_;
    Stream* enc_output_;
private:
    void ReleaseRes();
    bool CreateMSDKRes(MFXVideoSession** psession, GeneralAllocator** palloc);

    VADisplay va_dpy_;
    CHWDevice *hwdev_;
    int card_fd_;
    mfxIMPL impl_;
    mfxVersion ver_;
    // resource
    MSDKCodec* dec_;
    Dispatcher* tee_;
    MFXVideoSession* pMainSession_;
    GeneralAllocator* pMainAllocator_;

    MSDKCodec* vpp4va_;
    MSDKCodec* va_;
    MSDKCodec* va4enc_;
    MFXVideoSession* pVaSession_;
    GeneralAllocator* pVaAllocator_;

    MSDKCodec* vpp4enc_;
    MSDKCodec* enc_;
    MFXVideoSession* pEncSession_;
    GeneralAllocator* pEncAllocator_;

    MSDKCodec* vpp4dis_;
    MSDKCodec* dis_;
    MFXVideoSession* pDisSession_;
    GeneralAllocator* pDisAllocator_;
};
#endif

#endif
