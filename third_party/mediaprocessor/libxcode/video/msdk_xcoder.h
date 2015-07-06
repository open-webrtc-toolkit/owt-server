/* <COPYRIGHT_TAG> */

#ifndef TEE_TRANSCODER_H_
#define TEE_TRANSCODER_H_

#include "base/media_common.h"
#include "base/mem_pool.h"
#include "base/stream.h"
#include "base/measurement.h"
#include "msdk_codec.h"
#include "mfxvideo++.h"
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
#include "hw_device.h"
#endif

class MFXVideoSession;
class GeneralAllocator;
class Dispatcher;

typedef enum {
    CODEC_TYPE_INVALID = 0,
    CODEC_TYPE_VIDEO_FIRST,
    CODEC_TYPE_VIDEO_AVC,
    CODEC_TYPE_VIDEO_VP8,
    CODEC_TYPE_VIDEO_STRING,
    CODEC_TYPE_VIDEO_PICTURE,
    CODEC_TYPE_VIDEO_LAST,
    CODEC_TYPE_OTHER
} CodecType;

typedef enum {
    DECODER,
    ENCODER
} CoderType;

/* background color*/
typedef struct {
    unsigned short Y;
    unsigned short U;
    unsigned short V;
} BgColor;

typedef struct DecOptions_ {
    union {
        MemPool *inputStream;
        StringInfo *inputString; //added for string decoder
        PicInfo *inputPicture; //added for picture decoder
    };
    CodecType input_codec_type;
    void *DecHandle;
    Measurement *measuremnt;
} DecOptions;

typedef struct VppOptions_ {
    // The output pic/surface width/height
    unsigned short out_width;
    unsigned short out_height;

    /**
     * \brief depth of the look ahead bitrate control .
     * \note Valid values are:
     *    MFX_PICSTRUCT_UNKNOWN =0x00
     *    MFX_PICSTRUCT_PROGRESSIVE =0x01
     *    MFX_PICSTRUCT_FIELD_TFF =0x02
     *    MFX_PICSTRUCT_FIELD_BFF =0x04
     * Default value: MFX_PICSTRUCT_PROGRESSIVE
     */
    unsigned int nPicStruct;

    /**
     * \brief color sampling method.
     * \note Valid values are:
     *    MFX_CHROMAFORMAT_MONOCHROME =0,
     *    MFX_CHROMAFORMAT_YUV420 =1,
     *    MFX_CHROMAFORMAT_YUV422 =2,
     *    MFX_CHROMAFORMAT_YUV444 =3,
     *    MFX_CHROMAFORMAT_YUV400 = MFX_CHROMAFORMAT_MONOCHROME,
     *    MFX_CHROMAFORMAT_YUV411 = 4,
     *    MFX_CHROMAFORMAT_YUV422H = MFX_CHROMAFORMAT_YUV422,
     *    MFX_CHROMAFORMAT_YUV422V = 5
     * Default value: MFX_CHROMAFORMAT_YUV420
     */
    unsigned int chromaFormat;

    /**
     * \brief color format.
     * \note Valid values are:
     *    MFX_FOURCC_NV12 = MFX_MAKEFOURCC('N','V','1','2'), //Native Format
     *    MFX_FOURCC_YV12 = MFX_MAKEFOURCC('Y','V','1','2'),
     *    MFX_FOURCC_YUY2 = MFX_MAKEFOURCC('Y','U','Y','2'),
     *    MFX_FOURCC_RGB3 = MFX_MAKEFOURCC('R','G','B','3'), //RGB24
     *    MFX_FOURCC_RGB4 = MFX_MAKEFOURCC('R','G','B','4'), //RGB32
     *    MFX_FOURCC_P8 = 41, //D3DFMT_P8
     *    MFX_FOURCC_P8_TEXTURE = MFX_MAKEFOURCC('P','8','M','B')
     * Default value: MFX_FOURCC_NV12
     */
    unsigned int colorFormat;
    void *VppHandle;
    Measurement *measuremnt;
} VppOptions;

/**
 * \brief Encoder configuration.
 */
