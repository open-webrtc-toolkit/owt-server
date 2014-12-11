/* <COPYRIGHT_TAG> */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "base/partially_linear_bitrate.h"
#include "msdk_codec.h"
#include "vaapi_allocator.h"

#ifdef ENABLE_VPX_CODEC
#include "vp8_decode_plugin.h"
#include "vp8_encode_plugin.h"
#endif

#ifdef ENABLE_STRING_CODEC
#include "string_decode_plugin.h"
#endif

#ifdef ENABLE_PICTURE_CODEC
#include "picture_decode_plugin.h"
#endif

#ifdef ENABLE_SW_CODEC
#include "sw_dec_plugin.h"
#include "sw_enc_plugin.h"
#endif

#ifdef ENABLE_RAW_CODEC
#include "yuv_writer_plugin.h"
#endif

#ifdef ENABLE_VA
#include "va_plugin.h"
#endif

#define MSDK_ALIGN32(X)     (((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define MSDK_ALIGN16(value)     (((value + 15) >> 4) << 4)

static unsigned int GetSysTimeInUs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

static mfxU16 CalDefBitrate(mfxU32 nCodecId, mfxU32 nTargetUsage, mfxU32 nWidth, mfxU32 nHeight, mfxF64 dFrameRate)
{
    PartiallyLinearBitrate fnc;
    mfxF64 bitrate = 0;

    switch (nCodecId)
    {
        case MFX_CODEC_AVC :
            fnc.AddPair(0, 0);
            fnc.AddPair(25344, 225);
            fnc.AddPair(101376, 1000);
            fnc.AddPair(414720, 4000);
            fnc.AddPair(2058240, 5000);
            break;
        case MFX_CODEC_MPEG2:
            fnc.AddPair(0, 0);
            fnc.AddPair(414720, 12000);
            break;
        default:
            fnc.AddPair(0, 0);
            fnc.AddPair(414720, 12000);
            break;
    }

    mfxF64 at = nWidth * nHeight * dFrameRate / 30.0;

    if (!at)
    {
        return 0;
    }

    switch (nTargetUsage)
    {
        case MFX_TARGETUSAGE_BEST_QUALITY :
            bitrate = (&fnc)->at(at);
            break;
        case MFX_TARGETUSAGE_BEST_SPEED :
            bitrate = (&fnc)->at(at) * 0.5;
            break;
        case MFX_TARGETUSAGE_BALANCED :
        default:
            bitrate = (&fnc)->at(at) * 0.75;
            break;
    }

    return (mfxU16)bitrate;
}

MSDKCodec::MSDKCodec(ElementType type, MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator):
    element_type_(type), mfx_session_(session)
{
    codec_init_ = false;
    surface_pool_ = NULL;
    num_of_surf_ = 0;
    output_stream_ = NULL;
    input_mp_ = NULL;
    input_str_ = NULL;
    input_pic_ = NULL;
    mfx_dec_ = NULL;
    mfx_enc_ = NULL;
    mfx_vpp_ = NULL;
    vppframe_num_ = 0;
    force_key_frame_ = false;
    reset_bitrate_flag_ = false;
    reset_res_flag_ = false;
    combo_type_ = COMBO_BLOCKS;
    combo_master_ = NULL;
    reinit_vpp_flag_ = false;
    res_change_flag_ = false;
    apply_region_info_flag_ = false;
    stream_cnt_ = 0;
    string_cnt_ = 0;
    pic_cnt_ = 0;
    eos_cnt = 0;
    is_eos_ = false;
#ifdef ENABLE_VA
    user_va_ = NULL;
#endif
#ifdef ENABLE_VPX_CODEC
    user_dec_ = NULL;
    user_enc_ = NULL;
#endif
#ifdef ENABLE_STRING_CODEC
    str_dec_ = NULL;
#endif
#ifdef ENABLE_PICTURE_CODEC
    pic_dec_ = NULL;
#endif
#ifdef ENABLE_SW_CODEC
    user_sw_dec_ = NULL;
    user_sw_enc_ = NULL;
#endif
#ifdef ENABLE_RAW_CODEC
    user_yuv_writer_ = NULL;
#endif
#ifdef MSDK_FEI
    mfx_fei_preenc_ = NULL;
    num_ext_out_param_ = 0;
    num_ext_in_param_ = 0;
    mfx_fei_ref_dist_ = 0;
    mfx_fei_gop_size_ = 0;
    mfx_fei_frame_seq_ = 0;
#endif

    if (pMFXAllocator) {
        m_pMFXAllocator = pMFXAllocator;
        m_bUseOpaqueMemory = false;
    } else {
        m_pMFXAllocator = NULL;
        m_bUseOpaqueMemory = true;
    }

    memset(&ext_opaque_alloc_, 0, sizeof(ext_opaque_alloc_));
    memset(&input_bs_, 0, sizeof(input_bs_));
    memset(&mfx_video_param_, 0, sizeof(mfx_video_param_));
    memset(&output_bs_, 0, sizeof(output_bs_));
    memset(&m_mfxDecResponse, 0, sizeof(m_mfxDecResponse));
    memset(&m_mfxEncResponse, 0, sizeof(m_mfxEncResponse));
    memset(&coding_opt, 0, sizeof(coding_opt));
    memset(&coding_opt2, 0, sizeof(coding_opt2));
    comp_stc_ = 0;
    max_comp_framerate_ = 0;
    current_comp_number_ = 0;
    aux_param_size_ = 0;
    aux_param_ = NULL;
    comp_info_change_ = false;
    frame_width_ = 0;
    frame_height_ = 0;
    old_out_width_ = 0;
    old_out_height_ = 0;
    memset(&bg_color_info, 0, sizeof(bg_color_info));
}

MSDKCodec::~MSDKCodec()
{
    if (ELEMENT_DECODER == element_type_) {
        if (mfx_dec_) {
            mfx_dec_->Close();
            delete mfx_dec_;
            mfx_dec_ = NULL;
        }

#ifdef MSDK_FEI
    } else if (ELEMENT_FEI_PREENC == element_type_) {
        if (mfx_fei_preenc_) {
            mfx_fei_preenc_->Close();
            delete mfx_fei_preenc_;
            mfx_fei_preenc_ = NULL;
        }

#endif
#ifdef ENABLE_VPX_CODEC
    } else if (ELEMENT_VP8_DEC == element_type_) {
        if (user_dec_) {
            ((VP8DecPlugin *)user_dec_)->Close();
            MFXVideoUSER_Unregister(*mfx_session_, 0);
            delete user_dec_;
            user_dec_ = NULL;
        }

#endif
#ifdef ENABLE_STRING_CODEC
    } else if (ELEMENT_STRING_DEC == element_type_) {
        if (str_dec_) {
            ((StringDecPlugin *)str_dec_)->Close();
            MFXVideoUSER_Unregister(*mfx_session_, 0);
            delete str_dec_;
            str_dec_ = NULL;
        }

#endif
#ifdef ENABLE_PICTURE_CODEC
    } else if (ELEMENT_BMP_DEC == element_type_) {
        if (pic_dec_) {
            ((PicDecPlugin *)pic_dec_)->Close();
            MFXVideoUSER_Unregister(*mfx_session_, 0);
            delete pic_dec_;
            pic_dec_ = NULL;
        }

#endif
#ifdef ENABLE_SW_CODEC
    } else if (ELEMENT_SW_DEC == element_type_) {
        if (user_sw_dec_) {
            ((SWDecPlugin *)user_sw_dec_)->Close();
            MFXVideoUSER_Unregister(*mfx_session_, 0);
            delete user_sw_dec_;
            user_sw_dec_ = NULL;
        }

#endif
    } else if (ELEMENT_VPP == element_type_) {
        if (mfx_vpp_) {
            mfx_vpp_->Close();
            delete mfx_vpp_;
            mfx_vpp_ = NULL;
        }

#ifdef ENABLE_VA
    }  else if (ELEMENT_VA == element_type_) {
        if (user_va_) {
            ((Analyze *)user_va_)->Close();
            MFXVideoUSER_Unregister(*mfx_session_, 0);
            delete user_va_;
            user_va_ = NULL;
        }

        if (aux_param_) {
            delete aux_param_;
            aux_param_ = NULL;
            aux_param_size_ = 0;
        }
    }  else if (ELEMENT_RENDER == element_type_) {
#endif
    } else if (ELEMENT_ENCODER == element_type_) {
        if (mfx_enc_) {
            mfx_enc_->Close();
            delete mfx_enc_;
            mfx_enc_ = NULL;
        }

#ifdef ENABLE_VPX_CODEC
    } else if (ELEMENT_VP8_ENC == element_type_) {
        if (user_enc_) {
            ((VP8EncPlugin *)user_enc_)->Close();
            MFXVideoUSER_Unregister(*mfx_session_, 0);
            delete user_enc_;
            user_enc_ = NULL;
        }

#endif
#ifdef ENABLE_SW_CODEC
    } else if (ELEMENT_SW_ENC == element_type_) {
        if (user_sw_enc_) {
            ((SWEncPlugin *)user_sw_enc_)->Close();
            MFXVideoUSER_Unregister(*mfx_session_, 0);
            delete user_sw_enc_;
            user_sw_enc_ = NULL;
        }

#endif
#ifdef ENABLE_RAW_CODEC
    } else if (ELEMENT_YUV_WRITER == element_type_) {
        if (user_yuv_writer_) {
            ((YUVWriterPlugin *)user_yuv_writer_)->Close();
            MFXVideoUSER_Unregister(*mfx_session_, 0);
            delete user_yuv_writer_;
            user_yuv_writer_ = NULL;
        }

#endif
    }

    if (!m_EncExtParams.empty()) {
        m_EncExtParams.clear();
    }

    if (!m_DecExtParams.empty()) {
        m_DecExtParams.clear();
    }

    for (unsigned int i = 0; i < num_of_surf_; i++) {
        //when element is deleted, all pre-allocated surfaces should have already been recycled.
        //dj: workaround here, surface not unlocked by msdk. Try "p d <end> q" to reproduce
        if (element_type_ != ELEMENT_DECODER && element_type_ != ELEMENT_STRING_DEC && element_type_ != ELEMENT_BMP_DEC) {
            assert(surface_pool_[i]->Data.Locked == 0);
        }
        delete surface_pool_[i];
        surface_pool_[i] = NULL;
#if VA_ENABLE
        mfxExtBuffer **ext_buf = surface_pool_[i]->ExtParam;
        VAPluginData* plugin_data = (VAPluginData*)(*ext_buf);
        unsigned int* vector = plugin_data->Vector;
        delete vector;
        delete plugin_data;
#endif
    }

    if (surface_pool_) {
        delete surface_pool_;
        surface_pool_ = NULL;
    }

    if (NULL != output_bs_.Data) {
        delete[] output_bs_.Data;
        output_bs_.Data = NULL;
    }

    if (m_pMFXAllocator && !m_bUseOpaqueMemory && (ELEMENT_DECODER == element_type_ || ELEMENT_VP8_DEC == element_type_ || ELEMENT_STRING_DEC == element_type_ || ELEMENT_BMP_DEC == element_type_)) {
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxDecResponse);
    }

    if (m_pMFXAllocator && !m_bUseOpaqueMemory && ELEMENT_VPP == element_type_) {
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxEncResponse);
    }
}

void MSDKCodec::SetForceKeyFrame()
{
    if (ELEMENT_ENCODER != element_type_ && ELEMENT_VP8_ENC != element_type_) {
        return;
    }

    force_key_frame_ = true;
}

void MSDKCodec::SetResetBitrateFlag()
{
    reset_bitrate_flag_ = true;
}

void MSDKCodec::SetBitrate(unsigned short bitrate)
{
    mfx_video_param_.mfx.MaxKbps = mfx_video_param_.mfx.TargetKbps = bitrate;
}

ElementType MSDKCodec::GetCodecType()
{
    return element_type_;
}

/*
 * master_handle: not decoder handle, but its mapped dispatcher's handle
 * Returns:   0 - Successful
 *            1 - Warning, non VPP element
 *           -1 - Invalid inputs
 */
int MSDKCodec::ConfigVppCombo(ComboType combo_type, void *master_handle)
{
    if (ELEMENT_VPP != element_type_) {
        return 1;
    }

    if (combo_type < COMBO_BLOCKS || combo_type > COMBO_CUSTOM) {
        printf("Set invalid combo type\n");
        return -1;
    }

    if (COMBO_MASTER == combo_type && NULL == master_handle) {
        printf("Set invalid combo master handle\n");
        return -1;
    }

    if (combo_type_ != combo_type || (COMBO_MASTER == combo_type && combo_master_ != master_handle)) {
        combo_type_ = combo_type;

        if (COMBO_MASTER == combo_type) {
            combo_master_ = master_handle;
        }

        //For COMBO_CUSTOM, wait for SetRegionInfo() to re-init VPP.
        if (COMBO_CUSTOM != combo_type) {
            reinit_vpp_flag_ = true;
        }
        printf("Set VPP combo type=%d, master=%p\n", combo_type, master_handle);
    }

    return 0;
}

