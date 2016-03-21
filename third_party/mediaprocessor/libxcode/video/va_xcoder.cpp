/* <COPYRIGHT_TAG> */

#include "va_xcoder.h"

#ifdef ENABLE_VA

DEFINE_MLOGINSTANCE_CLASS(VAXcoder, "VAXcoder");
VAXcoder::VAXcoder()
{
    va_dpy_ = NULL;
    hwdev_ = NULL;
    card_fd_ = -1;
    impl_ = MFX_IMPL_AUTO_ANY;
    ver_.Minor = 8;
    ver_.Major = 1;
    mp_ = NULL;
    dec_ = NULL;
    tee_ = NULL;
    pMainSession_ = NULL;
    pMainAllocator_ = NULL;

    va4enc_ = NULL;
    vpp4va_ = NULL;
    va_ = NULL;
    pVaSession_ = NULL;
    pVaAllocator_ = NULL;
    va_output_ = NULL;

    vpp4enc_ = NULL;
    enc_ = NULL;
    pEncSession_ = NULL;
    pEncAllocator_ = NULL;
    enc_output_ = NULL;

    vpp4dis_ = NULL;
    dis_ = NULL;
    pDisSession_ = NULL;
    pDisAllocator_ = NULL;
}

VAXcoder::~VAXcoder()
{
    if (hwdev_) {
        hwdev_->Close();
        hwdev_ = NULL;
    }

    if (card_fd_ != -1) {
        close(card_fd_);
        card_fd_ = -1;
    }

    ReleaseRes();
}

int VAXcoder::InitInput(bool bDis, DecParam *dec_param)
{
     int sts;

     if (bDis) {
         // Create the VAAPI device
         // Use x11 display to render the frame.
         hwdev_ = CreateVAAPIDevice();
         if (hwdev_ == NULL) {
             MLOG_ERROR("createVAAPIDevice failed\n");
             return MFX_ERR_MEMORY_ALLOC;
         }

         sts = hwdev_->Init(NULL, 1, 0);
         if (sts != MFX_ERR_NONE) {
             MLOG_ERROR("hwdevice initialized failed\n");
             return MFX_ERR_UNKNOWN;
         }

         sts = hwdev_->GetHandle(MFX_HANDLE_VA_DISPLAY, (mfxHDL*)&va_dpy_);
         if (sts != MFX_ERR_NONE) {
             MLOG_ERROR("Get the va handle failed\n");
             return MFX_ERR_UNKNOWN;
         }
     } else {
         int major_version, minor_version;
         card_fd_ = open("/dev/dri/card0", O_RDWR);
         if (card_fd_ == -1) {
             MLOG_ERROR("Failed to open /dev/dri/card0\n");
             return MFX_ERR_UNKNOWN;
         }

         va_dpy_ = vaGetDisplayDRM(card_fd_);
         VAStatus ret = vaInitialize(va_dpy_, &major_version, &minor_version);
         if (ret != VA_STATUS_SUCCESS) {
             MLOG_ERROR("va initialized failed\n");
             return MFX_ERR_UNKNOWN;
         }
     }

     // Create the session and allocator.
     bool r = CreateMSDKRes(&pMainSession_, &pMainAllocator_);
     if (r == false) {
         ReleaseRes();
         return MFX_ERR_UNKNOWN;
     }

     mp_ = dec_param->inputBs;

     ElementCfg dec_cfg;
     memset(&dec_cfg, 0, sizeof(dec_cfg));
     dec_cfg.input_stream = dec_param->inputBs;
     dec_cfg.DecParams.mfx.CodecId = dec_param->input_codec_type;

     dec_ = new MSDKCodec(ELEMENT_DECODER, pMainSession_, pMainAllocator_);

     if (dec_param->mode == ELEMENT_MODE_PASSIVE) {
         dec_->Init(&dec_cfg, ELEMENT_MODE_PASSIVE);
     } else {
         dec_->Init(&dec_cfg, ELEMENT_MODE_ACTIVE);
     }

     tee_ = new Dispatcher;
     tee_->Init(NULL, ELEMENT_MODE_PASSIVE);

     dec_->LinkNextElement(tee_);

     return MFX_ERR_NONE;
}

