/* <COPYRIGHT_TAG> */

#pragma once

#ifdef ENABLE_VPX_CODEC
#include "mfxvideo++.h"
#include "mfxplugin++.h"
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
#include <pthread.h>

/**
 * \brief Encoder configuration.
 * \note It will be set with default values if not set by the user
 */
typedef struct VP8EncodeOptions {
    /*!\brief Output stream type
     * Type : RAW|IVF
     */
    unsigned int output_type;

    /*!\brief Output raw stream with header or not
     * Type : 0|1
     */
    unsigned int is_raw_header;

    /*!\brief Maximum number of threads to use
     **
     ** For multi-threaded implementations, use no more than this number of
     ** threads. The codec may use fewer threads than allowed. The value
     ** 0 is equivalent to the value 1.
     **/
    unsigned int threads;

    /*!\brief Bitstream profile to use
     **
     ** Some codecs support a notion of multiple bitstream profiles. Typically
     ** this maps to a set of features that are turned on or off. Often the
     ** profile to use is determined by the features of the intended decoder.
     ** Consult the documentation for the codec to determine the valid values
     ** for this parameter, or set to zero for a sane default.
     **/
    unsigned int       profile;

    /**
     * \brief Bit rate used during encoding.
     * in kilobits per second.
     */
    unsigned int       bitrate;

    /*!\brief Rate control algorithm to use.
     **
     ** Indicates whether the end usage of this stream is to be streamed over
     ** a bandwidth constrained link, indicating that Constant Bit Rate (CBR)
     ** mode should be used, or whether it will be played back on a high
     ** bandwidth link, as from a local disk, where higher variations in
     ** bitrate are acceptable.
     **/
    unsigned int       bitratectrl;

    /**
     * \brief indicates the smallest interval of time in seconds.
     * Default value: 1000
     */
    unsigned int time_base_num;

    /**
     * \brief indicates the smallest interval of time in seconds.
     * Default value: 30000
     */
    unsigned int time_base_den;

    /*!\brief Minimum (Best Quality) Quantizer
     **
     ** The quantizer is the most direct control over the quality of the
     ** encoded image. The range of valid values for the quantizer is codec
     ** specific. Consult the documentation for the codec to determine the
     ** values to use. To determine the range programmatically, call
     ** vpx_codec_enc_config_default() with a usage value of 0.
     **/
    unsigned int min_quantizer;

    /*!\brief Minimum (Best Quality) Quantizer
     **
     ** The quantizer is the most direct control over the quality of the
     ** encoded image. The range of valid values for the quantizer is codec
     ** specific. Consult the documentation for the codec to determine the
     ** values to use. To determine the range programmatically, call
     ** vpx_codec_enc_config_default() with a usage value of 0.
     **/
    unsigned int max_quantizer;

    /*!\brief Keyframe placement mode
     **
     ** This value indicates whether the encoder should place keyframes at a
     ** fixed interval, or determine the optimal placement automatically
     ** (as governed by the #kf_min_dist and #kf_max_dist parameters)
     ** 0:kf_disable   1:kf_auto
     **/
    unsigned int keyframe_auto;

    /*!\brief Keyframe minimum interval
     **
     ** This value, expressed as a number of frames, prevents the encoder from
     ** placing a keyframe nearer than kf_min_dist to the previous keyframe. At
     ** least kf_min_dist frames non-keyframes will be coded before the next
     ** keyframe. Set kf_min_dist equal to kf_max_dist for a fixed interval.
     **/
    unsigned int kf_min_dist;

    /*!\brief Keyframe maximum interval
     **
     ** This value, expressed as a number of frames, forces the encoder to code
     ** a keyframe if one has not been coded in the last kf_max_dist frames.
     ** A value of 0 implies all frames will be keyframes. Set kf_min_dist
     ** equal to kf_max_dist for a fixed interval.
     **/
    unsigned int kf_max_dist;

    /*!\brief control function to set vp8 encoder cpuused
     **
     ** Changes in this value influences, among others, the encoder's selection
     ** of motion estimation methods. Values greater than 0 will increase encoder
     ** speed at the expense of quality.
     ** The full set of adjustments can be found in
     ** onyx_if.c:vp8_set_speed_features().
     ** \todo List highlights of the changes at various levels.
     **
     ** \note Valid range: -16..16
     **/
    int cpu_speed;

    //set default options
    VP8EncodeOptions():
        output_type(-1),
        is_raw_header(1),
        threads(0),
        profile(-1),
        bitrate(0),
        bitratectrl(-1),
        time_base_num(0),
        time_base_den(0),
        min_quantizer(-1),
        max_quantizer(-1),
        keyframe_auto(-1),
        kf_min_dist(-1),
        kf_max_dist(-1),
        cpu_speed(-6)
    {};
} VP8EncodeOptions;