bool MSDKCodec::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    ElementCfg *ele_param = static_cast<ElementCfg *>(cfg);

    if (NULL == ele_param) {
        return false;
    }

    measuremnt = ele_param->measuremnt;

    if (ELEMENT_DECODER == element_type_) {
        mfx_dec_ = new MFXVideoDECODE(*mfx_session_);
        mfx_video_param_.mfx.CodecId = ele_param->DecParams.mfx.CodecId;

        // to enable decorative flags, has effect with 1.3 libraries only
        // (in case of JPEG decoder - it is not valid to use this field)
        //if (mfx_video_param_.mfx.CodecId != MFX_CODEC_JPEG) {
        if (mfx_video_param_.mfx.CodecId == MFX_CODEC_AVC || \
                mfx_video_param_.mfx.CodecId == MFX_CODEC_MPEG2 || \
                mfx_video_param_.mfx.CodecId == MFX_CODEC_VC1) {
            mfx_video_param_.mfx.ExtendedPicStruct = 1;
        }

        if (!m_bUseOpaqueMemory) {
            mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        } else {
            mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
        }

        mfx_video_param_.AsyncDepth = 1;
        input_mp_ = ele_param->input_stream;
        input_bs_.MaxLength = input_mp_->GetTotalBufSize();
        input_bs_.Data = input_mp_->GetReadPtr();
#ifdef ENABLE_VPX_CODEC
    } else if (ELEMENT_VP8_DEC == element_type_) {
        user_dec_ = new VP8DecPlugin();
        mfxPlugin dec_plugin = make_mfx_plugin_adapter(user_dec_);
        MFXVideoUSER_Register(*mfx_session_, 0, &dec_plugin);

        //mfx_video_param_.mfx.CodecId = ele_param->DecParams.mfx.CodecId;
        if (!m_bUseOpaqueMemory) {
            mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        } else {
            mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
        }

        mfx_video_param_.AsyncDepth = 1;
        input_mp_ = ele_param->input_stream;
        input_bs_.MaxLength = input_mp_->GetTotalBufSize();
        input_bs_.Data = input_mp_->GetReadPtr();
#endif
#ifdef ENABLE_STRING_CODEC
    } else if (ELEMENT_STRING_DEC == element_type_) {
        str_dec_ = new StringDecPlugin();
        mfxPlugin dec_plugin = make_mfx_plugin_adapter(str_dec_);
        MFXVideoUSER_Register(*mfx_session_, 0, &dec_plugin);

        if (!m_bUseOpaqueMemory) {
            mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        } else {
            mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
        }

        mfx_video_param_.AsyncDepth = 1;
        input_str_ = ele_param->input_str;
#endif
#ifdef ENABLE_PICTURE_CODEC
    } else if (ELEMENT_BMP_DEC == element_type_) {
        pic_dec_ = new PicDecPlugin();
        mfxPlugin dec_plugin = make_mfx_plugin_adapter(pic_dec_);
        MFXVideoUSER_Register(*mfx_session_, 0, &dec_plugin);

        if (!m_bUseOpaqueMemory) {
            mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        } else {
            mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
        }

        mfx_video_param_.AsyncDepth = 1;
        input_pic_ = ele_param->input_pic;
#endif
#ifdef ENABLE_SW_CODEC
    } else if (ELEMENT_SW_DEC == element_type_) {
        user_sw_dec_ = new SWDecPlugin();
        mfxPlugin dec_plugin = make_mfx_plugin_adapter(user_sw_dec_);
        MFXVideoUSER_Register(*mfx_session_, 0, &dec_plugin);
        mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
        mfx_video_param_.AsyncDepth = 1;
        input_mp_ = ele_param->input_stream;
        input_bs_.MaxLength = input_mp_->GetTotalBufSize();
        input_bs_.Data = input_mp_->GetReadPtr();
#endif
    } else if (ELEMENT_VPP == element_type_) {
        mfx_vpp_ = new MFXVideoVPP(*mfx_session_);

        if (ele_param->VppParams.vpp.Out.FourCC == 0) {
            mfx_video_param_.vpp.Out.FourCC        = MFX_FOURCC_NV12;
        } else {
            mfx_video_param_.vpp.Out.FourCC        = ele_param->VppParams.vpp.Out.FourCC;
        }

        //printf("FourCC is %d\n", mfx_video_param_.vpp.Out.FourCC);

        if (mfx_video_param_.vpp.Out.FourCC == MFX_FOURCC_NV12) {
            mfx_video_param_.vpp.Out.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
        }

        mfx_video_param_.vpp.Out.CropX         = ele_param->VppParams.vpp.Out.CropX;
        mfx_video_param_.vpp.Out.CropY         = ele_param->VppParams.vpp.Out.CropY;
        mfx_video_param_.vpp.Out.CropW         = ele_param->VppParams.vpp.Out.CropW;
        mfx_video_param_.vpp.Out.CropH         = ele_param->VppParams.vpp.Out.CropH;
        mfx_video_param_.vpp.Out.PicStruct     = ele_param->VppParams.vpp.Out.PicStruct;
        mfx_video_param_.vpp.Out.FrameRateExtN = ele_param->VppParams.vpp.Out.FrameRateExtN;
        mfx_video_param_.vpp.Out.FrameRateExtD = ele_param->VppParams.vpp.Out.FrameRateExtD;
        mfx_video_param_.AsyncDepth = 1;

        bg_color_info.Y = 0;
        bg_color_info.U = 128;
        bg_color_info.V = 128;

#ifdef ENABLE_VA
    }  else if (ELEMENT_VA == element_type_) {
        user_va_ = new Analyze();
        // Register plugin, acquire mfxCore interface which will be needed in Init
        mfxPlugin va_plugin = make_mfx_plugin_adapter(user_va_);
        mfxStatus sts = MFXVideoUSER_Register(*mfx_session_, 0, &va_plugin);

        if (sts != MFX_ERR_NONE) {
            printf("Register va plugin failed\n");
        }

        output_stream_ = ele_param->output_stream;
        VAPluginParam *va_param;
        va_param = new VAPluginParam();
        va_param->bDump = ele_param->bDump;
        va_param->bOpenCLEnable = ele_param->bOpenCLEnable;
        va_param->VaInterval = ele_param->va_interval;
        va_param->VaType = ele_param->va_type;
        aux_param_ = (void *)va_param;
        aux_param_size_ = sizeof(VAPluginParam);
    }  else if (ELEMENT_RENDER == element_type_) {
        m_hwdev = ele_param->hwdev;
#endif
    } else if (ELEMENT_ENCODER == element_type_) {
        mfx_enc_ = new MFXVideoENCODE(*mfx_session_);
        mfx_video_param_.mfx.CodecId = ele_param->EncParams.mfx.CodecId;
        mfx_video_param_.mfx.CodecProfile = ele_param->EncParams.mfx.CodecProfile;
        mfx_video_param_.mfx.CodecLevel = ele_param->EncParams.mfx.CodecLevel;
        /* Encoding Options */
        mfx_video_param_.mfx.TargetUsage = ele_param->EncParams.mfx.TargetUsage;
        mfx_video_param_.mfx.GopPicSize = ele_param->EncParams.mfx.GopPicSize;
        mfx_video_param_.mfx.GopRefDist = ele_param->EncParams.mfx.GopRefDist;
        mfx_video_param_.mfx.IdrInterval = ele_param->EncParams.mfx.IdrInterval;
        mfx_video_param_.mfx.RateControlMethod =
            (ele_param->EncParams.mfx.RateControlMethod > 0) ?
            ele_param->EncParams.mfx.RateControlMethod : MFX_RATECONTROL_VBR;

        if (mfx_video_param_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
            mfx_video_param_.mfx.QPI = ele_param->EncParams.mfx.QPI;
            mfx_video_param_.mfx.QPP = ele_param->EncParams.mfx.QPP;
            mfx_video_param_.mfx.QPB = ele_param->EncParams.mfx.QPB;
        } else {
            mfx_video_param_.mfx.TargetKbps = ele_param->EncParams.mfx.TargetKbps;
        }

        mfx_video_param_.mfx.NumSlice = ele_param->EncParams.mfx.NumSlice;
        mfx_video_param_.mfx.NumRefFrame = 1;//ele_param->EncParams.mfx.NumRefFrame;
        mfx_video_param_.mfx.EncodedOrder = ele_param->EncParams.mfx.EncodedOrder;
        mfx_video_param_.AsyncDepth = 1;
        extParams.bMBBRC = ele_param->extParams.bMBBRC;
        extParams.nLADepth = ele_param->extParams.nLADepth;
        //if mfx_video_param_.mfx.NumSlice > 0, don't set MaxSliceSize and give warning.
        if (mfx_video_param_.mfx.NumSlice && ele_param->extParams.nMaxSliceSize) {
            printf("warning: MaxSliceSize is not compatible with NumSlice, discard it\n");
            extParams.nMaxSliceSize = 0;
        } else {
            extParams.nMaxSliceSize = ele_param->extParams.nMaxSliceSize;
        }

        output_stream_ = ele_param->output_stream;
#ifdef ENABLE_VPX_CODEC
    } else if (ELEMENT_VP8_ENC == element_type_) {
        user_enc_ = new VP8EncPlugin();
        mfxPlugin enc_plugin = make_mfx_plugin_adapter(user_enc_);
        MFXVideoUSER_Register(*mfx_session_, 0, &enc_plugin);
        mfx_video_param_.mfx.TargetKbps = ele_param->EncParams.mfx.TargetKbps;
        mfx_video_param_.mfx.GopPicSize = ele_param->EncParams.mfx.GopPicSize;
        mfx_video_param_.Protected = ele_param->EncParams.Protected;
        mfx_video_param_.AsyncDepth = 1;
        mfx_video_param_.mfx.RateControlMethod =
            (ele_param->EncParams.mfx.RateControlMethod > 0) ?
            ele_param->EncParams.mfx.RateControlMethod : MFX_RATECONTROL_VBR;
        output_stream_ = ele_param->output_stream;
#endif
#ifdef ENABLE_SW_CODEC
    } else if (ELEMENT_SW_ENC == element_type_) {
        user_sw_enc_ = new SWEncPlugin();
        mfxPlugin enc_plugin = make_mfx_plugin_adapter(user_sw_enc_);
        MFXVideoUSER_Register(*mfx_session_, 0, &enc_plugin);
        mfx_video_param_.mfx.CodecId = ele_param->EncParams.mfx.CodecId;
        mfx_video_param_.mfx.CodecProfile = ele_param->EncParams.mfx.CodecProfile;
        mfx_video_param_.mfx.CodecLevel = ele_param->EncParams.mfx.CodecLevel;
        /* Encoding Options */
        mfx_video_param_.mfx.TargetUsage = ele_param->EncParams.mfx.TargetUsage;
        mfx_video_param_.mfx.GopPicSize = ele_param->EncParams.mfx.GopPicSize;
        mfx_video_param_.mfx.GopRefDist = ele_param->EncParams.mfx.GopRefDist;
        mfx_video_param_.mfx.IdrInterval = ele_param->EncParams.mfx.IdrInterval;
        mfx_video_param_.mfx.RateControlMethod =
            (ele_param->EncParams.mfx.RateControlMethod > 0) ?
            ele_param->EncParams.mfx.RateControlMethod : MFX_RATECONTROL_VBR;

        if (mfx_video_param_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
            mfx_video_param_.mfx.QPI = ele_param->EncParams.mfx.QPI;
            mfx_video_param_.mfx.QPP = ele_param->EncParams.mfx.QPP;
            mfx_video_param_.mfx.QPB = ele_param->EncParams.mfx.QPB;
        } else {
            mfx_video_param_.mfx.TargetKbps = ele_param->EncParams.mfx.TargetKbps;
        }

        mfx_video_param_.mfx.NumSlice = ele_param->EncParams.mfx.NumSlice;
        mfx_video_param_.mfx.NumRefFrame = 1;//ele_param->EncParams.mfx.NumRefFrame;
        mfx_video_param_.mfx.EncodedOrder = ele_param->EncParams.mfx.EncodedOrder;
        extParams.bMBBRC = ele_param->extParams.bMBBRC;
        extParams.nLADepth = ele_param->extParams.nLADepth;
        mfx_video_param_.AsyncDepth = 1;
        output_stream_ = ele_param->output_stream;
#endif
#ifdef ENABLE_RAW_CODEC
    } else if (ELEMENT_YUV_WRITER == element_type_) {
        user_yuv_writer_ = new YUVWriterPlugin();
        mfxPlugin enc_plugin = make_mfx_plugin_adapter(user_yuv_writer_);
        MFXVideoUSER_Register(*mfx_session_, 0, &enc_plugin);
        mfx_video_param_.AsyncDepth = 1;
        output_stream_ = ele_param->output_stream;
#endif
#ifdef MSDK_FEI
    } else if (ELEMENT_FEI_PREENC == element_type_) {
        printf("Init ELEMENT_FEI_PREENC...\n");

        if (element_mode_ != ELEMENT_MODE_ACTIVE) {
            printf("element PREENC only support active mode!\n");
            return false;
        }

        fei_cb_func_ = ele_param->fei_cb_func;

        if (!fei_cb_func_) {
            printf("Warning: fei call back function is not set\n");
        }

        //mfx_enc_ = new MFXVideoENCODE(*mfx_session_);
        mfx_fei_preenc_ = new MFXVideoENC(*mfx_session_);
        /* Encoding Options */
        mfx_video_param_.mfx.CodecId = MFX_CODEC_AVC;
        mfx_video_param_.mfx.TargetUsage = 0;
        mfx_video_param_.mfx.TargetKbps = 2000;
        mfx_video_param_.mfx.RateControlMethod = MFX_RATECONTROL_CBR;
        mfx_video_param_.mfx.CodecProfile = MFX_PROFILE_AVC_MAIN;
        mfx_video_param_.mfx.CodecLevel = MFX_LEVEL_AVC_3;
        mfx_video_param_.mfx.IdrInterval = 0;
        mfx_video_param_.mfx.NumSlice = 1;
        mfx_video_param_.mfx.NumRefFrame = 1; //current limitation
        mfx_video_param_.mfx.EncodedOrder = 0;
        mfx_video_param_.AsyncDepth = 1;
        memset(&mfx_fei_preenc_ctr_, 0, sizeof (mfxExtFeiPreEncCtrl));
        mfx_fei_preenc_ctr_.Header.BufferId = MFX_EXTBUFF_FEI_PREENC_CTRL;
        mfx_fei_preenc_ctr_.Header.BufferSz = sizeof (mfxExtFeiPreEncCtrl);
        mfx_fei_preenc_ctr_.DisableMVOutput = 0;
        mfx_fei_preenc_ctr_.DisableStatisticsOutput = 0;
        mfx_fei_preenc_ctr_.FTEnable = 0;
        mfx_fei_preenc_ctr_.AdaptiveSearch = 1;
        mfx_fei_preenc_ctr_.LenSP = 57;
        mfx_fei_preenc_ctr_.MBQp = 0;
        mfx_fei_preenc_ctr_.MVPredictor = 0;
        mfx_fei_preenc_ctr_.RefHeight = 40;
        mfx_fei_preenc_ctr_.RefWidth = 48;
        mfx_fei_preenc_ctr_.SubPelMode = 3; //quarter-pixel motion estimation
        mfx_fei_preenc_ctr_.SearchWindow = 0;
        mfx_fei_preenc_ctr_.MaxLenSP = 57;
        mfx_fei_preenc_ctr_.Qp = 26;
        mfx_fei_preenc_ctr_.InterSAD = 2; //haar transform
        mfx_fei_preenc_ctr_.IntraSAD = 2;
        mfx_fei_preenc_ctr_.SubMBPartMask = 0x77; //only 8x8
        inBufsPreEnc[num_ext_in_param_++] = (mfxExtBuffer *) & mfx_fei_preenc_ctr_;
#endif
    } else {
        printf("Element type %d not supported, assert\n", element_type_);
        assert(0);
    }

    return true;
}


int MSDKCodec::Recycle(MediaBuf &buf)
{
    //when this assert happens, check if you return the buf the second time or to incorrect element.
    assert(surface_pool_);
    if (buf.msdk_surface != NULL) {
        unsigned int i;
        for (i = 0; i < num_of_surf_; i++) {
            if (surface_pool_[i] == buf.msdk_surface) {
                mfxFrameSurface1 *msdk_surface = (mfxFrameSurface1 *)buf.msdk_surface;
                msdk_surface->Data.Locked--;
                break;
            }
        }
        //or else the buf is not owned by this element, why did you return it to this element?
        assert(i < num_of_surf_);
    }

#ifdef ENABLE_STRING_CODEC
    if (str_dec_) {
        if (buf.is_eos) {
            printf("Now set EOS to string decoder.\n");
            is_eos_ = true;
        }
    }
#endif
#ifdef ENABLE_PICTURE_CODEC
    if (pic_dec_) {
         if (buf.is_eos) {
            printf("Now set EOS to bitmap decoder.\n");
            is_eos_ = true;
        }
    }
#endif

#ifdef MSDK_FEI

    if (element_type_ == ELEMENT_FEI_PREENC) {
        mfxFrameSurface1 *msdk_surface = (mfxFrameSurface1 *)buf.msdk_surface;

        if (msdk_surface) {
            msdk_surface->Data.Locked--;
        }
    }

#endif
    return 0;
}

int MSDKCodec::ProcessChainDecode()
{
    return -1;
}