void VAXcoder::AttachVaOutput(VppParam *vpp_param, VaParam *va_param)
{
    bool r = CreateMSDKRes(&pVaSession_, &pVaAllocator_);
    if (r == false) {
        ReleaseRes();
        return;
    }

    ElementCfg vpp_cfg;
    memset(&vpp_cfg, 0, sizeof(vpp_cfg));
    vpp_cfg.VppParams.vpp.Out.CropW = vpp_param->width;
    vpp_cfg.VppParams.vpp.Out.CropH = vpp_param->height;
    vpp_cfg.VppParams.vpp.Out.FourCC = MFX_FOURCC_NV12;
    vpp4va_ = new MSDKCodec(ELEMENT_VPP, pVaSession_, pVaAllocator_);
    vpp4va_->Init(&vpp_cfg, ELEMENT_MODE_PASSIVE);

    ElementCfg va_cfg;
    memset(&va_cfg, 0, sizeof(va_cfg));
    va_output_ = va_param->va_output;
    va_cfg.bDump = va_param->dump;
    va_cfg.bOpenCLEnable = va_param->OpenCLEnable;
    va_cfg.KernelPath = va_param->KernelPath;
    va_cfg.PerfTrace = va_param->PerfTrace;
    va_cfg.va_interval = va_param->interval;
    va_cfg.va_type = va_param->type;
    va_cfg.output_format = va_param->out_format;
    if (va_cfg.output_format != FORMAT_ES) {
        va_cfg.output_stream = va_param->va_output;
    }
    va_ = new MSDKCodec(ELEMENT_VA, pVaSession_, pVaAllocator_);
    va_->Init(&va_cfg, ELEMENT_MODE_PASSIVE);

    if (va_param->out_format == FORMAT_ES) {
        ElementCfg enc_cfg;
        memset(&enc_cfg, 0 , sizeof(enc_cfg));
        enc_cfg.output_stream = va_param->va_output;
        enc_cfg.EncParams.mfx.CodecId = MFX_CODEC_AVC;
        enc_cfg.EncParams.mfx.CodecProfile = MFX_PROFILE_AVC_MAIN;
        enc_cfg.EncParams.mfx.CodecLevel = MFX_LEVEL_AVC_41;
        enc_cfg.EncParams.mfx.IdrInterval= 0;
        enc_cfg.EncParams.mfx.NumRefFrame = 1;
        enc_cfg.EncParams.mfx.GopRefDist = 1;
        // Hard code for the va output encoder.
        enc_cfg.EncParams.mfx.TargetKbps = 2000;
        enc_cfg.EncParams.mfx.GopPicSize = 30;
        enc_cfg.EncParams.mfx.TargetUsage = MFX_TARGETUSAGE_7;
        enc_cfg.EncParams.mfx.NumSlice = 1;
        enc_cfg.EncParams.mfx.RateControlMethod = MFX_RATECONTROL_CBR;
        enc_cfg.extParams.bMBBRC = false;
        enc_cfg.extParams.nLADepth = 0;
        // Hard code for the va output encoder.
        enc_cfg.EncParams.mfx.FrameInfo.FrameRateExtN = 30;
        enc_cfg.EncParams.mfx.FrameInfo.FrameRateExtD = 1;
        va4enc_ = new MSDKCodec(ELEMENT_ENCODER, pVaSession_, pVaAllocator_);
        va4enc_->Init(&enc_cfg, ELEMENT_MODE_PASSIVE);
    }

    pMainSession_->JoinSession(*pVaSession_);
    if (va_param->out_format == FORMAT_ES) {
        va_->LinkNextElement(va4enc_);
    }
    vpp4va_->LinkNextElement(va_);
    tee_->LinkNextElement(vpp4va_);
}

