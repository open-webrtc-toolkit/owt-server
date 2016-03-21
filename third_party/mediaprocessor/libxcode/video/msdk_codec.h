/* <COPYRIGHT_TAG> */


#ifndef MSDK_CODEC_H
#define MSDK_CODEC_H

#include <map>
#include <stdio.h>
#include <mfxvideo++.h>
#include <mfxplugin++.h>
#ifdef MFX_DISPATCHER_EXPOSED_PREFIX
#include <mfxvp8.h>
#endif

#ifdef MSDK_FEI
#include <mfxfei.h>
#endif

#include "base/base_element.h"
#include "base/media_pad.h"
#include "base/media_types.h"
#include "base/media_common.h"
#include "base/mem_pool.h"
#include "base/stream.h"
#include "base/measurement.h"
#include "base/trace.h"
#include "general_allocator.h"
#include "base/logger.h"

#ifdef ENABLE_VA
#include "base/frame_meta.h"
#endif

#if defined(ENABLE_VA) || defined(LIBVA_X11_SUPPORT) || defined(LIBVA_DRM_SUPPORT)
#include "vaapi_device.h"
#endif

#ifndef MFX_DISPATCHER_EXPOSED_PREFIX
#define MFX_CODEC_VP8 101
#endif
#define MFX_CODEC_STRING 102
#define MFX_CODEC_PICTURE 103
#ifdef ENABLE_RAW_DECODE
#define MFX_CODEC_YUV 104
#endif
// number of video enhancement filters (denoise, procamp, detail, video_analysis, image stab)
#define ENH_FILTERS_COUNT 5

#ifdef MSDK_FEI
typedef struct {
    mfxExtFeiPreEncMV *mv_info;
    mfxExtFeiPreEncMBStat *mb_info;
    unsigned yuv_width;
    unsigned yuv_height;
} PreEncInfo;

typedef struct {
    bool sceneChangeFrame;
} PreEncResults;

typedef int (*FEIChainCBFunc)(PreEncInfo *info, PreEncResults *results);

//for PreEnc reordering
struct iTask {
    mfxENCInput in;
    mfxENCOutput out;
    mfxU16 frameType;
    mfxU32 frameDisplayOrder;
    mfxSyncPoint EncSyncP;
    mfxI32 encoded;
    iTask *prevTask;
    iTask *nextTask;
};
#endif

#ifdef ENABLE_VA
typedef enum {
    FORMAT_ES,
    FORMAT_TXT,
    FORMAT_VECTOR
} VA_OUTPUT_FORMAT;
#endif

enum ElementType : unsigned int {
    ELEMENT_DECODER,
    ELEMENT_ENCODER,
    ELEMENT_VPP,
    ELEMENT_VA,
    ELEMENT_VP8_DEC, //vp8 sw decoder
    ELEMENT_VP8_ENC, //vp8 sw encoder
    ELEMENT_SW_DEC,
    ELEMENT_SW_ENC,
    ELEMENT_YUV_WRITER,
    ELEMENT_YUV_READER,
    ELEMENT_STRING_DEC,
    ELEMENT_BMP_DEC,
    ELEMENT_RENDER,
    ELEMENT_CUSTOM_PLUGIN,
    ELEMENT_FEI_PREENC
};

typedef struct {
    bool bMBBRC;
    mfxU16 nLADepth; // depth of the look ahead bitrate control  algorithm
    mfxU32 nMaxSliceSize; //Max Slice Size, for AVC only
} EncExtParams;