int MSDKCodec::SetVppCompParam(mfxExtVPPComposite *vpp_comp)
{
    memset(vpp_comp, 0, sizeof(mfxExtVPPComposite));
    vpp_comp->Header.BufferId = MFX_EXTBUFF_VPP_COMPOSITE;
    vpp_comp->Header.BufferSz = sizeof(mfxExtVPPComposite);
    vpp_comp->NumInputStream = (mfxU16)vpp_comp_map_.size();
    vpp_comp->Y = bg_color_info.Y;
    vpp_comp->U = bg_color_info.U;
    vpp_comp->V = bg_color_info.V;
    vpp_comp->InputStream = new mfxVPPCompInputStream[vpp_comp->NumInputStream];
    memset(vpp_comp->InputStream, 0 , sizeof(mfxVPPCompInputStream) * vpp_comp->NumInputStream);
    printf("About to set composition parameter, number of stream is %d\n",
           vpp_comp->NumInputStream);
    std::map<MediaPad *, VPPCompInfo>::iterator it_comp_info;
    int pad_idx = 0;    //indicate the index in sinkpads(or all inputs)
    int vid_idx = 0;    //indicate the index in video streams(subset of inputs)
    int output_width = mfx_video_param_.vpp.Out.Width;
    int output_height = mfx_video_param_.vpp.Out.Height;
    //3 kinds of inputs: stream(video), string and picture
    int stream_cnt = 0; //count of stream inputs
    int string_cnt = 0; //count of string inputs
    int pic_cnt = 0;    //count of picture inputs

    for (it_comp_info = vpp_comp_map_.begin();
            it_comp_info != vpp_comp_map_.end();
            ++it_comp_info) {
        if (!it_comp_info->second.str_info.text && !it_comp_info->second.pic_info.bmp) {
            stream_cnt++;
        } else if (it_comp_info->second.str_info.text && !it_comp_info->second.pic_info.bmp) {
            string_cnt++;
        } else if (!it_comp_info->second.str_info.text && it_comp_info->second.pic_info.bmp) {
            pic_cnt++;
        } else {
            printf("Input can't be string and picture at the same time.\n");
            assert(0);
            return -1;
        }
    }
    printf("Input comp count is %d,video streams %d,text strings %d, picture %d\n",
           vpp_comp->NumInputStream, stream_cnt_, string_cnt_, pic_cnt_);

    //there should be at least one stream
    assert(stream_cnt);
    stream_cnt_ = stream_cnt;
    string_cnt_ = string_cnt;
    pic_cnt_ = pic_cnt;
    eos_cnt = 0;

    //stream_idx - to indicate stream's index in vpp_comp->InputStream(as that in sinkpads_).
    int *stream_idx;
    stream_idx = new int[stream_cnt];
    std::list<MediaPad *>::iterator it_sinkpad;
    bool is_text;
    bool is_pic;
    VPPCompInfo pad_comp_info;

    //Set the comp info for text&pic inputs, and set stream_idx[] for stream inputs
    for (it_sinkpad = this->sinkpads_.begin(), pad_idx = 0;
            it_sinkpad != this->sinkpads_.end();
            ++it_sinkpad, ++pad_idx) {
        is_text = false;
        is_pic = false;
        pad_comp_info = vpp_comp_map_[*it_sinkpad];

        if (pad_comp_info.str_info.text) {
            is_text = true;
        }
        if (pad_comp_info.pic_info.bmp) {
            is_pic = true;
        }
        if (is_text && is_pic) {
            printf("Input can't be text and picture at the same time.\n");
            delete [] stream_idx;
            return -1;
        }
        if (!is_text && !is_pic) { //input is video stream
            stream_idx[vid_idx] = pad_idx;
            vid_idx++;
        } else if (is_text) { //input is text string
            vpp_comp->InputStream[pad_idx].DstX = pad_comp_info.str_info.pos_x;
            vpp_comp->InputStream[pad_idx].DstY = pad_comp_info.str_info.pos_y;
            vpp_comp->InputStream[pad_idx].DstW = pad_comp_info.str_info.width;
            vpp_comp->InputStream[pad_idx].DstH = pad_comp_info.str_info.height;
            vpp_comp->InputStream[pad_idx].LumaKeyEnable = 1;
            vpp_comp->InputStream[pad_idx].LumaKeyMin = 0;
            vpp_comp->InputStream[pad_idx].LumaKeyMax = 10;
            vpp_comp->InputStream[pad_idx].GlobalAlphaEnable = 1;
            vpp_comp->InputStream[pad_idx].GlobalAlpha = (mfxU16)(pad_comp_info.str_info.alpha * 255);
        } else if (is_pic) { //input is picture (bitmap)
            vpp_comp->InputStream[pad_idx].DstX = pad_comp_info.pic_info.pos_x;
            vpp_comp->InputStream[pad_idx].DstY = pad_comp_info.pic_info.pos_y;
            vpp_comp->InputStream[pad_idx].DstW = pad_comp_info.pic_info.width;
            vpp_comp->InputStream[pad_idx].DstH = pad_comp_info.pic_info.height;
            vpp_comp->InputStream[pad_idx].LumaKeyEnable = 0;
            //vpp_comp->InputStream[pad_idx].LumaKeyMin = 0;
            //vpp_comp->InputStream[pad_idx].LumaKeyMax = 0;
            vpp_comp->InputStream[pad_idx].GlobalAlphaEnable = 1;
            vpp_comp->InputStream[pad_idx].GlobalAlpha = (mfxU16)(pad_comp_info.pic_info.alpha * 255);
        }
    }

    //Set comp info for "stream" inputs according to the combo_type_
    VPPCompInfo *info;
    if (combo_type_ == COMBO_CUSTOM) {
        if (apply_region_info_flag_) {
            //Set VPPCompInfo->VppRect
            for (vid_idx = 0, it_sinkpad = this->sinkpads_.begin();
                 vid_idx < stream_cnt && it_sinkpad != this->sinkpads_.end();
                 ++it_sinkpad) {
                info = &vpp_comp_map_[*it_sinkpad];
                if (!info->str_info.text && !info->pic_info.bmp) {
                    assert(dec_region_map_.find(*it_sinkpad) != dec_region_map_.end());
                    info->dst_rect.x = dec_region_map_[*it_sinkpad].left * output_width;
                    info->dst_rect.y = dec_region_map_[*it_sinkpad].top * output_height;
                    info->dst_rect.w = dec_region_map_[*it_sinkpad].width_ratio * output_width;
                    if (info->dst_rect.w == 0) {
                        info->dst_rect.w = 1;
                    }
                    info->dst_rect.h = dec_region_map_[*it_sinkpad].height_ratio * output_height;
                    if (info->dst_rect.h == 0) {
                        info->dst_rect.h = 1;
                    }

                    vid_idx++;
                }
            }

            apply_region_info_flag_ = false;

            //If apply_region_info_flag_ and res_change_flag_ are enabled at the same time
            if (res_change_flag_) {
                res_change_flag_ = false;
            }
        }

        if (res_change_flag_) {
            assert(old_out_width_ && old_out_height_);
            float width_scale = (float)output_width / old_out_width_;
            float height_scale = (float)output_height / old_out_height_;

            for (vid_idx = 0, it_sinkpad = this->sinkpads_.begin();
                 vid_idx < stream_cnt && it_sinkpad != this->sinkpads_.end();
                 ++it_sinkpad) {
                info = &vpp_comp_map_[*it_sinkpad];
                if (!info->str_info.text && !info->pic_info.bmp) {
                    VppRect *rect = &(info->dst_rect);
                    rect->x = (unsigned int)rect->x * width_scale;
                    rect->y = (unsigned int)rect->y * height_scale;
                    assert(rect->w && rect->h);
                    //if(w==1 && h==1), then this video should be hidden, so don't scale it
                    if (rect->w > 1 && rect->h > 1) {
                        rect->w = (unsigned int)rect->w * width_scale;
                        rect->h = (unsigned int)rect->h * height_scale;
                    }

                    vid_idx++;
                }
            }
        }

        //Set vpp_comp->InputStream
        for (vid_idx = 0, it_sinkpad = this->sinkpads_.begin();
             vid_idx < stream_cnt && it_sinkpad != this->sinkpads_.end();
             ++it_sinkpad) {
            info = &vpp_comp_map_[*it_sinkpad];
            if (!info->str_info.text && !info->pic_info.bmp) {
                vpp_comp->InputStream[vid_idx].DstX = info->dst_rect.x;
                vpp_comp->InputStream[vid_idx].DstY = info->dst_rect.y;
                assert(info->dst_rect.w && info->dst_rect.h);
                vpp_comp->InputStream[vid_idx].DstW = info->dst_rect.w;
                vpp_comp->InputStream[vid_idx].DstH = info->dst_rect.h;

                vid_idx++;
            }
        }
    } else if (combo_type_ == COMBO_MASTER) {
        int master_vid_idx = -1; //master's index in video streams
        int sequence = 0;
        int line_size = 0;

        if (stream_cnt <= 6) {
            line_size = 3;
        } else if (stream_cnt > 6 && stream_cnt <= 16) {
            line_size = stream_cnt / 2;
        }

        //find the index for master stream input
        if (combo_master_) {
            for (it_sinkpad = this->sinkpads_.begin();
                 it_sinkpad != this->sinkpads_.end();
                 it_sinkpad++) {
                pad_comp_info = vpp_comp_map_[*it_sinkpad];
                if (!pad_comp_info.str_info.text && !pad_comp_info.pic_info.bmp) {
                    master_vid_idx++;
                    if ((*it_sinkpad)->get_peer_pad()->get_parent() == combo_master_) {
                        break;
                    }
                }
            }
            assert(it_sinkpad != this->sinkpads_.end());
        } else {
            //happens when master is detached.
            //TODO: how to handle this gently?
            printf("master doesn't exist, set the first input as master\n");
            master_vid_idx = 0;
        }
        assert(master_vid_idx >= 0 && master_vid_idx < stream_cnt);

        for (vid_idx = 0; vid_idx < stream_cnt; vid_idx++) {
            if (vid_idx == master_vid_idx) {
                vpp_comp->InputStream[stream_idx[vid_idx]].DstX = 0;
                vpp_comp->InputStream[stream_idx[vid_idx]].DstY = 0;
                vpp_comp->InputStream[stream_idx[vid_idx]].DstW = output_width * (line_size - 1) / line_size;
                vpp_comp->InputStream[stream_idx[vid_idx]].DstH = output_height * (line_size - 1) / line_size;
                continue;
            }

            sequence = (vid_idx > master_vid_idx) ? vid_idx : (vid_idx + 1);
            if (sequence <= line_size) {
                vpp_comp->InputStream[stream_idx[vid_idx]].DstX = (sequence - 1) * output_width / line_size;
                vpp_comp->InputStream[stream_idx[vid_idx]].DstY = output_height * (line_size - 1) / line_size;
                vpp_comp->InputStream[stream_idx[vid_idx]].DstW = output_width / line_size;
                vpp_comp->InputStream[stream_idx[vid_idx]].DstH = output_height / line_size;
            } else if (sequence < line_size * 2) {
                vpp_comp->InputStream[stream_idx[vid_idx]].DstX = output_width * (line_size - 1) / line_size;
                vpp_comp->InputStream[stream_idx[vid_idx]].DstY = output_height - (sequence - (line_size - 1)) * output_height / line_size;
                vpp_comp->InputStream[stream_idx[vid_idx]].DstW = output_width / line_size;
                vpp_comp->InputStream[stream_idx[vid_idx]].DstH = output_height / line_size;
            }
        }
    } else if (combo_type_ == COMBO_BLOCKS) {
        //to calculate block count for every line/column
        int block_count = 0;
        int i = 0;

        for (i = 0; ; i++) {
            if (stream_cnt > i * i && stream_cnt <= (i + 1) * (i + 1)) {
                block_count = i + 1;
                break;
            }
        }

        int block_width = output_width / block_count;
        int block_height = output_height / block_count;

        printf("COMBO_BLOCKS mode, %d stream(s), %dx%d, block size %dx%d\n",
               stream_cnt_, block_count, block_count, block_width, block_height);

        for (i = 0; i < stream_cnt; i++) {
            vpp_comp->InputStream[stream_idx[i]].DstX = (i % block_count) * block_width;
            vpp_comp->InputStream[stream_idx[i]].DstY = (i / block_count) * block_height;
            vpp_comp->InputStream[stream_idx[i]].DstW = block_width;
            vpp_comp->InputStream[stream_idx[i]].DstH = block_height;
        }
    } else {
        assert(0);
    }

#ifndef NDEBUG
    for (int i = 0; i < stream_cnt; i++) {
        printf("vpp_comp->InputStream[%d].DstX = %d\n", stream_idx[i], vpp_comp->InputStream[stream_idx[i]].DstX);
        printf("vpp_comp->InputStream[%d].DstY = %d\n", stream_idx[i], vpp_comp->InputStream[stream_idx[i]].DstY);
        printf("vpp_comp->InputStream[%d].DstW = %d\n", stream_idx[i], vpp_comp->InputStream[stream_idx[i]].DstW);
        printf("vpp_comp->InputStream[%d].DstH = %d\n", stream_idx[i], vpp_comp->InputStream[stream_idx[i]].DstH);
    }
#endif

    delete [] stream_idx;
    return 0;
}
#ifdef ENABLE_VA
mfxStatus MSDKCodec::InitRender(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}