void VAXcoder::AttachBsOutput(VppParam *vpp_param, EncParam *enc_param)
{
    bool r = CreateMSDKRes(&pEncSession_, &pEncAllocator_);
    if (r == false) {
        ReleaseRes();
        return;
    }

    enc_output_ = enc_param->bs_output;
    ElementCfg vpp_cfg;
    memset(&vpp_cfg, 0, sizeof(vpp_cfg));
    vpp_cfg.VppParams.vpp.Out.CropW = vpp_param->width;
    vpp_cfg.VppParams.vpp.Out.CropH = vpp_param->height;

    vpp4enc_ = new MSDKCodec(ELEMENT_VPP, pEncSession_, pEncAllocator_);
    vpp4enc_->Init(&vpp_cfg, ELEMENT_MODE_PASSIVE);
    ElementCfg enc_cfg;
    memset(&enc_cfg, 0 , sizeof(enc_cfg));
    enc_cfg.output_stream = enc_param->bs_output;
    enc_cfg.EncParams.mfx.CodecId = MFX_CODEC_AVC;
    enc_cfg.EncParams.mfx.CodecProfile = MFX_PROFILE_AVC_MAIN;
    enc_cfg.EncParams.mfx.CodecLevel = MFX_LEVEL_AVC_41;
    enc_cfg.EncParams.mfx.IdrInterval= 0;
    enc_cfg.EncParams.mfx.NumRefFrame = 1;
    enc_cfg.EncParams.mfx.GopRefDist = 1;
    enc_cfg.EncParams.mfx.TargetKbps = enc_param->bitrate;
    enc_cfg.EncParams.mfx.GopPicSize = enc_param->gop_size;
    enc_cfg.EncParams.mfx.TargetUsage = MFX_TARGETUSAGE_7;
    enc_cfg.EncParams.mfx.NumSlice = 1;
    enc_cfg.EncParams.mfx.RateControlMethod = MFX_RATECONTROL_CBR;
    enc_cfg.extParams.bMBBRC = false;
    enc_cfg.extParams.nLADepth = 0;
    if (enc_param->frame_rate_num != 0 && enc_param->frame_rate_den != 0) {
        enc_cfg.EncParams.mfx.FrameInfo.FrameRateExtN =
                                enc_param->frame_rate_num;
        enc_cfg.EncParams.mfx.FrameInfo.FrameRateExtD =
                                enc_param->frame_rate_den;
    } else {
        enc_cfg.EncParams.mfx.FrameInfo.FrameRateExtN = enc_param->gop_size;
        enc_cfg.EncParams.mfx.FrameInfo.FrameRateExtD = 1;
    }

    enc_ = new MSDKCodec(ELEMENT_ENCODER, pEncSession_, pEncAllocator_);
    enc_->Init(&enc_cfg, ELEMENT_MODE_PASSIVE);
    pMainSession_->JoinSession(*pEncSession_);

    vpp4enc_->LinkNextElement(enc_);
    tee_->LinkNextElement(vpp4enc_);
}

void VAXcoder::AttachDisOutput(VppParam* vpp_param, DisParam *dis_param)
{
    bool r = CreateMSDKRes(&pDisSession_, &pDisAllocator_);
    if (r == false) {
        ReleaseRes();
        return;
    }

    ElementCfg vpp_cfg;
    memset(&vpp_cfg, 0, sizeof(vpp_cfg));
    vpp_cfg.VppParams.vpp.Out.CropW = vpp_param->width;
    vpp_cfg.VppParams.vpp.Out.CropH = vpp_param->height;
    vpp_cfg.VppParams.vpp.Out.FourCC = MFX_FOURCC_RGB4;

    vpp4dis_ = new MSDKCodec(ELEMENT_VPP, pDisSession_, pDisAllocator_);
    vpp4dis_->Init(&vpp_cfg, ELEMENT_MODE_PASSIVE);

    ElementCfg dis_cfg;
    memset(&dis_cfg, 0, sizeof(dis_cfg));
    dis_cfg.hwdev = hwdev_;
    dis_ = new MSDKCodec(ELEMENT_RENDER, pDisSession_, pDisAllocator_);
    dis_->Init(&dis_cfg, ELEMENT_MODE_PASSIVE);
    pMainSession_->JoinSession(*pDisSession_);

    vpp4dis_->LinkNextElement(dis_);
    tee_->LinkNextElement(vpp4dis_);
}

bool VAXcoder::Start()
{
    if (enc_ && vpp4enc_) {
        enc_->Start();
        vpp4enc_->Start();
    }

    if (dis_ && vpp4dis_) {
        dis_->Start();
        vpp4dis_->Start();
    }

    if (va_ && vpp4va_) {
        va_->Start();
        vpp4va_->Start();
    }

    if (va4enc_) {
        va4enc_->Start();
    }

    if (dec_) {
        dec_->Start();
    }

    if (tee_) {
        tee_->Start();
    }

    return true;
}

int VAXcoder::Step(FrameMeta* input)
{
    // Prepeare the bit stream for the decoder.

    // Prepare the meta information if there is.
    if (va_) {
        va_->SetVARegion(input);
    }
    // start the video decoder.
    if (dec_) {
        dec_->Step(input);
    }

    return 0;
}