typedef struct {
    union {
        union {
            MemPool *input_stream;
            StringInfo *input_str; //added for string decoder
            PicInfo *input_pic; //added for picture decoder
#if defined  ENABLE_RAW_DECODE
            RawInfo *input_raw; //added for picture decoder
#endif
        };
        Stream *output_stream;
    };
    union {
        mfxVideoParam DecParams;
        mfxVideoParam EncParams;
        mfxVideoParam VppParams;
        mfxVideoParam PlgParams;
    };
    union {
        void* DecHandle;
        void* VppHandle;
        void* EncHandle;
    };
    Measurement  *measuremnt;
    EncExtParams extParams;
#ifdef ENABLE_VA
    bool bOpenCLEnable;
    char* KernelPath;
    bool bDump;
    bool PerfTrace;
    int  va_interval;
    VAType va_type;
    VA_OUTPUT_FORMAT output_format;
#endif
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
    CHWDevice *hwdev;
#endif
#ifdef MSDK_FEI
    FEIChainCBFunc fei_cb_func;
#endif
} ElementCfg;

typedef struct {
    unsigned x;
    unsigned y;
    unsigned w;
    unsigned h;
} VppRect;

typedef struct VPPCompInfo {
    MediaPad *vpp_sinkpad;
    VppRect dst_rect;
    StringInfo str_info;
    PicInfo pic_info;
#if defined  ENABLE_RAW_DECODE
    RawInfo raw_info;
#endif
    MediaBuf ready_surface;
    //mfxFrameSurface1* ready_surface;
    float frame_rate;
    unsigned int drop_frame_num; // Number of frames need to be dropped.
    unsigned int total_dropped_frames; // Totally dropped frames.
    unsigned int org_width; // frame's width before composite.
    unsigned int org_height; // frame's height before composite.

    VPPCompInfo():
    drop_frame_num(0),
    total_dropped_frames(0) {
    };
} VPPCompInfo;

/* background color*/
typedef struct {
    unsigned short Y;
    unsigned short U;
    unsigned short V;
} BgColorInfo;

class MSDKCodec: public BaseElement
{
public:
    /*!
     *  \brief Initialize a MSDK element.
     *  @param[in]: element_type, specify the type of element, see @ElementType
     *  @param[in]: session, session of this element.
     */
DECLARE_MLOGINSTANCE();
    MSDKCodec(ElementType element_type, MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL, CodecEventCallback *callback = NULL);

    virtual ~MSDKCodec();

    /*!
     *  \brief Initialize a MSDK element.
     *  @param[in]: ele_param, specify the configuration of element.
     *  @param[in]: element_mode, active or passive.
     */
    virtual bool Init(void *cfg, ElementMode element_mode);

#ifdef ENABLE_VA
    int SetVARegion(FrameMeta* frame);
    int Step(FrameMeta* input);
#endif

    int SetVPPCompRect(BaseElement *element, VppRect *rect);

    ElementType GetCodecType();

    int MarkLTR();

    void SetForceKeyFrame();

    void SetResetBitrateFlag();

    void SetBitrate(unsigned short bitrate);

    int SetRes(unsigned int width, unsigned int height);

    int ConfigVppCombo(ComboType combo_type, void *master_handle);

    int SetCompRegion(const CustomLayout *layout);

    int SetBgColor(BgColorInfo *bgColorInfo);

    int SetKeepRatio(bool isKeepRatio);

    StringInfo *GetStrInfo() {
        return input_str_;
    }
    void SetStrInfo(StringInfo *strinfo) {
        input_str_ = strinfo;
    }
    void SetCompInfoChe(bool ifchange) {
        comp_info_change_ = ifchange;
    }
    unsigned long GetNumOfVppFrm() {
        return vppframe_num_;
    }
    unsigned int GetVppOutFpsN() {
        return mfx_video_param_.vpp.Out.FrameRateExtN;
    }
    unsigned int GetVppOutFpsD() {
        return mfx_video_param_.vpp.Out.FrameRateExtD;
    }
    PicInfo *GetPicInfo() {
        return input_pic_;
    }
    void SetPicInfo(PicInfo *picinfo) {
        input_pic_ = picinfo;
    }
#if defined  ENABLE_RAW_DECODE
    RawInfo *GetRawInfo() {
        return input_raw_;
    }
    void SetRawInfo(RawInfo *rawinfo) {
        input_raw_ = rawinfo;
    }
#endif
    int QueryStreamCnt() {
        return stream_cnt_;
    }
protected:
    virtual void NewPadAdded(MediaPad *pad);
    virtual void PadRemoved(MediaPad *pad);
    virtual int HandleProcess();