typedef enum VP8Complexity
{
    complexityNormal = 0,
    complexityHigh = 1,
    complexityHigher = 2,
    complexityMax = 3
}VP8Complexity;

// Task structure for OCL plugin
typedef struct {
    mfxFrameSurface1*   surfaceIn;    // Pointer to input stream
    mfxBitstream*       bitstreamOut;     // Pointer to output surface
    bool                bBusy;          // Task is in use or not
} MFXTask1;

// This class uses the generic OpenCL processing class and the Media SDK plug-in interface
// to realize an asynchronous OpenCL frame processing user plug-in which can be used in a transcode pipeline
//
class VP8EncPlugin : public MFXGenericPlugin
{
public:
    VP8EncPlugin();
    void SetAllocPoint(MFXFrameAllocator *pMFXAllocator);
    virtual mfxStatus Init(mfxVideoParam *param);
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus Close();
    virtual ~VP8EncPlugin();
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

    // Interface for vpx encoder
    mfxStatus InitVpxEnc();
    void WriteIvfFileHeader(const vpx_codec_enc_cfg_t *cfg, int frame_cnt);
    void WriteIvfFrameHeader(const vpx_codec_cx_pkt_t *pkt);
    bool SetBitRateCtrlMode(unsigned int mode);
    bool SetBitRate(unsigned int bitrate);
    bool SetProfile(unsigned int profile);
    bool SetGopSize(unsigned int gopsize);
    bool SetGenericThread(unsigned int cpu_cores);
    bool SetCPUSpeed(VP8Complexity complexity_level);
    bool CheckParameters(mfxVideoParam *par);
    mfxStatus GetInputYUV(mfxFrameSurface1* surf);
    void VP8ForceKeyFrame();
    void VP8ResetBitrate(unsigned int bitrate);
    int  VP8ResetRes(unsigned int width, unsigned int height);
    static bool MutexInit() { pthread_mutex_init(&plugin_mutex_, NULL); return true; }
    static pthread_mutex_t plugin_mutex_;

protected:
    mfxCoreInterface*   m_pmfxCore;         // Pointer to mfxCoreInterface. used to increase-decrease reference for in-out frames
    mfxPluginParam      m_mfxPluginParam;   // Plug-in parameters
    mfxU16              m_MaxNumTasks;      // Max number of concurrent tasks
    MFXTask1*            m_pTasks;           // Array of tasks
    mfxVideoParam       m_VideoParam;
    mfxFrameAllocator*  m_pAlloc;
    mfxBitstream*       m_BS;
    mfxExtBuffer*       m_ExtParam;
    bool vp8_force_key_frame;
    vpx_codec_enc_cfg_t  *cfg;
    bool m_bIsOpaque;
    // VPX Encoder
    vpx_codec_ctx_t  vpx_codec_;
    vpx_image_t      raw_;
    VP8EncodeOptions encoder_options_;
    bool             vpx_init_flag_;
    bool             output_ivf_;
    bool             is_raw_hdr_;
    unsigned int     m_width_;
    unsigned int     m_height_;
    unsigned int     keyframe_dist_;
    unsigned int     frame_cnt_;
    unsigned char*   frame_buf_;
    unsigned char*   swap_buf_; //local buffer for surface data read
    
};

#endif