bool VAXcoder::Stop()
{
    if (enc_ && vpp4enc_) {
        vpp4enc_->Stop();
        enc_->Stop();
    }

    if(pEncSession_) {
        pEncSession_->Close();
    }

    if (dis_ && vpp4dis_) {
        vpp4dis_->Stop();
        dis_->Stop();
    }

    if (pDisSession_) {
         pDisSession_->Close();
    }

    if (va_ && vpp4va_) {
        vpp4va_->Stop();
        va_->Stop();
    }

    if (va4enc_) {
        va4enc_->Stop();
    }

    if (pVaSession_) {
        pVaSession_->Close();
    }

    if (dec_) {
        dec_->Stop();
    }

    if (tee_) {
        tee_->Stop();
    }

    if (pMainSession_) {
        pMainSession_->Close();
    }
    return true;
}

bool VAXcoder::Join()
{
    bool res = true;
    if (dec_) {
        res &= dec_->Join();
    }

    if (tee_) {
        res &= tee_->Join();
    }

    if (va_) {
        res &= va_->Join();
    }

    if (vpp4va_) {
        res &= vpp4va_->Join();
    }

    if (va4enc_) {
        res &= va4enc_->Join();
    }

    if (enc_) {
        res &= enc_->Join();
    }

    if (vpp4enc_) {
        res &= vpp4enc_->Join();
    }

    if (dis_) {
        res &= dis_->Join();
    }

    if (vpp4dis_) {
        res &= vpp4dis_->Join();
    }

    return res;
}

void VAXcoder::ReleaseRes()
{
    if (dec_) {
        delete dec_;
        dec_ = NULL;
    }

    if (tee_) {
        delete tee_;
        tee_ = NULL;
    }

    if (pMainSession_) {
        delete pMainSession_;
        pMainSession_ = NULL;
    }

    if (pMainAllocator_) {
        delete pMainAllocator_;
        pMainAllocator_ = NULL;
    }

    if (vpp4enc_) {
        delete vpp4enc_;
        vpp4enc_ = NULL;
    }

    if (enc_) {
        delete enc_;
        enc_ = NULL;
    }

    if (pEncSession_) {
        delete pEncSession_;
        pEncSession_ = NULL;
    }

    if (pEncAllocator_) {
        delete pEncAllocator_;
        pEncAllocator_ = NULL;
    }

    if (vpp4va_) {
        delete vpp4va_;
        vpp4va_ = NULL;
    }

    if (va_) {
        delete va_;
        va_ = NULL;
    }

    if (va4enc_) {
        delete va4enc_;
        va4enc_ = NULL;
    }

    if (pVaSession_) {
        delete pVaSession_;
        pVaSession_ = NULL;
    }

    if (pVaAllocator_) {
        delete pVaAllocator_;
        pVaAllocator_ = NULL;
    }

    if (vpp4dis_) {
        delete vpp4dis_;
        vpp4dis_ = NULL;
    }

    if (dis_) {
        delete dis_;
        dis_ = NULL;
    }

    if (pDisSession_) {
        delete pDisSession_;
        pDisSession_ = NULL;
    }

    if (pDisAllocator_) {
        delete pDisAllocator_;
        pDisAllocator_ = NULL;
    }
}

bool VAXcoder::CreateMSDKRes(MFXVideoSession** psession, GeneralAllocator** palloc)
{
    int sts;
    MFXVideoSession *pmfxSession = new MFXVideoSession;
    sts = pmfxSession->Init(impl_, &ver_);
    pmfxSession->QueryIMPL(&impl_);
    sts = pmfxSession->SetHandle((mfxHandleType)MFX_HANDLE_VA_DISPLAY, va_dpy_);
    if (sts != MFX_ERR_NONE) {
        delete pmfxSession;
        MLOG_ERROR("Set handle for the session failed\n");
        return false;
    }

    GeneralAllocator* pAlloc = new GeneralAllocator;
    sts = pAlloc->Init(&va_dpy_);
    if (sts != MFX_ERR_NONE) {
        delete pmfxSession;
        delete pAlloc;
        MLOG_ERROR("Initialize the allocator failed\n");
        return false;
    }
    sts = pmfxSession->SetFrameAllocator(pAlloc);
    if (sts != MFX_ERR_NONE) {
        delete pmfxSession;
        delete pAlloc;
        MLOG_ERROR("Set the allocator for the session failed\n");
        return false;
    }

    *psession = pmfxSession;
    *palloc = pAlloc;
    return true;
}

#endif