    virtual int Recycle(MediaBuf &buf);
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);


private:
    ElementType element_type_;
    MFXVideoSession *mfx_session_;
    unsigned num_of_surf_;
    bool codec_init_;
    MemPool *input_mp_;
    Stream *output_stream_;
    mfxFrameSurface1 **surface_pool_;
    mfxVideoParam mfx_video_param_;
    //mfxVideoParam mfx_va_param_;
    MFXFrameAllocator *m_pMFXAllocator;
    mfxExtBuffer *pExtBuf[1 + ENH_FILTERS_COUNT];
    mfxExtOpaqueSurfaceAlloc ext_opaque_alloc_;
    bool m_bUseOpaqueMemory;
    // Decoder
    mfxBitstream input_bs_;
    StringInfo *input_str_;
    PicInfo *input_pic_;
#if defined  ENABLE_RAW_DECODE
    RawInfo *input_raw_;
#endif
    MFXVideoDECODE *mfx_dec_;
    mfxFrameAllocResponse m_mfxDecResponse;  // memory allocation response for decoder
    std::vector<mfxExtBuffer *> m_DecExtParams;
    bool is_eos_;

    // Encoder
    MFXVideoENCODE *mfx_enc_;
    mfxBitstream output_bs_;
    mfxFrameAllocResponse m_mfxEncResponse;  // memory allocation response for encode
    std::vector<mfxExtBuffer *> m_EncExtParams;
    mfxExtCodingOption coding_opt;
    mfxExtCodingOption2 coding_opt2;
    bool force_key_frame_;
    bool reset_bitrate_flag_;
    bool reset_res_flag_;
    mfxEncodeCtrl enc_ctrl_;
    mfxU32 m_nFramesProcessed;
    mfxExtAVCRefListCtrl m_avcRefList;
    bool m_bMarkLTR;

    // VPP
    MFXVideoVPP *mfx_vpp_;
    std::map<MediaPad *, VPPCompInfo> vpp_comp_map_;
    unsigned int comp_stc_;
    float max_comp_framerate_; // Largest frame rate of composition streams.
    unsigned current_comp_number_;
    bool comp_info_change_;    //denote whether info for composition changed
    unsigned long vppframe_num_;
    ComboType combo_type_;
    CustomLayout dec_region_list_; //for COMBO_CUSTOM mode only.
    void *combo_master_;
    bool reinit_vpp_flag_;
    bool res_change_flag_;
    int stream_cnt_;
    int string_cnt_;
    int pic_cnt_;
    int raw_cnt_;
    int eos_cnt;
    //res of pre-allocated surfaces, following res change can't exceed it.
    unsigned int frame_width_;
    unsigned int frame_height_;
    BgColorInfo bg_color_info;
    // keep width/height ratio
    bool keep_ratio_;
    CodecEventCallback *callback_;

#ifdef MSDK_FEI
    MFXVideoENC *mfx_fei_preenc_;
    mfxExtFeiParam  mfx_ext_fei_param_;
    mfxExtFeiPreEncCtrl mfx_fei_preenc_ctr_;


    mfxEncodeCtrl mfx_fei_ctrl_;
    mfxExtFeiEncFrameCtrl mfx_fei_enc_ctrl_;
    mfxExtBuffer *inBufs[4];
    // mfxExtBuffer * outBufs[2];

    int num_ext_out_param_;
    int num_ext_in_param_;
    mfxExtBuffer *inBufsPreEnc[4];
    mfxExtBuffer *outBufsPreEnc[2];
    mfxExtBuffer *outBufsPreEncI[2];
    mfxExtFeiPreEncMV mvs;
    mfxExtFeiPreEncMBStat mbdata;

    int mfx_fei_ref_dist_;
    int mfx_fei_gop_size_;
    unsigned mfx_fei_frame_seq_;
    std::list<iTask *> mfx_fei_input_tasks_; //used in PreENC