mfxStatus MSDKCodec::InitVA(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    // 1. prepare the parameter for the msdk codec.
    mfx_video_param_.vpp.In.FourCC         = MFX_FOURCC_RGB4;
    mfx_video_param_.vpp.In.ChromaFormat   = MFX_CHROMAFORMAT_YUV420;
    mfx_video_param_.vpp.In.CropX          = 0;
    mfx_video_param_.vpp.In.CropY          = 0;
    mfx_video_param_.vpp.In.PicStruct      = MFX_PICSTRUCT_PROGRESSIVE;
    mfx_video_param_.vpp.In.FrameRateExtN  = 30;
    mfx_video_param_.vpp.In.FrameRateExtD  = 1;
    mfx_video_param_.vpp.In.CropW = buf.mWidth;
    mfx_video_param_.vpp.In.CropH = buf.mHeight;
    printf("---Init VA, in dst %d/%d\n", buf.mWidth, buf.mHeight);
    mfx_video_param_.vpp.In.Width  = MSDK_ALIGN16(mfx_video_param_.vpp.In.CropW);
    mfx_video_param_.vpp.In.Height = (MFX_PICSTRUCT_PROGRESSIVE == mfx_video_param_.vpp.In.PicStruct) ?
                                     MSDK_ALIGN16(mfx_video_param_.vpp.In.CropH) : MSDK_ALIGN32(mfx_video_param_.vpp.In.CropH);

    if (!m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    } else {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
    }

    mfxFrameAllocRequest VPPRequest[2];// [0] - in, [1] - out
    memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = user_va_->QueryIOSurf(&mfx_video_param_, &(VPPRequest[0]), &(VPPRequest[1]));
    printf("VA suggest number of surfaces is in/out %d/%d\n",
           VPPRequest[0].NumFrameSuggested,
           VPPRequest[1].NumFrameSuggested);

    if (m_bUseOpaqueMemory) {
        mfxExtBuffer *va_ext;
        ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        ext_opaque_alloc_.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
        va_ext = (mfxExtBuffer *)&ext_opaque_alloc_;
        memcpy(&ext_opaque_alloc_.In, &(((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out), sizeof(((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out));
        ext_opaque_alloc_.Out.Surfaces = surface_pool_;
        ext_opaque_alloc_.Out.NumSurface = num_of_surf_;
        ext_opaque_alloc_.Out.Type = VPPRequest[1].Type;
        printf("va surface pool %x, num_of_surf_ %d, type %x\n", surface_pool_, num_of_surf_, VPPRequest[1].Type);
        mfx_video_param_.ExtParam = &va_ext;
        mfx_video_param_.NumExtParam = 1;
    } else {
        mfx_video_param_.NumExtParam = 0;
        mfx_video_param_.ExtParam = NULL;
    }

    // 2. allocate the parameter for the element.
    // Init plugin
    sts = user_va_->Init(&mfx_video_param_);

    if (sts != MFX_ERR_NONE) {
        return sts;
    }

    if (aux_param_size_ != 0) {
        sts = user_va_->SetAuxParams(aux_param_, aux_param_size_);

        if (sts != MFX_ERR_NONE) {
            return sts;
        }
    }

    return sts;
}
#endif
mfxStatus MSDKCodec::InitVpp(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpStart(VPP_INIT_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    assert(buf.msdk_surface);
    mfx_video_param_.vpp.In.FrameRateExtN = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtN;
    mfx_video_param_.vpp.In.FrameRateExtD = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtD;
    mfx_video_param_.vpp.In.FourCC        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FourCC;
    mfx_video_param_.vpp.In.ChromaFormat  = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.ChromaFormat;
    mfx_video_param_.vpp.In.PicStruct     = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;;
    mfx_video_param_.vpp.In.CropX         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropX;
    mfx_video_param_.vpp.In.CropY         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropY;
    mfx_video_param_.vpp.In.CropW         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropW;
    mfx_video_param_.vpp.In.CropH         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropH;

    //set output frame rate
    if (!mfx_video_param_.vpp.Out.FrameRateExtN) {
        mfx_video_param_.vpp.Out.FrameRateExtN = mfx_video_param_.vpp.In.FrameRateExtN;
    }

    if (!mfx_video_param_.vpp.Out.FrameRateExtD) {
        mfx_video_param_.vpp.Out.FrameRateExtD = mfx_video_param_.vpp.In.FrameRateExtD;
    }

    printf("VPP Out put frame rate:%f\n", \
           1.0 * mfx_video_param_.vpp.Out.FrameRateExtN / \
           mfx_video_param_.vpp.Out.FrameRateExtD);

    //set vpp out ChromaFormat & FourCC
    if (!mfx_video_param_.vpp.Out.ChromaFormat) {
        mfx_video_param_.vpp.Out.ChromaFormat  = mfx_video_param_.vpp.In.ChromaFormat;
    }

    if (!mfx_video_param_.vpp.Out.FourCC) {
        mfx_video_param_.vpp.Out.FourCC  = mfx_video_param_.vpp.In.FourCC;
    }

    //set vpp PicStruct
    if (!mfx_video_param_.vpp.Out.PicStruct) {
        mfx_video_param_.vpp.Out.PicStruct  = mfx_video_param_.vpp.In.PicStruct;
    }

    printf("---Init VPP, in dst %d/%d\n", mfx_video_param_.vpp.In.CropW, \
           mfx_video_param_.vpp.In.CropH);

    if (mfx_video_param_.vpp.Out.CropW > 4096 || \
            mfx_video_param_.vpp.Out.CropH > 3112 || \
            mfx_video_param_.vpp.Out.CropW <= 0 || \
            mfx_video_param_.vpp.Out.CropH <= 0) {
        printf("Waring: set out put resolution: %dx%d,use original %dx%d\n", \
               mfx_video_param_.vpp.Out.CropW, \
               mfx_video_param_.vpp.Out.CropH, \
               mfx_video_param_.vpp.In.CropW, \
               mfx_video_param_.vpp.In.CropH);
        mfx_video_param_.vpp.Out.CropW = mfx_video_param_.vpp.In.CropW;
        mfx_video_param_.vpp.Out.CropH = mfx_video_param_.vpp.In.CropH;
    }

    // width must be a multiple of 16.
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture.
    mfx_video_param_.vpp.In.Width  = MSDK_ALIGN16(mfx_video_param_.vpp.In.CropW);
    mfx_video_param_.vpp.In.Height = (MFX_PICSTRUCT_PROGRESSIVE == mfx_video_param_.vpp.In.PicStruct) ?
                                     MSDK_ALIGN16(mfx_video_param_.vpp.In.CropH) : MSDK_ALIGN32(mfx_video_param_.vpp.In.CropH);
    mfx_video_param_.vpp.Out.Width  = MSDK_ALIGN16(mfx_video_param_.vpp.Out.CropW);
    mfx_video_param_.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE == mfx_video_param_.vpp.Out.PicStruct) ?
                                      MSDK_ALIGN16(mfx_video_param_.vpp.Out.CropH) : MSDK_ALIGN32(mfx_video_param_.vpp.Out.CropH);

    if (!m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    } else {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
    }

    mfxFrameAllocRequest VPPRequest[2];// [0] - in, [1] - out
    memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = mfx_vpp_->QueryIOSurf(&mfx_video_param_, VPPRequest);
    //TODO: try to get next element's surfaces;
    num_of_surf_ = VPPRequest[1].NumFrameSuggested + 15;
    printf("VPP suggest number of surfaces is in/out %d/%d\n",
           VPPRequest[0].NumFrameSuggested,
           VPPRequest[1].NumFrameSuggested);
#ifdef MSDK_FEI
    //TODO:
    //For FEI
    VPPRequest[1].Type =
        MFX_MEMTYPE_EXTERNAL_FRAME
        | MFX_MEMTYPE_FROM_DECODE
        | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
#endif

    if (surface_pool_ == NULL) {
        printf("Creating VPP surface pool, surface num %d\n", num_of_surf_);
        sts = AllocFrames(&VPPRequest[1], false);
        frame_width_ = VPPRequest[1].Info.Width;
        frame_height_ = VPPRequest[1].Info.Height;
    } else {
        //if VPP re-init without res change, don't do the following
        if (res_change_flag_) {
            //Re-use pre-allocated frames. For res changes, it decreases or increases
            //resolution up to the size of pre-allocated frames.
            unsigned i;
            for (i = 0; i < num_of_surf_; i++) {
                while (surface_pool_[i]->Data.Locked) {
                    usleep(1000);
                }

                memcpy(&(surface_pool_[i]->Info), &(VPPRequest[1].Info), sizeof(mfxFrameInfo));
            }
        }
    }


    mfxExtVPPComposite vpp_comp;
    SetVppCompParam(&vpp_comp);

    if (m_bUseOpaqueMemory) {
        mfxExtBuffer *vpp_ext[2];
        ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        ext_opaque_alloc_.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
        vpp_ext[0] = (mfxExtBuffer *)&ext_opaque_alloc_;
        vpp_ext[1] = (mfxExtBuffer *)&vpp_comp;
        memcpy(&ext_opaque_alloc_.In, \
               & (((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out), \
               sizeof(((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out));
        ext_opaque_alloc_.Out.Surfaces = surface_pool_;
        ext_opaque_alloc_.Out.NumSurface = num_of_surf_;
        ext_opaque_alloc_.Out.Type = VPPRequest[1].Type;
        printf("vpp surface pool %p, num_of_surf_ %d, type %x\n", surface_pool_, num_of_surf_, VPPRequest[1].Type);
        mfx_video_param_.ExtParam = vpp_ext;

        if (sinkpads_.size() == 1) {
            mfx_video_param_.NumExtParam = 1;
        } else {
            mfx_video_param_.NumExtParam = 2;
        }
    } else {
        mfx_video_param_.NumExtParam = 0;

        if (sinkpads_.size() == 1) {
            mfx_video_param_.ExtParam = NULL;
        } else {
            mfx_video_param_.ExtParam = (mfxExtBuffer **)pExtBuf;
            mfx_video_param_.ExtParam[mfx_video_param_.NumExtParam++] = (mfxExtBuffer *) & (vpp_comp);
        }
    }

    sts = mfx_vpp_->Init(&mfx_video_param_);

    if (MFX_WRN_FILTER_SKIPPED == sts) {
        printf("------------------- Got MFX_WRN_FILTER_SKIPPED\n");
        sts = MFX_ERR_NONE;
    }

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpFinish(VPP_INIT_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    delete[] vpp_comp.InputStream;
    return sts;
}

mfxStatus MSDKCodec::InitEncoder(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest EncRequest;
    mfxVideoParam par;

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpStart(ENC_INIT_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    mfx_video_param_.mfx.FrameInfo.FrameRateExtN = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtN;
    mfx_video_param_.mfx.FrameInfo.FrameRateExtD = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtD;
    mfx_video_param_.mfx.FrameInfo.FourCC        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FourCC;
    mfx_video_param_.mfx.FrameInfo.ChromaFormat  = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.ChromaFormat;
    mfx_video_param_.mfx.FrameInfo.CropX         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropX;
    mfx_video_param_.mfx.FrameInfo.CropY         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropY;
    mfx_video_param_.mfx.FrameInfo.CropW         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropW;
    mfx_video_param_.mfx.FrameInfo.CropH         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropH;
    mfx_video_param_.mfx.FrameInfo.Width         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Width;
    mfx_video_param_.mfx.FrameInfo.Height        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Height;
    if (((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_TFF || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_BFF) {
        mfx_video_param_.mfx.FrameInfo.PicStruct     = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;
    } else {
        mfx_video_param_.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    // Hack for MPEG2
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_MPEG2) {
        float result = mfx_video_param_.mfx.FrameInfo.FrameRateExtN;
        result = round(result / mfx_video_param_.mfx.FrameInfo.FrameRateExtD);
        mfx_video_param_.mfx.FrameInfo.FrameRateExtN = result;
        mfx_video_param_.mfx.FrameInfo.FrameRateExtD = 1;

        float frame_rate = mfx_video_param_.mfx.FrameInfo.FrameRateExtN / \
                           mfx_video_param_.mfx.FrameInfo.FrameRateExtD;
        if ((frame_rate != 24) && (frame_rate != 25) && (frame_rate != 30) && \
                (frame_rate != 50) && (frame_rate != 60)) {
            printf("Waring: mpeg2 encoder frame rate %f, use default 30fps\n", frame_rate);
            mfx_video_param_.mfx.FrameInfo.FrameRateExtN = 30;
            mfx_video_param_.mfx.FrameInfo.FrameRateExtD = 1;
        }
    }

    if ((mfx_video_param_.mfx.RateControlMethod != MFX_RATECONTROL_CQP) && \
            !mfx_video_param_.mfx.TargetKbps) {
        mfx_video_param_.mfx.TargetKbps = CalDefBitrate(mfx_video_param_.mfx.CodecId, \
            mfx_video_param_.mfx.TargetUsage, \
            mfx_video_param_.mfx.FrameInfo.Width, \
            mfx_video_param_.mfx.FrameInfo.Height, \
            1.0 * mfx_video_param_.mfx.FrameInfo.FrameRateExtN / \
            mfx_video_param_.mfx.FrameInfo.FrameRateExtD);
        printf("Waring: invalid setup bitrate,use default %d Kbps\n", \
               mfx_video_param_.mfx.TargetKbps);
    }

#ifdef ENABLE_ASTA_CONFIG
    //extcoding options for instructing encoder to specify maxdecodebuffering=1
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_AVC) {
        coding_opt.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
        coding_opt.Header.BufferSz = sizeof(coding_opt);
        coding_opt.AUDelimiter = MFX_CODINGOPTION_OFF;//No AUD
        coding_opt.RecoveryPointSEI = MFX_CODINGOPTION_OFF;//No SEI
        coding_opt.PicTimingSEI = MFX_CODINGOPTION_OFF;
        //when cVuiNalHrdParameters = MFX_CODINGOPTION_OFF
        //cause that the ouptu file unusual when setup the NumSlice > 1
        coding_opt.VuiNalHrdParameters = MFX_CODINGOPTION_ON;
        coding_opt.VuiVclHrdParameters = MFX_CODINGOPTION_OFF;
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&coding_opt));
    }
#endif

    //extcoding option2
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_AVC) {
        coding_opt2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
        coding_opt2.Header.BufferSz = sizeof(coding_opt2);
        coding_opt2.RepeatPPS = MFX_CODINGOPTION_OFF;//No repeat pps

        if (extParams.bMBBRC) {
            coding_opt2.MBBRC = MFX_CODINGOPTION_ON;
        }

        if ((mfx_video_param_.mfx.RateControlMethod == MFX_RATECONTROL_LA) && \
                extParams.nLADepth) {
            coding_opt2.LookAheadDepth = extParams.nLADepth;
        }

        //max slice size setting, supported for AVC only, not compatible with "slice num"
        if (extParams.nMaxSliceSize) {
            coding_opt2.MaxSliceSize = extParams.nMaxSliceSize;
        }

        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&coding_opt2));
    }

    if (m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;
        //opaue alloc
        ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        ext_opaque_alloc_.Header.BufferSz = sizeof(ext_opaque_alloc_);
        memcpy(&ext_opaque_alloc_.In, \
               & ((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out, \
               sizeof(((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out));
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&ext_opaque_alloc_));

    } else {
#ifdef MSDK_FEI
        memset(&mfx_ext_fei_param_, 0, sizeof(mfx_ext_fei_param_));
        mfx_ext_fei_param_.Header.BufferId = MFX_EXTBUFF_FEI_PARAM;
        mfx_ext_fei_param_.Header.BufferSz = sizeof (mfxExtFeiParam);
        mfx_ext_fei_param_.Func = MFX_FEI_FUNCTION_ENCPAK;
        m_EncExtParams.push_back((mfxExtBuffer *)&mfx_ext_fei_param_);
        memset(&mfx_fei_enc_ctrl_, 0, sizeof (mfxExtFeiEncFrameCtrl));
        mfx_fei_enc_ctrl_.Header.BufferId = MFX_EXTBUFF_FEI_ENC_CTRL;
        mfx_fei_enc_ctrl_.Header.BufferSz = sizeof (mfxExtFeiEncFrameCtrl);
        mfx_fei_enc_ctrl_.MaxLenSP = 47;
        mfx_fei_enc_ctrl_.LenSP = 47;
        mfx_fei_enc_ctrl_.SubMBPartMask = 0xf;
        mfx_fei_enc_ctrl_.MultiPredL0 = 0;
        mfx_fei_enc_ctrl_.MultiPredL1 = 0;
        mfx_fei_enc_ctrl_.SubPelMode = 3;
        mfx_fei_enc_ctrl_.InterSAD = 2;
        mfx_fei_enc_ctrl_.IntraSAD = 2;
        mfx_fei_enc_ctrl_.DistortionType = 2;
        mfx_fei_enc_ctrl_.RepartitionCheckEnable = 0;
        mfx_fei_enc_ctrl_.AdaptiveSearch = 0;
        mfx_fei_enc_ctrl_.MVPredictor = 0;
        mfx_fei_enc_ctrl_.NumMVPredictors = 4; //always 4 predictors
        mfx_fei_enc_ctrl_.PerMBQp = 0;
        mfx_fei_enc_ctrl_.PerMBInput = 0;
        mfx_fei_enc_ctrl_.MBSizeCtrl = 0;
        mfx_fei_enc_ctrl_.RefHeight = 48;
        mfx_fei_enc_ctrl_.RefWidth = 48;
        mfx_fei_enc_ctrl_.SearchWindow = 1;
        inBufs[num_ext_in_param_++] = (mfxExtBuffer *) & mfx_fei_enc_ctrl_;
        mfx_fei_ctrl_.FrameType = MFX_FRAMETYPE_UNKNOWN;
        mfx_fei_ctrl_.QP = 27;
        mfx_fei_ctrl_.SkipFrame = 0;
        mfx_fei_ctrl_.NumPayload = 0;
        mfx_fei_ctrl_.Payload = NULL;
        mfx_fei_ctrl_.NumExtParam = num_ext_in_param_;
        mfx_fei_ctrl_.ExtParam = &inBufs[0];
        printf("----ext buffer count %d\n", num_ext_in_param_);
#endif
        printf("------------Encoder using video memory\n");
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }

    //set mfx video ExtParam
    if (!m_EncExtParams.empty()) {
        mfx_video_param_.ExtParam = &m_EncExtParams.front(); // vector is stored linearly in memory
        mfx_video_param_.NumExtParam = (mfxU16)m_EncExtParams.size();
        printf("init encoder, ext param number is %d\n", mfx_video_param_.NumExtParam);
    }

    memset(&EncRequest, 0, sizeof(EncRequest));

    if (ELEMENT_ENCODER == element_type_) {
        sts = mfx_enc_->QueryIOSurf(&mfx_video_param_, &EncRequest);
#ifdef ENABLE_VPX_CODEC
    } else if (ELEMENT_VP8_ENC == element_type_) {
        sts = ((VP8EncPlugin *)user_enc_)->QueryIOSurf(&mfx_video_param_, &EncRequest, NULL);
#endif
#ifdef ENABLE_SW_CODEC
    } else if (ELEMENT_SW_ENC == element_type_) {
        sts = ((SWEncPlugin *)user_sw_enc_)->QueryIOSurf(&mfx_video_param_, &EncRequest, NULL);
#endif
#ifdef ENABLE_RAW_CODEC
    } else if (ELEMENT_YUV_WRITER == element_type_) {
        sts = ((YUVWriterPlugin *)user_yuv_writer_)->QueryIOSurf(&mfx_video_param_, \
                &EncRequest, NULL);
#endif
    } else {
        sts = mfx_enc_->QueryIOSurf(&mfx_video_param_, &EncRequest);
    }

    printf("Enc suggest surfaces %d\n", EncRequest.NumFrameSuggested);
    assert(sts >= MFX_ERR_NONE);
    memset(&par, 0, sizeof(par));

    if (ELEMENT_ENCODER == element_type_) {
        sts = mfx_enc_->Init(&mfx_video_param_);

        if (sts != MFX_ERR_NONE) {
            printf("\t\t------------enc init return %d\n", sts);
            assert(sts >= 0);
        }

        sts = mfx_enc_->GetVideoParam(&par);
#ifdef ENABLE_VPX_CODEC
    } else if (ELEMENT_VP8_ENC == element_type_) {
        sts = ((VP8EncPlugin *)user_enc_)->Init(&mfx_video_param_);
        ((VP8EncPlugin *)user_enc_)->SetAllocPoint(m_pMFXAllocator);
        sts = ((VP8EncPlugin *)user_enc_)->GetVideoParam(&par);
#endif
#ifdef ENABLE_SW_CODEC
    } else if (ELEMENT_SW_ENC == element_type_) {
        sts = ((SWEncPlugin *)user_sw_enc_)->Init(&mfx_video_param_);
        sts = ((SWEncPlugin *)user_sw_enc_)->GetVideoParam(&par);
#endif
#ifdef ENABLE_RAW_CODEC
    } else if (ELEMENT_YUV_WRITER == element_type_) {
        sts = ((YUVWriterPlugin *)user_yuv_writer_)->Init(&mfx_video_param_);
        ((YUVWriterPlugin *)user_yuv_writer_)->SetAllocPoint(m_pMFXAllocator);
        sts = ((YUVWriterPlugin *)user_yuv_writer_)->GetVideoParam(&par);
#endif
    } else {
        sts = mfx_enc_->Init(&mfx_video_param_);
        sts = mfx_enc_->GetVideoParam(&par);
    }

    // Prepare Media SDK bit stream buffer for encoder
    memset(&output_bs_, 0, sizeof(output_bs_));
    output_bs_.MaxLength = par.mfx.BufferSizeInKB * 2000;
    output_bs_.Data = new mfxU8[output_bs_.MaxLength];
    printf("output bitstream buffer size %d\n", output_bs_.MaxLength);

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpFinish(ENC_INIT_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    return sts;
}

int MSDKCodec::ProcessChainEncode(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pmfxInSurface = (mfxFrameSurface1 *)buf.msdk_surface;
    mfxSyncPoint syncpE;
    mfxEncodeCtrl *enc_ctrl = NULL;

    if (!codec_init_) {
        sts = InitEncoder(buf);

        if (MFX_ERR_NONE == sts) {
            codec_init_ = true;

            if (measuremnt) {
                measuremnt->GetLock();
                measuremnt->TimeStpStart(ENC_ENDURATION_TIME_STAMP, this);
                pipelineinfo einfo;
                einfo.mElementType = element_type_;
                einfo.mChannelNum = MSDKCodec::mNumOfEncChannels;
                MSDKCodec::mNumOfEncChannels++;
                if(ELEMENT_VP8_ENC == element_type_) {
                    einfo.mCodecName = "VP8";
                }
                else {
                    switch (mfx_video_param_.mfx.CodecId) {
                        case MFX_CODEC_AVC:
                            einfo.mCodecName = "AVC";
                            break;
                        case MFX_CODEC_MPEG2:
                            einfo.mCodecName = "MPEG2";
                            break;
                        case MFX_CODEC_VC1:
                            einfo.mCodecName = "VC1";
                            break;
                        default:
                            einfo.mCodecName = "unkown codec";
                            break;
                    }
                }

                measuremnt->SetElementInfo(ENC_ENDURATION_TIME_STAMP, this, &einfo);
                measuremnt->RelLock();
            }

            printf("Encoder %p init successfully\n", this);
        } else {
            printf("encoder create failed %d\n", sts);
            return -1;
        }
    }

    // to check if there is res change in incoming surface
    if (pmfxInSurface) {
        if (mfx_video_param_.mfx.FrameInfo.Width  != pmfxInSurface->Info.Width ||
            mfx_video_param_.mfx.FrameInfo.Height != pmfxInSurface->Info.Height) {
            mfx_video_param_.mfx.FrameInfo.Width   = pmfxInSurface->Info.Width;
            mfx_video_param_.mfx.FrameInfo.Height  = pmfxInSurface->Info.Height;
            mfx_video_param_.mfx.FrameInfo.CropW   = pmfxInSurface->Info.CropW;
            mfx_video_param_.mfx.FrameInfo.CropH   = pmfxInSurface->Info.CropH;

            reset_res_flag_ = true;
            printf("Info: to reset encoder res as %d x %d\n",
                   mfx_video_param_.mfx.FrameInfo.Width, mfx_video_param_.mfx.FrameInfo.Height);
        }
    }

    if (force_key_frame_) {
        if (ELEMENT_ENCODER == element_type_) {
            memset(&enc_ctrl_, 0, sizeof(enc_ctrl_));
            enc_ctrl_.FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;
            enc_ctrl = &enc_ctrl_;
#ifdef ENABLE_VPX_CODEC
        } else {
            ((VP8EncPlugin *)user_enc_)->VP8ForceKeyFrame();
#endif
        }

        force_key_frame_ = false;
    }

    if (reset_bitrate_flag_) {
        if (ELEMENT_ENCODER == element_type_) {
            mfxVideoParam par;
            memset(&par, 0, sizeof(par));
            mfx_enc_->GetVideoParam(&par);
            par.mfx.TargetKbps = mfx_video_param_.mfx.TargetKbps;

            //need to complete encoding with current settings
            for (;;) {
                for (;;) {
                    sts = mfx_enc_->EncodeFrameAsync(enc_ctrl, NULL, &output_bs_, &syncpE);

                    if (MFX_ERR_NONE < sts && !syncpE) {
                        if (MFX_WRN_DEVICE_BUSY == sts) {
                            usleep(20);
                        }
                    } else if (MFX_ERR_NONE < sts && syncpE) {
                        // ignore warnings if output is available
                        sts = MFX_ERR_NONE;
                        break;
                    } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                        // Allocate more bitstream buffer memory here if needed...
                        break;
                    } else {
                        break;
                    }
                }

                if (sts == MFX_ERR_MORE_DATA) {
                    break;
                }
            }

            mfx_enc_->Reset(&par);
#ifdef ENABLE_VPX_CODEC
        } else {
            ((VP8EncPlugin *)user_enc_)->VP8ResetBitrate(mfx_video_param_.mfx.TargetKbps);
#endif
        }

        reset_bitrate_flag_ = false;
    }

    if (reset_res_flag_) {
        if (ELEMENT_ENCODER == element_type_) {
            mfxVideoParam par;
            memset(&par, 0, sizeof(par));
            mfx_enc_->GetVideoParam(&par);
            par.mfx.FrameInfo.CropW = mfx_video_param_.mfx.FrameInfo.CropW;
            par.mfx.FrameInfo.CropH = mfx_video_param_.mfx.FrameInfo.CropH;
            par.mfx.FrameInfo.Width = mfx_video_param_.mfx.FrameInfo.CropW;
            par.mfx.FrameInfo.Height = mfx_video_param_.mfx.FrameInfo.CropH;

            //need to complete encoding with current settings
            for (;;) {
                for (;;) {
                    sts = mfx_enc_->EncodeFrameAsync(enc_ctrl, NULL, &output_bs_, &syncpE);

                    if (MFX_ERR_NONE < sts && !syncpE) {
                        if (MFX_WRN_DEVICE_BUSY == sts) {
                            usleep(20);
                        }
                    } else if (MFX_ERR_NONE < sts && syncpE) {
                        // ignore warnings if output is available
                        sts = MFX_ERR_NONE;
                        break;
                    } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                        // Allocate more bitstream buffer memory here if needed...
                        break;
                    } else {
                        break;
                    }
                }

                if (sts == MFX_ERR_MORE_DATA) {
                    break;
                }
            }

            mfx_enc_->Reset(&par);
#ifdef ENABLE_VPX_CODEC
        } else {
            ((VP8EncPlugin *)user_enc_)->VP8ResetRes(mfx_video_param_.mfx.FrameInfo.CropW,
                                                     mfx_video_param_.mfx.FrameInfo.CropH);
#endif
        }

        reset_res_flag_ = false;
    }

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpStart(ENC_FRAME_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    do {
        while (is_running_) {
            if (ELEMENT_ENCODER == element_type_) {
#ifdef MSDK_FEI

                if (buf.is_key_frame == 1) {
                    mfx_fei_ctrl_.FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;
                } else {
                    mfx_fei_ctrl_.FrameType = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
                }

                sts = mfx_enc_->EncodeFrameAsync(&mfx_fei_ctrl_, pmfxInSurface,
                                                 &output_bs_, &syncpE);
#else
                sts = mfx_enc_->EncodeFrameAsync(enc_ctrl, pmfxInSurface,
                                                 &output_bs_, &syncpE);
#endif
            } else {
                sts = MFXVideoUSER_ProcessFrameAsync(*mfx_session_,
                                                     (mfxHDL *) &pmfxInSurface, 1, (mfxHDL *) &output_bs_, 1, &syncpE);
            }

            if (MFX_ERR_NONE < sts && !syncpE) {
                if (MFX_WRN_DEVICE_BUSY == sts) {
                    usleep(20);
                }
            } else if (MFX_ERR_NONE < sts && syncpE) {
                // ignore warnings if output is available
                sts = MFX_ERR_NONE;
                break;
            } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                // Allocate more bitstream buffer memory here if needed...
                break;
            } else {
                break;
            }
        }

        if (MFX_ERR_NONE == sts) {
            sts = mfx_session_->SyncOperation(syncpE, 60000);

            if (measuremnt) {
                measuremnt->GetLock();
                measuremnt->TimeStpFinish(ENC_FRAME_TIME_STAMP, this);
                measuremnt->RelLock();
            }

            if (output_stream_) {
                sts = WriteBitStreamFrame(&output_bs_, output_stream_);
            }
        }
    } while (pmfxInSurface == NULL && sts >= MFX_ERR_NONE);

    if (NULL == pmfxInSurface && buf.is_eos && output_stream_ != NULL) {
        output_stream_->SetEndOfStream();

        if (measuremnt) {
            measuremnt->GetLock();
            measuremnt->TimeStpFinish(ENC_ENDURATION_TIME_STAMP, this);
            measuremnt->RelLock();
        }

        printf("Got EOF in Encoder %p\n", this);
        is_running_ = false;
    }

    //printf("leave ProcessChainEncode\n");
    return 0;
}

mfxStatus MSDKCodec::WriteBitStreamFrame(mfxBitstream *pMfxBitstream,
        Stream *out_stream)
{
    mfxU32 nBytesWritten = (mfxU32)out_stream->WriteBlock(
                               pMfxBitstream->Data + pMfxBitstream->DataOffset,
                               pMfxBitstream->DataLength);

    if (nBytesWritten != pMfxBitstream->DataLength) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    pMfxBitstream->DataLength = 0;
    return MFX_ERR_NONE;
}

mfxStatus MSDKCodec::WriteBitStreamFrame(mfxBitstream *pMfxBitstream,
        FILE *fSink)
{
    mfxU32 nBytesWritten = (mfxU32)fwrite(
                               pMfxBitstream->Data + pMfxBitstream->DataOffset,
                               1, pMfxBitstream->DataLength, fSink);

    if (nBytesWritten != pMfxBitstream->DataLength) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    pMfxBitstream->DataLength = 0;
    return MFX_ERR_NONE;
}

mfxStatus ReadBitStreamData(mfxBitstream *pBS, FILE *fSource)
{
    memmove(pBS->Data, pBS->Data + pBS->DataOffset, pBS->DataLength);
    pBS->DataOffset = 0;
    mfxU32 nBytesRead = (mfxU32)fread(pBS->Data + pBS->DataLength,
                                      1, pBS->MaxLength - pBS->DataLength, fSource);

    if (0 == nBytesRead) {
        return MFX_ERR_MORE_DATA;
    }

    pBS->DataLength += nBytesRead;
    return MFX_ERR_NONE;
}
#ifdef ENABLE_VA
mfxStatus MSDKCodec::DoingRender(mfxFrameSurface1 *in_surf)
{
    mfxStatus sts = MFX_ERR_NONE;
    m_hwdev->RenderFrame(in_surf, m_pMFXAllocator);
    return sts;
}

mfxStatus MSDKCodec::DoingVA(mfxFrameSurface1 *in_surf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxSyncPoint syncpV;

    do {
        for (;;) {
            // Process a frame asychronously (returns immediately).
            sts = MFXVideoUSER_ProcessFrameAsync(*mfx_session_, (mfxHDL *) & (in_surf), 1, NULL, 0, &syncpV);
            assert(sts == MFX_ERR_NONE);
            sts = mfx_session_->SyncOperation(syncpV, 0xffffffff);
            assert(sts == MFX_ERR_NONE);
            if (MFX_ERR_NONE < sts && !syncpV) {
                if (MFX_WRN_DEVICE_BUSY == sts) {
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
                    Sleep(10);
#else
                    usleep(10000);
#endif
                }
            } else if (MFX_ERR_NONE < sts && syncpV) {
                // Ignore warnings if output is available.
                sts = MFX_ERR_NONE;
                break;
            } else {
                break;
            }
        }

        if (MFX_ERR_MORE_DATA == sts && NULL != in_surf) {
            // Composite case, direct return
            break;
        }

        if (MFX_ERR_NONE < sts && syncpV) {
            sts = MFX_ERR_NONE;
        }
        if (sts == MFX_ERR_NONE) {
            if (output_stream_) {
                char buffer[256];
                int len;
                mfxU32 nBytesWritten;
                mfxExtBuffer **ext_buf = in_surf->Data.ExtParam;
                VAPluginData* plugin_data = (VAPluginData*)(*ext_buf);
                mfxU32 size = plugin_data->Size;
                mfxU32 FId = plugin_data->FId;
                mfxU32 ObjectNum = plugin_data->ObjectNum;
                mfxU32 ObjectSize = plugin_data->ObjectSize;
                len = snprintf(buffer, 256, "the frame #%d is detected %d objects\n", FId, ObjectNum);
                if (len > 256) {
                    len = 256;
                }
                nBytesWritten = output_stream_->WriteBlock(buffer, len);
                if (nBytesWritten != len) {
                    sts = MFX_ERR_UNDEFINED_BEHAVIOR;
                }

                for (int i = 0; i < ObjectNum; i++) {
                    unsigned int tl_x, tl_y, br_x, br_y;
                    unsigned int* ptr = plugin_data->Vector + i * 4;
                    tl_x = *ptr;
                    tl_y = *(ptr + 1);
                    br_x = *(ptr + 2);
                    br_y = *(ptr + 3);
                    len = snprintf(buffer, 256, "    tl(%d:%d), br(%d:%d)\n", tl_x, tl_y, br_x, br_y);
                    if (len > 256) {
                        len = 256;
                    }
                    nBytesWritten = output_stream_->WriteBlock(buffer, len);
                    if (nBytesWritten != len) {
                        sts = MFX_ERR_UNDEFINED_BEHAVIOR;
                    }
                }

                len = snprintf(buffer, 256, "\n");
                if (len > 256) {
                    len = 256;
                }
                nBytesWritten = output_stream_->WriteBlock(buffer, len);
                if (nBytesWritten != len) {
                    sts = MFX_ERR_UNDEFINED_BEHAVIOR;
                }
            }
        }
    } while (in_surf == NULL && sts >= MFX_ERR_NONE);

    return sts;
}
#endif

mfxStatus MSDKCodec::DoingVpp(mfxFrameSurface1* in_surf, mfxFrameSurface1* out_surf)
{
    mfxStatus sts = MFX_ERR_NONE;
    MediaBuf out_buf;
    mfxSyncPoint syncpV;
    MediaPad *srcpad = *(this->srcpads_.begin());

    do {
        for (;;) {
            // Process a frame asychronously (returns immediately).
            sts = mfx_vpp_->RunFrameVPPAsync(in_surf,
                                             out_surf, NULL, &syncpV);

            if (MFX_ERR_NONE < sts && !syncpV) {
                if (MFX_WRN_DEVICE_BUSY == sts) {
                    usleep(10000);
                }
            } else if (MFX_ERR_NONE < sts && syncpV) {
                // Ignore warnings if output is available.
                sts = MFX_ERR_NONE;
                break;
            } else {
                break;
            }
        }

        if (MFX_ERR_MORE_DATA == sts && NULL != in_surf) {
            // Composite case, direct return
            break;
        }

        if (MFX_ERR_NONE < sts && syncpV) {
            sts = MFX_ERR_NONE;
        }

        if (MFX_ERR_NONE == sts) {
            out_buf.msdk_surface = out_surf;

            if (out_buf.msdk_surface) {
                // Increase the lock count within the surface
                out_surf->Data.Locked++;
            }

            out_buf.mWidth = mfx_video_param_.vpp.Out.CropW;
            out_buf.mHeight = mfx_video_param_.vpp.Out.CropH;
            out_buf.is_eos = 0;

            if (m_bUseOpaqueMemory) {
                out_buf.ext_opaque_alloc = &ext_opaque_alloc_;
            }

            if (srcpad) {
                srcpad->PushBufToPeerPad(out_buf);
                vppframe_num_++;
            }
        }

        if (MFX_ERR_MORE_DATA == sts && NULL == syncpV) {
            // Is it last frame decoded? Do not push EOS here.
            out_buf.msdk_surface = NULL;
            out_buf.is_eos = 0;

            if (srcpad) {
                srcpad->PushBufToPeerPad(out_buf);
                vppframe_num_++;
            }
        }
    } while (in_surf == NULL && sts >= MFX_ERR_NONE);

    return sts;
}

int MSDKCodec::ProcessChainVpp(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pmfxInSurface = (mfxFrameSurface1 *)buf.msdk_surface;

    if (!codec_init_) {
        sts = InitVpp(buf);

        if (MFX_ERR_NONE == sts) {
            printf("VPP element init successfully\n");
            codec_init_ = true;
        } else {
            printf("vpp create failed %d\n", sts);
            return -1;
        }
    }

    if (buf.is_eos) {
        eos_cnt += buf.is_eos;
        //printf("The EOS number is:%d\n", eos_cnt);
    }
    if (stream_cnt_ == eos_cnt) {
        printf("All video streams EOS, can end VPP now. %d, %d\n", stream_cnt_, eos_cnt);
        WaitSrcMutex();
        MediaBuf out_buf;
        MediaPad *srcpad = *(this->srcpads_.begin());
        out_buf.msdk_surface = NULL;
        out_buf.is_eos = 1;
        if (srcpad) {
            srcpad->PushBufToPeerPad(out_buf);
        }
        ReleaseSrcMutex();
        return 1;
    }

    int nIndex = 0;

    while (is_running_) {
        nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);

        if (MFX_ERR_NOT_FOUND == nIndex) {
            usleep(10000);
            //return MFX_ERR_MEMORY_ALLOC;
        } else {
            break;
        }
    }

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpStart(VPP_FRAME_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    DoingVpp(pmfxInSurface, surface_pool_[nIndex]);

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpFinish(VPP_FRAME_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    return 0;
}

#ifdef ENABLE_VA
int MSDKCodec::ProcessChainRender(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pmfxInSurface = (mfxFrameSurface1 *)buf.msdk_surface;

    if (!codec_init_) {
        sts = InitRender(buf);

        if (MFX_ERR_NONE == sts) {
            printf("Render element init successfully\n");
            codec_init_ = true;
        } else {
            printf("va create failed %d\n", sts);
            return -1;
        }
    }

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpStart(VPP_FRAME_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    DoingRender(pmfxInSurface);

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpFinish(VPP_FRAME_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    return 0;
}

int MSDKCodec::ProcessChainVA(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pmfxInSurface = (mfxFrameSurface1 *)buf.msdk_surface;

    if (!codec_init_) {
        sts = InitVA(buf);

        if (MFX_ERR_NONE == sts) {
            printf("VA element init successfully\n");
            codec_init_ = true;
        } else {
            printf("va create failed %d\n", sts);
            return -1;
        }
    }

    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpStart(VPP_FRAME_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    if (pmfxInSurface) {
        DoingVA(pmfxInSurface);
    }
    if (measuremnt) {
        measuremnt->GetLock();
        measuremnt->TimeStpFinish(VPP_FRAME_TIME_STAMP, this);
        measuremnt->RelLock();
    }

    return 0;
}
#endif
int MSDKCodec::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    if (buf.msdk_surface == NULL && buf.is_eos) {
        printf("Got EOF in element %p, type is %d\n", this, element_type_);
    }

    if (ELEMENT_DECODER == element_type_ || \
            ELEMENT_VP8_DEC == element_type_ || \
            ELEMENT_STRING_DEC == element_type_ || \
            ELEMENT_BMP_DEC == element_type_ || \
            ELEMENT_SW_DEC == element_type_) {
        return ProcessChainDecode();
    } else if (ELEMENT_ENCODER == element_type_ || \
               ELEMENT_VP8_ENC == element_type_ || \
               ELEMENT_SW_ENC == element_type_ || \
               ELEMENT_YUV_WRITER == element_type_) {
        return ProcessChainEncode(buf);
    } else if (ELEMENT_VPP == element_type_) {
        return ProcessChainVpp(buf);
#ifdef ENABLE_VA
    } else if (ELEMENT_VA == element_type_) {
        return ProcessChainVA(buf);
    } else if (ELEMENT_RENDER == element_type_) {
        return ProcessChainRender(buf);
#endif
    } else {
        return -1;
    }
}
void MSDKCodec::UpdateBitStream()
{
    input_bs_.Data = input_mp_->GetReadPtr();
    input_bs_.DataOffset = 0;
    input_bs_.DataLength = input_mp_->GetFlatBufFullness();
}

void MSDKCodec::UpdateMemPool()
{
    input_mp_->UpdateReadPtr(input_bs_.DataOffset);
}

mfxStatus MSDKCodec::InitDecoder()
{
    mfxStatus sts = MFX_ERR_NONE;

    if ((ELEMENT_STRING_DEC != element_type_) && (ELEMENT_BMP_DEC != element_type_)) {
        UpdateBitStream();

        if (ELEMENT_DECODER == element_type_) {
            sts = mfx_dec_->DecodeHeader(&input_bs_, &mfx_video_param_);
#ifdef ENABLE_VPX_CODEC
        } else if (ELEMENT_VP8_DEC == element_type_) {
            sts = ((VP8DecPlugin *)user_dec_)->DecodeHeader(&input_bs_, &mfx_video_param_);
#endif
#ifdef ENABLE_SW_CODEC
        } else if (ELEMENT_SW_DEC == element_type_) {
            sts = ((SWDecPlugin *)user_sw_dec_)->DecodeHeader(&input_bs_, &mfx_video_param_);
#endif
        } else {
            sts = mfx_dec_->DecodeHeader(&input_bs_, &mfx_video_param_);
        }

        UpdateMemPool();

        if (MFX_ERR_MORE_DATA == sts) {
            usleep(100000);
            return sts;
        } else if (MFX_ERR_NONE == sts) {
            mfxFrameAllocRequest DecRequest;

            if (ELEMENT_DECODER == element_type_) {
                sts = mfx_dec_->QueryIOSurf(&mfx_video_param_, &DecRequest);
#ifdef ENABLE_VPX_CODEC
            } else if (ELEMENT_VP8_DEC == element_type_) {
                sts = ((VP8DecPlugin *)user_dec_)->QueryIOSurf(&mfx_video_param_, NULL, &DecRequest);
#endif
#ifdef ENABLE_SW_CODEC
            } else if (ELEMENT_SW_DEC == element_type_) {
                sts = ((SWDecPlugin *)user_sw_dec_)->QueryIOSurf(&mfx_video_param_, NULL, &DecRequest);
#endif
            } else {
                sts = mfx_dec_->QueryIOSurf(&mfx_video_param_, &DecRequest);
            }

            if ((MFX_ERR_NONE != sts) && (MFX_WRN_PARTIAL_ACCELERATION != sts)) {
                printf("QueryIOSurf return %d, failed\n", sts);
                return sts;
            }

            num_of_surf_ = DecRequest.NumFrameSuggested + 10;
            sts = AllocFrames(&DecRequest, true);

            if (m_bUseOpaqueMemory) {
                ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
                ext_opaque_alloc_.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
                ext_opaque_alloc_.Out.Surfaces = surface_pool_;
                ext_opaque_alloc_.Out.NumSurface = num_of_surf_;
                ext_opaque_alloc_.Out.Type = DecRequest.Type;
                m_DecExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&ext_opaque_alloc_));
            }

            if (!m_DecExtParams.empty()) {
                mfx_video_param_.ExtParam = &m_DecExtParams.front(); // vector is stored linearly in memory
                mfx_video_param_.NumExtParam = (mfxU16)m_DecExtParams.size();
                printf("init decoder, ext param number is %d\n", mfx_video_param_.NumExtParam);
            }

            // Initialize the Media SDK decoder
            if (ELEMENT_DECODER == element_type_) {
                sts = mfx_dec_->Init(&mfx_video_param_);
#ifdef ENABLE_VPX_CODEC
            } else if (ELEMENT_VP8_DEC == element_type_) {
                sts = ((VP8DecPlugin *)user_dec_)->Init(&mfx_video_param_);
                ((VP8DecPlugin *)user_dec_)->SetAllocPoint(m_pMFXAllocator);
#endif
#ifdef ENABLE_SW_CODEC
            } else if (ELEMENT_SW_DEC == element_type_) {
                sts = ((SWDecPlugin *)user_sw_dec_)->Init(&mfx_video_param_);
#endif
            } else {
                sts = mfx_dec_->Init(&mfx_video_param_);
            }

            if ((sts != MFX_ERR_NONE) && (MFX_WRN_PARTIAL_ACCELERATION != sts)) {
                return sts;
            }
        } else {
            printf("DecodeHeader returns %d\n", sts);
            return sts;
        }

#ifdef ENABLE_STRING_CODEC
    } else if (ELEMENT_STRING_DEC == element_type_) {
        sts = ((StringDecPlugin *)str_dec_)->DecodeHeader(input_str_, &mfx_video_param_);
        mfxFrameAllocRequest DecRequest;
        sts = ((StringDecPlugin *)str_dec_)->QueryIOSurf(&mfx_video_param_, NULL, &DecRequest);

        if (MFX_ERR_NONE != sts) {
            printf("QueryIOSurf return %d, failed.\n", sts);
            return sts;
        }

        num_of_surf_ = DecRequest.NumFrameSuggested + 10;
        sts = AllocFrames(&DecRequest, true);

        if (m_bUseOpaqueMemory) {
            ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
            ext_opaque_alloc_.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
            ext_opaque_alloc_.Out.Surfaces = surface_pool_;
            ext_opaque_alloc_.Out.NumSurface = num_of_surf_;
            ext_opaque_alloc_.Out.Type = DecRequest.Type;
            m_DecExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&ext_opaque_alloc_));
        }

        if (!m_DecExtParams.empty()) {
            mfx_video_param_.ExtParam = &m_DecExtParams.front(); // vector is stored linearly in memory
            mfx_video_param_.NumExtParam = (mfxU16)m_DecExtParams.size();
            printf("init decoder, ext param number is %d\n", mfx_video_param_.NumExtParam);
        }

        //Initialize the string decoder
        ((StringDecPlugin *)str_dec_)->SetAllocPoint(m_pMFXAllocator);
        sts = ((StringDecPlugin *)str_dec_)->Init(&mfx_video_param_);

        if (sts != MFX_ERR_NONE) {
            return sts;
        }

#endif
#ifdef ENABLE_PICTURE_CODEC
    } else if (ELEMENT_BMP_DEC == element_type_) {
        sts = ((PicDecPlugin *)pic_dec_)->DecodeHeader(input_pic_, &mfx_video_param_);
        mfxFrameAllocRequest DecRequest;
        sts = ((PicDecPlugin *)pic_dec_)->QueryIOSurf(&mfx_video_param_, NULL, &DecRequest);

        if (MFX_ERR_NONE != sts) {
            printf("QueryIOSurf return %d, failed.\n", sts);
            return sts;
        }

        num_of_surf_ = DecRequest.NumFrameSuggested + 10;
        sts = AllocFrames(&DecRequest, true);

        if (m_bUseOpaqueMemory) {
            ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
            ext_opaque_alloc_.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
            mfxExtBuffer *pExtParamsDec = (mfxExtBuffer *)&ext_opaque_alloc_;
            ext_opaque_alloc_.Out.Surfaces = surface_pool_;
            ext_opaque_alloc_.Out.NumSurface = num_of_surf_;
            ext_opaque_alloc_.Out.Type = DecRequest.Type;
            mfx_video_param_.ExtParam = &pExtParamsDec;
            mfx_video_param_.NumExtParam = 1;
        }

        // Initialize the picture decoder
        ((PicDecPlugin *)pic_dec_)->SetAllocPoint(m_pMFXAllocator);
        sts = ((PicDecPlugin *)pic_dec_)->Init(&mfx_video_param_);

        if (sts != MFX_ERR_NONE) {
            return sts;
        }

#endif
    }

    return MFX_ERR_NONE;
}

int MSDKCodec::HandleProcessDecode()
{
    mfxStatus sts = MFX_ERR_NONE;
    MediaBuf buf;
    MediaPad *srcpad = *(this->srcpads_.begin());
    mfxSyncPoint syncpD;
    mfxFrameSurface1 *pmfxOutSurface = NULL;
    bool bEndOfDecoder = false;

    while (is_running_) {
        usleep(100);

        if (!codec_init_) {
            if (measuremnt) {
                measuremnt->GetLock();
                measuremnt->TimeStpStart(DEC_INIT_TIME_STAMP, this);
                measuremnt->RelLock();
            }

            sts = InitDecoder();

            if (measuremnt) {
                measuremnt->GetLock();
                measuremnt->TimeStpFinish(DEC_INIT_TIME_STAMP, this);
                measuremnt->RelLock();
            }

            if (MFX_ERR_MORE_DATA == sts) {
                continue;
            } else if (MFX_ERR_NONE == sts) {
                codec_init_ = true;

                if (measuremnt) {
                    measuremnt->GetLock();
                    pipelineinfo einfo;
                    einfo.mElementType = element_type_;
                    einfo.mChannelNum = MSDKCodec::mNumOfDecChannels;
                    MSDKCodec::mNumOfDecChannels++;
                    if (input_mp_) {
                        einfo.mInputName.append(input_mp_->GetInputName());
                    }
                    measuremnt->SetElementInfo(DEC_ENDURATION_TIME_STAMP, this, &einfo);
                    measuremnt->RelLock();
                }
            } else {
                printf("Decode create failed: %d\n", sts);
                return -1;
            }
        }

        // Read es stream into bit stream
        int nIndex = 0;

        while (is_running_) {
            nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);

            if (MFX_ERR_NOT_FOUND == nIndex) {
                usleep(10000);
            } else {
                break;
            }
        }

        // Check again in case for a invalid surface.
        if (!is_running_) {
            break;
        }

        if ((ELEMENT_STRING_DEC != element_type_) && (ELEMENT_BMP_DEC != element_type_)) {
            if (measuremnt) {
                measuremnt->GetLock();
                measuremnt->TimeStpStart(DEC_FRAME_TIME_STAMP, this);
                measuremnt->RelLock();
            }

            UpdateBitStream();

            if (input_mp_->GetDataEof() && bEndOfDecoder) {
                if (ELEMENT_DECODER == element_type_) {
                    sts = mfx_dec_->DecodeFrameAsync(NULL, surface_pool_[nIndex],
                                                     &pmfxOutSurface, &syncpD);
                } else {
                    sts = MFXVideoUSER_ProcessFrameAsync(*mfx_session_,
                                                         (mfxHDL *) NULL, 1, (mfxHDL *) &surface_pool_[nIndex], 1, &syncpD);
                }

                if (!syncpD && MFX_ERR_MORE_DATA == sts) {
                    // Last frame decoded, push EOS
                    // printf("Decoder flushed, sending EOS to next element\n");
                    buf.msdk_surface = NULL;
                    buf.is_eos = 1;
                    printf("This video stream EOS.\n");
                    srcpad->PushBufToPeerPad(buf);
                    break;
                }
            } else {
                if (ELEMENT_DECODER == element_type_) {
                    sts = mfx_dec_->DecodeFrameAsync(&input_bs_, surface_pool_[nIndex],
                                                     &pmfxOutSurface, &syncpD);
                } else {
                    sts = MFXVideoUSER_ProcessFrameAsync(*mfx_session_,
                                                         (mfxHDL *) &input_bs_, 1, (mfxHDL *) &surface_pool_[nIndex], 1, &syncpD);
                }

                if (!syncpD && input_mp_->GetDataEof()) {
                    if (MFX_ERR_NONE > sts && MFX_ERR_MORE_SURFACE != sts) {
                        if (MFX_ERR_MORE_DATA != sts) {
                            printf("Decoder return error code:%d\n", sts);
                        }
                        bEndOfDecoder = true;
                    }
                }
            }

            if (MFX_ERR_NONE < sts && syncpD) {
                sts = MFX_ERR_NONE;
            }

            if (MFX_ERR_NONE == sts) {
                if (measuremnt) {
                    measuremnt->GetLock();
                    measuremnt->TimeStpFinish(DEC_FRAME_TIME_STAMP, this);
                    measuremnt->RelLock();
                }

                if (ELEMENT_DECODER == element_type_) {
                    buf.msdk_surface = pmfxOutSurface;
                } else {
                    buf.msdk_surface = surface_pool_[nIndex];
                }

                if (buf.msdk_surface) {
                    mfxFrameSurface1 *msdk_surface =
                        (mfxFrameSurface1 *)buf.msdk_surface;
                    msdk_surface->Data.Locked++;
                }

                buf.mWidth = mfx_video_param_.mfx.FrameInfo.CropW;
                buf.mHeight = mfx_video_param_.mfx.FrameInfo.CropH;
                buf.is_eos = 0;

                if (m_bUseOpaqueMemory) {
                    buf.ext_opaque_alloc = &ext_opaque_alloc_;
                }

                sts = mfx_session_->SyncOperation(syncpD, 60000);

                if (MFX_ERR_NONE == sts) {
                    srcpad->PushBufToPeerPad(buf);
                    if (measuremnt) {
                        measuremnt->GetLock();
                        measuremnt->UpdateCodecNum();
                        measuremnt->RelLock();
                    }
                } else {
                    if (buf.msdk_surface) {
                        mfxFrameSurface1 *msdk_surface =
                            (mfxFrameSurface1 *)buf.msdk_surface;
                        msdk_surface->Data.Locked--;
                    }
                }
            }

            UpdateMemPool();
#ifdef ENABLE_STRING_CODEC
        } else if (ELEMENT_STRING_DEC == element_type_) {
           if (is_eos_) {
               printf("string decoder needs to stop now.\n");
               break;
           }

           sts = MFXVideoUSER_ProcessFrameAsync(*mfx_session_,
                            (mfxHDL*)&input_str_, 1, (mfxHDL*)&surface_pool_[nIndex], 1, &syncpD);

           if (MFX_ERR_NONE < sts && syncpD) {
               sts = MFX_ERR_NONE;
           }
           if (MFX_ERR_NONE == sts) {
               buf.msdk_surface = surface_pool_[nIndex];

               if (buf.msdk_surface) {
                   mfxFrameSurface1* msdk_surface =
                    (mfxFrameSurface1*)buf.msdk_surface;
                   msdk_surface->Data.Locked++;
               }
               buf.mWidth = mfx_video_param_.mfx.FrameInfo.CropW;
               buf.mHeight = mfx_video_param_.mfx.FrameInfo.CropH;
               if (m_bUseOpaqueMemory) {
                   buf.ext_opaque_alloc = &ext_opaque_alloc_;
               }
               sts = mfx_session_->SyncOperation(syncpD, 60000);
               if (MFX_ERR_NONE == sts) {
                   srcpad->PushBufToPeerPad(buf);
               } else {
                   if (buf.msdk_surface) {
                       mfxFrameSurface1* msdk_surface =
                        (mfxFrameSurface1*)buf.msdk_surface;
                        msdk_surface->Data.Locked--;
                   }
                }
            }

#endif
#ifdef ENABLE_PICTURE_CODEC
        } else if (ELEMENT_BMP_DEC == element_type_) {
            if (is_eos_) {
                printf("bitmap decoder needs to stop now.\n");
                break;
            }

            sts = MFXVideoUSER_ProcessFrameAsync(*mfx_session_,
                             (mfxHDL*)&input_pic_, 1, (mfxHDL*)&surface_pool_[nIndex], 1, &syncpD);

            if (MFX_ERR_NONE < sts && syncpD) {
                sts = MFX_ERR_NONE;
            }
            if (MFX_ERR_NONE == sts) {
                buf.msdk_surface = surface_pool_[nIndex];

                if (buf.msdk_surface) {
                    mfxFrameSurface1* msdk_surface =
                     (mfxFrameSurface1*)buf.msdk_surface;
                    msdk_surface->Data.Locked++;
                }
                buf.mWidth = mfx_video_param_.mfx.FrameInfo.CropW;
                buf.mHeight = mfx_video_param_.mfx.FrameInfo.CropH;
                if (m_bUseOpaqueMemory) {
                    buf.ext_opaque_alloc = &ext_opaque_alloc_;
                }
                sts = mfx_session_->SyncOperation(syncpD, 60000);
                if (MFX_ERR_NONE == sts) {
                    srcpad->PushBufToPeerPad(buf);
                } else {
                    if (buf.msdk_surface) {
                        mfxFrameSurface1* msdk_surface =
                         (mfxFrameSurface1*)buf.msdk_surface;
                        msdk_surface->Data.Locked--;
                    }
                }
            }
#endif
        }
    }

    if (!is_running_) {
        buf.msdk_surface = NULL;
        buf.is_eos = 1;
        printf("This video stream will be stoped.\n");
        srcpad->PushBufToPeerPad(buf);
    } else {
        is_running_ = false;
    }

    return 0;
}

int MSDKCodec::HandleProcessEncode()
{
    MediaBuf buf;
    MediaPad *sinkpad = *(this->sinkpads_.begin());
    assert(sinkpad != *(this->sinkpads_.end()));

    while (is_running_) {
        if (sinkpad->GetBufData(buf) != 0) {
            // No data, just sleep and wait
            usleep(10000);
            continue;
        }

        ProcessChain(sinkpad, buf);
        sinkpad->ReturnBufToPeerPad(buf);
    }

    if (!is_running_) {
        while (sinkpad->GetBufData(buf) == 0) {
            sinkpad->ReturnBufToPeerPad(buf);
        }
    } else {
        is_running_ = false;
    }

    return 0;
}


int MSDKCodec::HandleProcess()
{
    if (ELEMENT_DECODER == element_type_ || \
            ELEMENT_VP8_DEC == element_type_ || \
            ELEMENT_SW_DEC == element_type_ || \
            ELEMENT_STRING_DEC == element_type_ || \
            ELEMENT_BMP_DEC == element_type_) {
        return HandleProcessDecode();
    } else if (ELEMENT_ENCODER == element_type_ || \
               ELEMENT_VP8_ENC == element_type_ || \
               ELEMENT_SW_ENC == element_type_ || \
               ELEMENT_YUV_WRITER == element_type_) {
        return HandleProcessEncode();
    } else if (ELEMENT_VPP == element_type_) {
        return HandleProcessVpp();
#ifdef MSDK_FEI
    } else if (ELEMENT_FEI_PREENC == element_type_) {
        return HandleProcessPreENC();
#endif
    } else {
        return 0;
    }
}

#ifdef MSDK_FEI
#define MFX_FRAMETYPE_IPB (MFX_FRAMETYPE_I | MFX_FRAMETYPE_P | MFX_FRAMETYPE_B)
void MSDKCodec::initFrameParams(iTask *eTask)
{
    mfx_fei_preenc_ctr_.DisableMVOutput = 0;

    switch (eTask->frameType & MFX_FRAMETYPE_IPB) {
    case MFX_FRAMETYPE_I:
        eTask->in.NumFrameL0 = 0; //1;  //just workaround to support I frames
        eTask->in.NumFrameL1 = 0;
        eTask->in.L0Surface = NULL; //eTask->in.InSurface;
        //in data
        eTask->in.NumExtParam = num_ext_in_param_;
        eTask->in.ExtParam = inBufsPreEnc;

        //out data
        //exclude MV output from output buffers
        if (mfx_fei_preenc_ctr_.DisableStatisticsOutput) {
            eTask->out.NumExtParam = 0;
            eTask->out.ExtParam = NULL;
            printf("---Disable Statics Output\n");
        } else {
            eTask->out.NumExtParam = 1;
            eTask->out.ExtParam = outBufsPreEncI;
        }

        mfx_fei_preenc_ctr_.DisableMVOutput = 1;
        printf("num_ext_in_param_ is %d\n", num_ext_in_param_);
        break;
    case MFX_FRAMETYPE_P:
        eTask->in.NumFrameL0 = 1;
        eTask->in.NumFrameL1 = 0;
        eTask->in.L0Surface = eTask->prevTask->in.InSurface;
        //in data
        eTask->in.NumExtParam = num_ext_in_param_;
        eTask->in.ExtParam = inBufsPreEnc;
        //out data
        eTask->out.NumExtParam = num_ext_out_param_;
        eTask->out.ExtParam = outBufsPreEnc;
        break;
    case MFX_FRAMETYPE_B:
        eTask->in.NumFrameL0 = 1;
        eTask->in.NumFrameL1 = 1;
        eTask->in.L0Surface = eTask->prevTask->in.InSurface;
        eTask->in.L1Surface = eTask->nextTask->in.InSurface;
        //in data
        eTask->in.NumExtParam = num_ext_in_param_;
        eTask->in.ExtParam = inBufsPreEnc;
        //out data
        eTask->out.NumExtParam = num_ext_out_param_;
        eTask->out.ExtParam = outBufsPreEnc;
        break;
    }
}

iTask *MSDKCodec::findFrameToEncode()
{
    // at this point surface for encoder contains a frame from file
    std::list<iTask *>::iterator encTask = mfx_fei_input_tasks_.begin();
    iTask *prevTask = NULL;
    iTask *nextTask = NULL;

    //find task to encode
    for (; encTask != mfx_fei_input_tasks_.end(); ++encTask) {
        if ((*encTask)->encoded) {
            continue;
        }

        prevTask = NULL;
        nextTask = NULL;

        if ((*encTask)->frameType & MFX_FRAMETYPE_I) {
            break;
        } else if ((*encTask)->frameType & MFX_FRAMETYPE_P) { //find previous ref
            std::list<iTask *>::iterator it = encTask;

            while ((it--) != mfx_fei_input_tasks_.begin()) {
                iTask *curTask = *it;

                if (curTask->frameType & (MFX_FRAMETYPE_P | MFX_FRAMETYPE_I) &&
                        curTask->encoded) {
                    prevTask = curTask;
                    break;
                }
            }

            if (prevTask) {
                break;
            }
        } else if ((*encTask)->frameType & MFX_FRAMETYPE_B) {//find previous ref
            std::list<iTask *>::iterator it = encTask;

            while ((it--) != mfx_fei_input_tasks_.begin()) {
                iTask *curTask = *it;

                if (curTask->frameType & (MFX_FRAMETYPE_P | MFX_FRAMETYPE_I) &&
                        curTask->encoded) {
                    prevTask = curTask;
                    break;
                }
            }

            //find next ref
            it = encTask;

            while ((++it) != mfx_fei_input_tasks_.end()) {
                iTask *curTask = *it;

                if (curTask->frameType & (MFX_FRAMETYPE_P | MFX_FRAMETYPE_I) &&
                        curTask->encoded) {
                    nextTask = curTask;
                    break;
                }
            }

            if (prevTask && nextTask) {
                break;
            }
        }
    }

    if (encTask == mfx_fei_input_tasks_.end()) {
        return NULL; //skip B without refs
    }

    (*encTask)->prevTask = prevTask;
    (*encTask)->nextTask = nextTask;
    return *encTask;
}

int MSDKCodec::GetFrameType(int pos)
{
    if (mfx_fei_gop_size_ < 0) {
        if (0 == pos) {
            return MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;
        } else {
            return MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
        }
    } else {
        if ((pos % mfx_fei_gop_size_) == 0) {
            return MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;
        } else if ((pos % mfx_fei_gop_size_ % mfx_fei_ref_dist_) == 0) {
            return MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
        } else {
            return MFX_FRAMETYPE_B;
        }
    }
}

int MSDKCodec::InitPreENC(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    unsigned heightMB = ((buf.mHeight + 15) & ~15);
    unsigned widthMB = ((buf.mWidth + 15) & ~15);
    unsigned numMB = heightMB * widthMB / 256;
    mfx_video_param_.mfx.FrameInfo.FrameRateExtN = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtN;
    mfx_video_param_.mfx.FrameInfo.FrameRateExtD = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtD;
    mfx_video_param_.mfx.FrameInfo.FourCC        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FourCC;
    mfx_video_param_.mfx.FrameInfo.ChromaFormat  = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.ChromaFormat;
    mfx_video_param_.mfx.FrameInfo.CropX         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropX;
    mfx_video_param_.mfx.FrameInfo.CropY         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropY;
    mfx_video_param_.mfx.FrameInfo.CropW         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropW;
    mfx_video_param_.mfx.FrameInfo.CropH         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropH;
    mfx_video_param_.mfx.FrameInfo.Width         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Width;
    mfx_video_param_.mfx.FrameInfo.Height        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Height;
    mfx_video_param_.mfx.FrameInfo.PicStruct     = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;

    if (!mfx_video_param_.mfx.TargetKbps) {
        mfx_video_param_.mfx.TargetKbps = mfx_video_param_.mfx.FrameInfo.CropW *\
                                          mfx_video_param_.mfx.FrameInfo.CropH / 1000;
        printf("Waring: invalid setup bitrate,use default %d Kbps\n", mfx_video_param_.mfx.TargetKbps);
    }

    // Using Opaque Surfaces
    if (m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;
        ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        ext_opaque_alloc_.Header.BufferSz = sizeof(ext_opaque_alloc_);
        memcpy(&ext_opaque_alloc_.In, \
               & ((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out, \
               sizeof(((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out));
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&ext_opaque_alloc_));
    } else {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }

    //TEMP: force 1 ref
    mfx_video_param_.mfx.GopRefDist = 1;
    mfx_video_param_.mfx.GopPicSize = 1;

    //set mfx video ExtParam
    if (!m_EncExtParams.empty()) {
        mfx_video_param_.ExtParam = &m_EncExtParams.front(); // vector is stored linearly in memory
        mfx_video_param_.NumExtParam = (mfxU16)m_EncExtParams.size();
    }

    //sts = mfx_enc_->Init(&mfx_video_param_);
    //printf("mfx_enc_ init return %d\n", sts);
    //assert(sts == MFX_ERR_NONE);
    mfxVideoParam preEncParams = mfx_video_param_;
    // Update PREENC fields
    memset(&mfx_ext_fei_param_, 0, sizeof(mfx_ext_fei_param_));
    mfx_ext_fei_param_.Header.BufferId = MFX_EXTBUFF_FEI_PARAM;
    mfx_ext_fei_param_.Header.BufferSz = sizeof (mfxExtFeiParam);
    mfx_ext_fei_param_.Func = MFX_FEI_FUNCTION_PREENC;
    mfxExtBuffer *tmp_buf[1];
    tmp_buf[0] = (mfxExtBuffer *)&mfx_ext_fei_param_;
    preEncParams.NumExtParam = 1;
    preEncParams.ExtParam = tmp_buf;
    sts = mfx_fei_preenc_->Init(&preEncParams);
    printf("   ---PREENC init return %d\n", sts);
    assert(sts == MFX_ERR_NONE);

    if (MFX_WRN_PARTIAL_ACCELERATION == sts) {
        printf("WARNING: partial acceleration\n");
    }

    if (!mfx_fei_preenc_ctr_.DisableMVOutput) {
        mvs.Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MV;
        mvs.Header.BufferSz = sizeof (mfxExtFeiPreEncMV);
        mvs.NumMBAlloc = numMB;
        mvs.MB = new mfxExtFeiPreEncMV::mfxMB [numMB];
        printf("New mvs data, size %d, num MB is %d\n",
               sizeof(mfxExtFeiPreEncMV::mfxMB) * numMB, numMB);
        outBufsPreEnc[num_ext_out_param_++] = (mfxExtBuffer *) & mvs;
    }

    if (!mfx_fei_preenc_ctr_.DisableStatisticsOutput) {
        mbdata.Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MB;
        mbdata.Header.BufferSz = sizeof (mfxExtFeiPreEncMBStat);
        mbdata.NumMBAlloc = numMB;
        mbdata.MB = new mfxExtFeiPreEncMBStat::mfxMB [numMB];
        printf("New mb data, size %d, num MB is %d\n",
               sizeof(mfxExtFeiPreEncMBStat::mfxMB) * numMB, numMB);
        memset(mbdata.MB, 0, sizeof(mfxExtFeiPreEncMBStat::mfxMB) * numMB);
        outBufsPreEnc[num_ext_out_param_++] = (mfxExtBuffer *) & mbdata;
        outBufsPreEncI[0] = (mfxExtBuffer *) & mbdata; //special case for I frames
    }

    mfx_fei_ref_dist_ = 1; //no B frame, IBBP is 3, IP is 1
    mfx_fei_gop_size_ = -1; //-1 means no GOP. IBBPIBBP's gop is 4
    return 0;
}

int MSDKCodec::HandleProcessPreENC()
{
    mfxStatus sts = MFX_ERR_NONE;
    int ret = 0;
    MediaBuf buf;
    buf.ext_opaque_alloc = new mfxExtOpaqueSurfaceAlloc();
    MediaPad *sinkpad = *(this->sinkpads_.begin());
    assert(sinkpad != *(this->sinkpads_.end()));

    while (is_running_) {
        if (sinkpad->GetBufData(buf) != 0) {
            // No data, just sleep and wait
            usleep(10000);
            continue;
        }

        if (buf.msdk_surface == NULL) {
            printf("PreENC got EOF, task list size %d\n", mfx_fei_input_tasks_.size());
            MediaPad *srcpad = *(this->srcpads_.begin());
            srcpad->PushBufToPeerPad(buf);
            break;
        }

        if (!codec_init_) {
            ret = InitPreENC(buf);

            if (0 == ret) {
                printf("PreENC element init successfully\n");
                codec_init_ = true;
            } else {
                printf("PreENC create failed %d\n", ret);
                return -1;
            }
        }

        // Doing PreENC...
        iTask *eTask = new iTask;
        eTask->frameType = GetFrameType(mfx_fei_frame_seq_);
        eTask->frameDisplayOrder = mfx_fei_frame_seq_;
        eTask->encoded = 0;
        mfx_fei_frame_seq_++;
        mfxFrameSurface1 *pSurf = (mfxFrameSurface1 *)buf.msdk_surface;
        pSurf->Data.Locked++;
        eTask->in.InSurface = pSurf;
        mfx_fei_input_tasks_.push_back(eTask); //mfx_fei_input_tasks_ in display order
        eTask = findFrameToEncode();

        if (eTask == NULL) {
            printf("No frame to do the PreENC\n");
            continue; //not found frame to encode
        }

        eTask->in.InSurface->Data.FrameOrder = eTask->frameDisplayOrder;
        initFrameParams(eTask);

        for (;;) {
            // printf("frame: %d  t:0x%x : submit\n", eTask->frameDisplayOrder, eTask->frameType);
            sts = mfx_fei_preenc_->ProcessFrameAsync(&eTask->in, &eTask->out, &eTask->EncSyncP);
            assert(sts == MFX_ERR_NONE);
            sts = mfx_session_->SyncOperation(eTask->EncSyncP, 200000);
            assert(sts == MFX_ERR_NONE);

            if (MFX_ERR_NONE < sts && !eTask->EncSyncP) {
                // repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts) {
                    usleep(10000); // wait if device is busy
                }
            } else if (MFX_ERR_NONE < sts && eTask->EncSyncP) {
                sts = MFX_ERR_NONE; // ignore warnings if output is available
                break;
            } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                //                sts = AllocateSufficientBuffer(&pCurrentTask->mfxBS);
                //                MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
            } else {
                // get next surface and new task for 2nd bitstream in ViewOutput mode
                break;
            }
        }

        // TODO:
        // The Locked doesn't decrease after syncOperation
        // Decrease by 1 manually
        eTask->in.InSurface->Data.Locked--;
        eTask->encoded = 1;

        if ((this->mfx_fei_ref_dist_ * 2) == mfx_fei_input_tasks_.size()) {
            mfx_fei_input_tasks_.front()->in.InSurface->Data.Locked--;
            delete mfx_fei_input_tasks_.front();
            mfx_fei_input_tasks_.pop_front();
        }

        PreEncInfo info;
        PreEncResults results;
        memset(&info, 0, sizeof(info));
        memset(&results, 0, sizeof(results));
        info.mv_info = &mvs;
        info.mb_info = &mbdata;
        info.yuv_width = buf.mWidth;
        info.yuv_height = buf.mHeight;

        if (fei_cb_func_) {
            ret = fei_cb_func_(&info, &results);
        }

        if (results.sceneChangeFrame) {
            buf.is_key_frame = 1;
        } else {
            buf.is_key_frame = 0;
        }

        if (ret == 0) {
            mfxFrameSurface1 *msdk_surface = (mfxFrameSurface1 *)buf.msdk_surface;
            msdk_surface->Data.Locked++;
            MediaPad *srcpad = *(this->srcpads_.begin());
            srcpad->PushBufToPeerPad(buf);
        }

        ReturnBuf(buf);
    }

    is_running_ = false;
    return 0;
}
#endif

int MSDKCodec::PrepareVppCompFrames()
{
    MediaBuf buf;
    std::list<MediaPad *>::iterator it_sinkpad;
    MediaPad *sinkpad = NULL;

    if (0 == comp_stc_) {
        // Init stage.
        // Wait for all the frame to generate the 1st composited one.
        for (it_sinkpad = this->sinkpads_.begin();
                it_sinkpad != this->sinkpads_.end();
                ++it_sinkpad) {
            sinkpad = *it_sinkpad;

            while (sinkpad->GetBufData(buf) != 0) {
                // No data, just sleep and wait
                if (!is_running_) {
                    return -1;
                }
                usleep(1000);
                continue;
            }

            // Set corresponding framerate in VPPCompInfo
            if (vpp_comp_map_.find(sinkpad) == vpp_comp_map_.end()) {
                assert(0);
            }

            mfxFrameSurface1 *msdk_surface = (mfxFrameSurface1 *)buf.msdk_surface;
            if (msdk_surface) {
                vpp_comp_map_[sinkpad].ready_surface = buf;

                if (msdk_surface->Info.FrameRateExtD > 0) {
                    vpp_comp_map_[sinkpad].frame_rate = (float)msdk_surface->Info.FrameRateExtN;
                    vpp_comp_map_[sinkpad].frame_rate /= (float)msdk_surface->Info.FrameRateExtD;
                    printf("sinkpad %p, framerate is %f, %u,%u\n",
                    sinkpad,
                    vpp_comp_map_[sinkpad].frame_rate,
                    msdk_surface->Info.FrameRateExtN,
                    msdk_surface->Info.FrameRateExtD);
                } else {
                    // Set default frame rate as 30.
                    vpp_comp_map_[sinkpad].frame_rate = 30;
                }

                if (vpp_comp_map_[sinkpad].frame_rate > max_comp_framerate_) {
                    max_comp_framerate_ = vpp_comp_map_[sinkpad].frame_rate;
                    printf("------- Set max_comp_framerate_ to %f\n", max_comp_framerate_);
                }
            }
        }
    } else {
        // TODO:
        // How to end, what if input surface is NULL.
        bool out_of_time = false;
        for (it_sinkpad = this->sinkpads_.begin();
                it_sinkpad != this->sinkpads_.end();
                ++it_sinkpad) {
            sinkpad = *it_sinkpad;

            if ((vpp_comp_map_[sinkpad].ready_surface.msdk_surface == NULL && \
                    vpp_comp_map_[sinkpad].ready_surface.is_eos)) {
                continue;
            }
            while (sinkpad->GetBufData(buf) != 0) {
                // No data, just sleep and wait
                if (!is_running_) {
                    //return -1 and then it would return the queued buffers in HandleProcessVpp()
                    return -1;
                }
                unsigned tmp_cur_time = GetSysTimeInUs();

                if (tmp_cur_time - comp_stc_ > 1000 * 1000 / max_comp_framerate_) {
                    if (vpp_comp_map_[sinkpad].ready_surface.msdk_surface == NULL &&
                            vpp_comp_map_[sinkpad].ready_surface.is_eos == 0) {
                        // Newly attached stream doesn't have decoded frame yet, wait...
                    } else {
                        out_of_time = true;
                        printf("[%p]-frame comes late from pad %p, diff %u, framerate %f\n",
                               this,
                               sinkpad,
                               tmp_cur_time - comp_stc_,
                               max_comp_framerate_);
                        break;
                    }
                }

                usleep(5000);
            }
            if (!vpp_comp_map_[sinkpad].str_info.text && !vpp_comp_map_[sinkpad].pic_info.bmp) { //this sinkpad corresponds to video stream
                if (buf.is_eos) {
                    eos_cnt += buf.is_eos;
                }
                if (stream_cnt_ == eos_cnt) {
                    printf("All video streams EOS, can end VPP now. stream_cnt: %d, eos_cnt: %d\n", stream_cnt_, eos_cnt);
                    stream_cnt_ = 0;
                    return 1;
                }
            }

            if (out_of_time) {
                // Frames comes late.
                // Use last surface, drop 1 frame in future.
                vpp_comp_map_[sinkpad].drop_frame_num++;
                out_of_time = false;
            } else {
                // Update ready surface and release last surface.
                sinkpad->ReturnBufToPeerPad(vpp_comp_map_[sinkpad].ready_surface);
                vpp_comp_map_[sinkpad].ready_surface = buf;

                // Check if need to drop frame.
                while (vpp_comp_map_[sinkpad].drop_frame_num > 0) {
                    if (sinkpad->GetBufQueueSize() > 1) {
                        // Drop frames.
                        // Only drop frame if it has more than 1, don't drop all of them
                        MediaBuf drop_buf;
                        sinkpad->GetBufData(drop_buf);
                        sinkpad->ReturnBufToPeerPad(drop_buf);
                        vpp_comp_map_[sinkpad].total_dropped_frames++;
                        printf("[%p]-sinkpad %p drop 1 frame\n", this, sinkpad);
                        vpp_comp_map_[sinkpad].drop_frame_num--;
                    } else {
                        break;
                    }
                }
            }
        }
    }

    comp_stc_ = GetSysTimeInUs();
    return 0;
}
#ifdef ENABLE_VA
int MSDKCodec::HandleProcessVA()
{
    //TODO: handle the vpp plugin process.
    return 0;
}

int MSDKCodec::HandleProcessRender()
{
    //TODO: handle the vpp plugin process.
    return 0;
}
#endif
int MSDKCodec::HandleProcessVpp()
{
    std::list<MediaPad *>::iterator it_sinkpad;
    mfxStatus sts = MFX_ERR_NONE;
    int nIndex = 0;
    bool all_eos = false;

    while (is_running_) {
        usleep(10000);
        WaitSinkMutex();

        if (0 == current_comp_number_) {
            current_comp_number_ = sinkpads_.size();
        }

        if (0 == current_comp_number_) {
            printf("---------something wrong here, all sinkpads detached\n");
            ReleaseSinkMutex();
            break;
        }

        int prepare_result = PrepareVppCompFrames();
        if (prepare_result == -1) {
            ReleaseSinkMutex();
            //break, then to return the bufs in in_buf_q
            break;
        } else if (prepare_result == 1) {
            ReleaseSinkMutex();
            all_eos = true;
            is_running_ = 0;
            break;
        }

        if (!codec_init_) {
            sts = InitVpp(vpp_comp_map_[*sinkpads_.begin()].ready_surface);

            if (MFX_ERR_NONE == sts) {
                printf("[%p]VPP element init successfully\n", this);
                codec_init_ = true;
                //initialized, so set the following flags as false. (they mey be set before playing)
                comp_info_change_ = false;
                reinit_vpp_flag_ = false;
                res_change_flag_ = false;
            } else {
                printf("VPP create failed: %d\n", sts);
                ReleaseSinkMutex();
                return -1;
            }
        }

        if ((current_comp_number_ != sinkpads_.size()) || comp_info_change_ ||
            reinit_vpp_flag_ || res_change_flag_) {
            // Re-init VPP.
            unsigned int before_change = GetSysTimeInUs();
            printf("stop/init vpp\n");
            mfx_vpp_->Close();
            printf("Re-init VPP...\n");
            //TODO: if vpp re-inits when the first pad is EOS, ready_surface == NULL
            sts = InitVpp(vpp_comp_map_[*sinkpads_.begin()].ready_surface);
            current_comp_number_ = sinkpads_.size();
            comp_info_change_ = false;
            reinit_vpp_flag_ = false;
            res_change_flag_ = false;
            printf("Re-Init VPP takes time %u(us)\n", GetSysTimeInUs() - before_change);
        }

        while (is_running_) {
            nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);

            if (MFX_ERR_NOT_FOUND == nIndex) {
                usleep(10000);
                //return MFX_ERR_MEMORY_ALLOC;
            } else {
                break;
            }
        }
        if (nIndex == MFX_ERR_NOT_FOUND) {
            //is_running_ is false, so element is stopped when it was waiting for free surface.
            //break then return queued buffers.
            ReleaseSinkMutex();
            break;
        }

        if (measuremnt) {
            measuremnt->GetLock();
            measuremnt->TimeStpStart(VPP_FRAME_TIME_STAMP, this);
            measuremnt->RelLock();
        }

        for (it_sinkpad = this->sinkpads_.begin();
                it_sinkpad != this->sinkpads_.end();
                ++it_sinkpad) {
            sts = DoingVpp((mfxFrameSurface1 *)vpp_comp_map_[*it_sinkpad].ready_surface.msdk_surface,
                           surface_pool_[nIndex]);
        }
        if (measuremnt) {
            measuremnt->GetLock();
            measuremnt->TimeStpFinish(VPP_FRAME_TIME_STAMP, this);
            measuremnt->RelLock();
        }
        ReleaseSinkMutex();
    }

    if (!is_running_) {
        //handle both two cases: stopped by EOS and before EOS
        //from every sinkpad.
        WaitSrcMutex();
        MediaBuf out_buf;
        MediaPad *srcpad = *(this->srcpads_.begin());
        //sending the null buf might be useless, as encoder would be stopped immediately after vpp
        //and it would return this null buf back without processing it.
        out_buf.msdk_surface = NULL;
        out_buf.is_eos = 1;
        if (srcpad) {
            srcpad->PushBufToPeerPad(out_buf);
            vppframe_num_++;
        }
        ReleaseSrcMutex();

        WaitSinkMutex();
        for (it_sinkpad = this->sinkpads_.begin(); \
            it_sinkpad != this->sinkpads_.end(); \
            ++it_sinkpad) {
            MediaPad *sinkpad = *it_sinkpad;
            if (all_eos) {
                if (vpp_comp_map_[sinkpad].str_info.text || vpp_comp_map_[sinkpad].pic_info.bmp) {
                    vpp_comp_map_[sinkpad].ready_surface.is_eos = 1;
                }
            }
            sinkpad->ReturnBufToPeerPad(vpp_comp_map_[sinkpad].ready_surface);
            if (sinkpad) {
                while (sinkpad->GetBufQueueSize() > 0) {
                    MediaBuf return_buf;
                    sinkpad->GetBufData(return_buf);
                    sinkpad->ReturnBufToPeerPad(return_buf);
                }
            }
        }
        ReleaseSinkMutex();
    } else {
        is_running_ = false;
    }

    return 0;
}

int MSDKCodec::GetFreeSurfaceIndex(mfxFrameSurface1 **pSurfacesPool,
                                   mfxU16 nPoolSize)
{
    if (pSurfacesPool) {
        for (mfxU16 i = 0; i < nPoolSize; i++) {
            if (0 == pSurfacesPool[i]->Data.Locked) {
                return i;
            }
        }
    }

    return MFX_ERR_NOT_FOUND;
}

unsigned int MSDKCodec::mNumOfEncChannels = 0;
unsigned int MSDKCodec::mNumOfDecChannels = 0;

void MSDKCodec::PadRemoved(MediaPad *pad)
{
    if (ELEMENT_VPP != element_type_) {
        if(MEDIA_PAD_SINK == pad->get_pad_direction()) {
            //if this assert hits, check if the element is stopped before it's unlinked
            assert(pad->GetBufQueueSize() == 0);
        }
        return;
    }

    if (MEDIA_PAD_SINK == pad->get_pad_direction()) {
        // VPP remove an input, may be N:1 --> N-1:1
        printf("--------------vpp element %p, Pad %p removed----\n", this, pad);
        // vpp is unlinked with its prev element when it's still running, to return the queued
        // buffers below
        if (is_running_) {
            //Return the ready_surface
            pad->ReturnBufToPeerPad(vpp_comp_map_[pad].ready_surface);
            //Return the bufs in in_buf_q
            MediaBuf return_buf;
            while (pad->GetBufQueueSize() > 0) {
                pad->GetBufData(return_buf);
                pad->ReturnBufToPeerPad(return_buf);
            }
        }

        vpp_comp_map_.erase(pad);

        //remove the Region info from dec_region_info_
        if (combo_type_ == COMBO_CUSTOM) {
            assert(dec_region_map_.find(pad) != dec_region_map_.end());
            dec_region_map_.erase(pad);
        }

        //if master is removed
        if (combo_type_ == COMBO_MASTER) {
            assert(pad->get_pad_status() == MEDIA_PAD_LINKED);
            if (pad->get_peer_pad()->get_parent() == combo_master_) {
                combo_master_ = NULL;
            }
        }
    }

    return;
}


void MSDKCodec::NewPadAdded(MediaPad *pad)
{
    // Only cares about VPP input streams, which means ELEMENT_VPP's sink pads.
    if (ELEMENT_VPP != element_type_) {
        return;
    }

    if (MEDIA_PAD_SINK == pad->get_pad_direction()) {
        // VPP get an input, may be N:1 --> N+1:1
        printf("--------------vpp element %p, New Pad %p added ----\n", this, pad);
        VPPCompInfo pad_info;
        pad_info.vpp_sinkpad = pad;
        //pad_info.dst_rect is used in COMBO_CUSTOM mode only.
        //When pad is newly added, don't display it.
        pad_info.dst_rect.x = 0;
        pad_info.dst_rect.y = 0;
        pad_info.dst_rect.w = 1;
        pad_info.dst_rect.h = 1;
        pad_info.ready_surface.msdk_surface = NULL;
        pad_info.ready_surface.is_eos = 0;

        if (input_str_) {
            pad_info.str_info = *input_str_;
        }
        if (input_pic_) {
            pad_info.pic_info = *input_pic_;
        }

        vpp_comp_map_[pad] = pad_info;

        //used for CUSTOM layout
        Region info;
        info.left = 0.0;
        info.top = 0.0;
        //When pad is newly added, don't display it.
        //They will be set as 1 in SetVppCompParam() then.
        info.width_ratio = 0.0;
        info.height_ratio = 0.0;
        dec_region_map_[pad] = info;
    }

    return;
}

// pre_element is the element linked to VPP
int MSDKCodec::SetVPPCompRect(BaseElement *pre_element, VppRect *rect)
{
    if (!pre_element || !rect) {
        return -1;
    }

    if (0 == rect->w || 0 == rect->h) {
        printf("invalid dst rect\n");
        return -1;
    }

    std::map<MediaPad *, VPPCompInfo>::iterator it_comp_info;

    for (it_comp_info = vpp_comp_map_.begin();
            it_comp_info != vpp_comp_map_.end();
            ++it_comp_info) {
        MediaPad *vpp_sink_pad = it_comp_info->first;

        if (MEDIA_PAD_UNLINKED == vpp_sink_pad->get_pad_status()) {
            // should not happen
            assert(0);
            continue;
        }

        if (pre_element == vpp_sink_pad->get_peer_pad()->get_parent()) {
            printf("Set element %p's vpp rect, %d/%d/%d/%d\n",
                   pre_element, rect->x, rect->y, rect->w, rect->h);
            it_comp_info->second.dst_rect = *rect;
            return 0;
        }
    }

    return -1;
}

mfxStatus MSDKCodec::AllocFrames(mfxFrameAllocRequest *pRequest, bool isDecAlloc)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxU16 i;
    pRequest->NumFrameMin = num_of_surf_;
    pRequest->NumFrameSuggested = num_of_surf_;
    mfxFrameAllocResponse *pResponse = isDecAlloc ? &m_mfxDecResponse : &m_mfxEncResponse;

    if (!m_bUseOpaqueMemory) {
        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, pRequest, pResponse);

        if (sts != MFX_ERR_NONE) {
            printf("AllocFrame failed %d\n", sts);
            return sts;
        }
    }

    surface_pool_ = new mfxFrameSurface1*[num_of_surf_];
#ifdef ENABLE_VA
    extbuf_ = new mfxExtBuffer*[num_of_surf_];
#endif
    for (i = 0; i < num_of_surf_; i++) {
        surface_pool_[i] = new mfxFrameSurface1;
        memset(surface_pool_[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(surface_pool_[i]->Info), &(pRequest->Info), sizeof(mfxFrameInfo));

        if (!m_bUseOpaqueMemory) {
            surface_pool_[i]->Data.MemId = pResponse->mids[i];
        }
#ifdef ENABLE_VA
        if (element_type_ == ELEMENT_VPP)
        {
            VAPluginData* plugData = new VAPluginData;
            extbuf_[i] = (mfxExtBuffer*)(plugData);
            plugData->Vector = new unsigned int[MAX_VECTOR_SIZE];
            plugData->Header.BufferId = MFX_EXTBUFF_VA_PLUGIN_PARAM;
            plugData->Header.BufferSz = sizeof(VAPluginData);
            surface_pool_[i]->Data.NumExtParam = 1;
            surface_pool_[i]->Data.ExtParam = (mfxExtBuffer**)(&(extbuf_[i]));
        }
#endif
    }

    return MFX_ERR_NONE;
}

/*
 * Set/re-set resolution, it applys to VPP only, encoder will be updated when new frame arrives
 * Returns: 0 - Successful
 *          1 - Warning, it doesn't apply to non-VPP element
 */
int MSDKCodec::SetRes(unsigned int width, unsigned int height)
{
    if (ELEMENT_VPP != element_type_ )
        return 1;

    if (!codec_init_) {
        mfx_video_param_.vpp.Out.CropW = width;
        mfx_video_param_.vpp.Out.CropH = height;

        return 0;
    }

    width = MSDK_ALIGN16(width);
    height = (MFX_PICSTRUCT_PROGRESSIVE == mfx_video_param_.vpp.Out.PicStruct) ?
             MSDK_ALIGN16(height) : MSDK_ALIGN32(height);

    if (width > frame_width_) {
        printf("War: specified width %d exceeds max supported width %d.\n", width, frame_width_);
        width = frame_width_;
    }
    if (height > frame_height_) {
        printf("War: specified height %d exceeds max supported height %d.\n", height, frame_height_);
        height = frame_height_;
    }

    if (width != mfx_video_param_.vpp.Out.Width ||
        height != mfx_video_param_.vpp.Out.Height) {
        printf("Info: reset vpp res to %dx%d\n", width, height);
        old_out_width_ = mfx_video_param_.vpp.Out.CropW;
        old_out_height_ = mfx_video_param_.vpp.Out.CropH;
        mfx_video_param_.vpp.Out.CropW = width;
        mfx_video_param_.vpp.Out.CropH = height;

        res_change_flag_ = true;
    } else {
        printf("Info: specified res is the same as current one, ignored\n");
    }

    return 0;
}

/*
 * Set rendering region for input "dec_dis" in VPP. Used only in COMBO_CUSTOM mode.
 * Returns: 0 - Successful
 *          1 - Warning, it does't apply to non-vpp element
 *          2 - Warning, VPP is not in COMBO_CUSTOM mode
 *          3 - Warning, vpp has not finished the last request.
 *         -1 - Invalid inputs
 *         -2 - Decoder is not linked to this vpp
 */
int MSDKCodec::SetCompRegion(void *dec_dis, Region &info, bool bApply)
{
    if (ELEMENT_VPP != element_type_) {
        printf("War: %s is for ELEMENT_VPP only\n", __func__);
        return 1;
    }

    if (COMBO_CUSTOM != combo_type_) {
        printf("War: VPP is not in COMBO_CUSTOM mode, this call is invalid\n");
        return 2;
    }

    if (!dec_dis) {
        printf("Err: null input dec_dis\n");
        return -1;
    }

    if (info.left < 0.0 || info.left >= 1.0 ||
        info.top < 0.0 || info.top >= 1.0 ||
        info.width_ratio <= 0.0 || info.width_ratio > 1.0 ||
        info.height_ratio <= 0.0 || info.height_ratio > 1.0 ||
        info.left + info.width_ratio > 1.0 ||
        info.top + info.height_ratio > 1.0) {
        printf("Err: invalid Region infomation\n");
        assert(0);
        return -1;
    }

    if (bApply && apply_region_info_flag_ == true) {
        printf("War: vpp has not finished applying the last request.\n");
        return 3;
    }

    //map dec_dis to sink_pad
    std::list<MediaPad *>::iterator it_sinkpad;
    WaitSinkMutex();
    for (it_sinkpad = this->sinkpads_.begin(); it_sinkpad != this->sinkpads_.end(); ++it_sinkpad) {
        if (dec_dis == (*it_sinkpad)->get_peer_pad()->get_parent()) {
            assert(apply_region_info_flag_ == false);
            dec_region_map_[*it_sinkpad] = info;

            // if(bApply), indicate VPP to reinit and apply the changes.
            if (bApply) {
                apply_region_info_flag_ = true;

                //if vpp is already initialied, re-init it
                if (codec_init_) {
                    reinit_vpp_flag_ = true;
                }
            }

            ReleaseSinkMutex();
            return 0;
        }
    }

    //not found in sinkpads_
    ReleaseSinkMutex();
    printf("Err: the decoder is not linked to this VPP\n");
    return -2;
}

int MSDKCodec::SetBgColor(BgColorInfo *bgColorInfo)
{
    if (bgColorInfo) {
        bg_color_info.Y = bgColorInfo->Y;
        bg_color_info.U = bgColorInfo->U;
        bg_color_info.V = bgColorInfo->V;
        reinit_vpp_flag_ = true;
    } else {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return -1;
    }
    return 0;
}