typedef struct EncOptions_ {
    CodecType output_codec_type;

    /**
     * \ brief Codec Flag
     * \ note To indicate if sw code is preferred
     *     If it's true, MsdkXoder will try to use SW codec
     *     or else, it would use HW codec if it exists
     * Default value: false
     */
    bool swCodecPref;

    /**
     * \brief Encoding profile.
     * \note Valid values are:
     *    MFX_PROFILE_AVC_BASELINE = 66,
     *    MFX_PROFILE_AVC_MAIN=77,
     *    MFX_PROFILE_AVC_EXTENDED=88,
     *    MFX_PROFILE_AVC_HIGH=100.
     * Default value: MFX_PROFILE_AVC_MAIN
     */
    unsigned short profile;

    /**
     * \brief Encoding codec level
     * \note Valid values are 10-51.<br>
     * Default value: 41
     */
    unsigned short level;

    /**
     * \brief Bit rate used during encoding.
     * \note Valid range 0 .. \<any positive value\>.<br>
     * Default value: 0
     */
    unsigned short bitrate;

    /**
     * \brief Intra frames period.
     * \note Valid range 0 .. \<any positive value\>.<br>
     * Default value: 30
     */
    unsigned short intraPeriod;

    unsigned short gopRefDist;

    unsigned short idrInterval;

    /**
     * \brief Quantization parameter value.
     * \note Valid range H264[0,51], MPEG2[1,31].<br>
     * Default H264 value: 0
     */
    unsigned short qpValue;

    /**
     * \brief Bitrate control
     * \note Valid values are:
     *     MFX_RATECONTROL_CBR = 1
     *     MFX_RATECONTROL_VBR = 2
     *     MFX_RATECONTROL_CQP = 3
     *     MFX_RATECONTROL_LA = 8
     * Default value: CBR
     */
    unsigned short ratectrl;

    /**
     * \brief Target Usage.
     * \note Valid values are 0~7.<br>
     *    MFX_TARGETUSAGE_1 = 1,
     *    MFX_TARGETUSAGE_2 = 2,
     *    MFX_TARGETUSAGE_3 = 3,
     *    MFX_TARGETUSAGE_4 = 4,
     *    MFX_TARGETUSAGE_5 = 5,
     *    MFX_TARGETUSAGE_6 = 6,
     *    MFX_TARGETUSAGE_7 = 7,
     *    MFX_TARGETUSAGE_UNKNOWN = 0,
     * Default value:MFX_TARGETUSAGE_7
     * 1 is quality mode;7 is speed mode;others is normal mode.
     * *notes: not vaild in CQP mode.
     */
    unsigned short targetUsage;

    /**
     * \brief slice number for each encoded frame.
     * \note Valid range 0 .. 31.<br>
     * Default value: 0
     */
    unsigned short numSlices;

    /**
     * \brief mbbrc enable.
     * \note Valid range 0 .. 1.<br>
     * Default value: 0
     */
    unsigned short mbbrc_enable;

    /**
     * \brief depth of the look ahead bitrate control .
     * \note Valid range 10 .. 100.<br>
     * Default value: 10
     */
    unsigned short la_depth;

    unsigned short numRefFrame;

    Stream *outputStream;

    unsigned short vp8OutFormat;

    unsigned short nMaxSliceSize;

    void *EncHandle;

    Measurement *measuremnt;
} EncOptions;


/**
 * \brief MsdkXcoder class.
 * \details Manages decoder and encoder objects.
 */
class MsdkXcoder
{
public:
    MsdkXcoder();
    virtual ~MsdkXcoder();

    int Init(DecOptions *dec_cfg, VppOptions *vpp_cfg, EncOptions *enc_cfg);

    int ForceKeyFrame(void *ouput_handle);

    int SetBitrate(void *output_handle, unsigned short bitrate);

    int MarkLTR(void *encHandle);

    int SetResolution(void *vppHandle, unsigned int width, unsigned int height);

    int SetComboType(ComboType type, void *vpp, void* master);

    int AttachInput(DecOptions *dec_cfg, void *vppHandle);

    int DetachInput(void* input_handle);

    int SetCustomLayout(void *vppHandle, const CustomLayout *layout);

    int SetBackgroundColor(void *vppHandle, BgColor *bgColor);

    int AttachVpp(DecOptions *dec_cfg, VppOptions *vpp_cfg, EncOptions *enc_cfg);

    int DetachVpp(void* vpp_handle);

    int AttachOutput(EncOptions *enc_cfg, void *vppHandle);

    int DetachOutput(void* output_handle);

    void StringOperateAttach(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void StringOperateDetach(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void StringOperateChange(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void PicOperateAttach(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void PicOperateDetach(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void PicOperateChange(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    bool Start();

    bool Stop();

    bool Join();

    static bool MutexInit() { pthread_mutex_init(&va_mutex_, NULL); return true; }

    static pthread_mutex_t va_mutex_;  /**< \brief va entry mutex*/
private:
    MsdkXcoder(const MsdkXcoder&);
    MsdkXcoder& operator=(const MsdkXcoder&);

    int InitVaDisp();
    void DestroyVaDisp();
    MFXVideoSession* GenMsdkSession();
    void CloseMsdkSession(MFXVideoSession *session);
    void CreateSessionAllocator(MFXVideoSession **session, GeneralAllocator **allocator);
    void CleanUpResource();
    ElementType GetElementType(CoderType type, unsigned int file_type, bool swCodecPref);

    void ReadDecConfig(DecOptions *dec_cfg, void *decCfg);
    void ReadVppConfig(VppOptions *vpp_cfg, void *vppCfg);
    void ReadEncConfig(EncOptions *enc_cfg, void *encCfg);

    std::list<MSDKCodec*> del_dec_list_;

    std::list<MFXVideoSession*> session_list_;
    std::vector<GeneralAllocator*> m_pAllocArray;
    MFXVideoSession* main_session_;
    int dri_fd_;
    static void *va_dpy_;       /**< \brief va diaplay handle*/
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
    CHWDevice *hwdev_;
#endif
    bool is_running_;               /**< \brief xcoder running status*/
    bool done_init_;
    Mutex mutex;
    Mutex sess_mutex;

    MSDKCodec *dec_;
    std::list<MSDKCodec*> dec_list_;

    Dispatcher * dec_dis_;
    std::map<MSDKCodec*, Dispatcher*> dec_dis_map_;

    MSDKCodec* vpp_;
    std::list<MSDKCodec*> vpp_list_;

    Dispatcher* vpp_dis_;
    std::map<MSDKCodec*, Dispatcher*> vpp_dis_map_;

    MSDKCodec *enc_;
    std::multimap<MSDKCodec*, MSDKCodec*> enc_multimap_;

    MSDKCodec *render_;
    bool m_bRender_;
};

#endif /* TEE_TRANSCODER_H_ */