#endif
#ifdef ENABLE_VA
    // VA plugin
    MFXGenericPlugin *user_va_;
    mfxExtBuffer **extbuf_;
    VA_OUTPUT_FORMAT output_format_;
    FrameMeta* in_;
#endif
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
    CHWDevice *m_hwdev;
#endif
    void *aux_param_;
    int aux_param_size_;

    // VP8 plugins
#ifdef ENABLE_VPX_CODEC
    MFXGenericPlugin *user_dec_;
    MFXGenericPlugin *user_enc_;
#endif

#ifdef ENABLE_STRING_CODEC
    MFXGenericPlugin *str_dec_;
#endif

#ifdef ENABLE_PICTURE_CODEC
    MFXGenericPlugin *pic_dec_;
#endif

#ifdef ENABLE_SW_CODEC
    MFXGenericPlugin *user_sw_dec_;
    MFXGenericPlugin *user_sw_enc_;
#endif
#ifdef ENABLE_RAW_DECODE
    MFXGenericPlugin *yuv_dec_;
#endif
#ifdef ENABLE_RAW_CODEC
    MFXGenericPlugin *user_yuv_writer_;
#endif

    MSDKCodec(const MSDKCodec &);
    MSDKCodec &operator=(const MSDKCodec &);

    mfxStatus InitDecoder();
    mfxStatus InitEncoder(MediaBuf &buf);
    mfxStatus InitVpp(MediaBuf &buf);
#ifdef ENABLE_VA
    mfxStatus InitVA(MediaBuf &buf);
#endif
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
    mfxStatus InitRender(MediaBuf &buf);
#endif
    void UpdateMemPool();
    void UpdateBitStream();
    mfxStatus AllocFrames(mfxFrameAllocRequest *pRequest, bool isDecAlloc);

    int GetFreeSurfaceIndex(mfxFrameSurface1 **pSurfacesPool, mfxU16 nPoolSize);
    int HandleProcessDecode();
    int HandleProcessEncode();
    int HandleProcessVpp();
#ifdef MSDK_FEI
    int InitPreENC(MediaBuf &buf);
    int HandleProcessPreENC();
    int GetFrameType(int pos);
    iTask *findFrameToEncode();
    void initFrameParams(iTask *eTask);
    FEIChainCBFunc fei_cb_func_;
#endif
#ifdef ENABLE_VA
    int HandleProcessVA();
    int ProcessChainVA(MediaBuf &buf);
    mfxStatus DoingVA(mfxFrameSurface1 *in_surf, mfxFrameSurface1 *out_surf);
#endif
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
    int HandleProcessRender();
    int ProcessChainRender(MediaBuf &buf);
    mfxStatus DoingRender(mfxFrameSurface1 *in_surf);
#endif
    int ProcessChainDecode();
    int ProcessChainEncode(MediaBuf &buf);
    int ProcessChainVpp(MediaBuf &buf);
    mfxStatus DoingVpp(mfxFrameSurface1 *in_surf, mfxFrameSurface1 *out_surf);

    mfxStatus WriteBitStreamFrame(mfxBitstream *pMfxBitstream, FILE *fSink);
    mfxStatus WriteBitStreamFrame(mfxBitstream *pMfxBitstream, Stream *out_stream);
    int SetVppCompParam(mfxExtVPPComposite *vpp_comp);
    int PrepareVppCompFrames();

    void OnFrameEncoded(mfxBitstream *pBs);
    void OnMarkLTR();

    void ComputeCompPosition(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int org_width, unsigned int org_height, mfxVPPCompInputStream &inputStream);
    int GetCompBufFromPad(MediaPad *pad, bool &out_of_time
#if not defined SUPPORT_SMTA
            , int &pad_max_level
            , unsigned pad_com_start
            , int pad_comp_timer
            , int pad_cursor
#endif
            );
    /**
     * \brief benchmark.
     */
    Measurement  *measuremnt;
    EncExtParams extParams;
    static unsigned int  mNumOfEncChannels;
    static unsigned int  mNumOfDecChannels;
};

#endif
