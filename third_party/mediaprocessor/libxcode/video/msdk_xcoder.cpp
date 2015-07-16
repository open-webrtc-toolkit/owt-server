/* <COPYRIGHT_TAG> */

/**
 *\file MsdkXcoder.cpp
 *\brief Implementation for MsdkXcoder class
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <va/va.h>
#include <va/va_drm.h>
#include "base/dispatcher.h"
#include "base/measurement.h"
#include "msdk_codec.h"
#include "msdk_xcoder.h"
#include "mfxvideo++.h"
 
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
#include "hw_device.h"
#endif

VADisplay MsdkXcoder::va_dpy_ = NULL;
pthread_mutex_t MsdkXcoder::va_mutex_ = PTHREAD_MUTEX_INITIALIZER;
static bool va_mutex_init = MsdkXcoder::MutexInit();

MsdkXcoder::MsdkXcoder()
{
    main_session_ = NULL;
    dri_fd_ = -1;
    is_running_ = false;
    done_init_ = false;
    dec_ = NULL;
    dec_dis_ = NULL;
    vpp_ = NULL;
    vpp_dis_ = NULL;
    enc_ = NULL;
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
    hwdev_ = NULL;
#endif
    render_ = NULL;
    m_bRender_ = false;
}

MsdkXcoder::~MsdkXcoder()
{
    CleanUpResource();
}

int MsdkXcoder::InitVaDisp()
{
    int major_version, minor_version;
    int ret = VA_STATUS_SUCCESS;

    pthread_mutex_lock(&MsdkXcoder::va_mutex_);
    assert(NULL == MsdkXcoder::va_dpy_);
    if (m_bRender_ == false) {
        dri_fd_ = open("/dev/dri/card0", O_RDWR);
        if (-1 == dri_fd_) {
            printf("Open dri failed!\n");
            pthread_mutex_unlock(&MsdkXcoder::va_mutex_);
            return -1;
        }

        MsdkXcoder::va_dpy_ = vaGetDisplayDRM(dri_fd_);
        ret = vaInitialize(MsdkXcoder::va_dpy_, &major_version, &minor_version);
        if (VA_STATUS_SUCCESS != ret) {
            printf("vaInitialize failed\n");
            pthread_mutex_unlock(&MsdkXcoder::va_mutex_);
            return -1;
        }
    //Render: to create the X11 device
    } else {
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
        int sts = 0;
        printf ("createVAAPIDevice!\n");
        // Create the device here.
        // Use x11 display. Need render the frame.
        hwdev_ = CreateVAAPIDevice();
        if (hwdev_ == NULL) {
            printf ("createVAAPIDevice fail!\n");
            return MFX_ERR_MEMORY_ALLOC;
        }

        sts = hwdev_->Init(NULL, 1, 0);
        if (sts != MFX_ERR_NONE) {
            printf("hwdevice initialized failed\n");
            return MFX_ERR_UNKNOWN;
        }

        printf("hwdev->GetHandle!\n");
        sts = hwdev_->GetHandle(MFX_HANDLE_VA_DISPLAY, (mfxHDL*)&(MsdkXcoder::va_dpy_));
        if (sts != MFX_ERR_NONE) {
            printf("get handle failed\n");
        }
#else
        printf("Err: to support rendering, you need to define LIBVA_X11_SUPPORT in Makefile\n");
        return -1;
#endif
    }

    pthread_mutex_unlock(&MsdkXcoder::va_mutex_);

    return 0;
}

void MsdkXcoder::DestroyVaDisp()
{
    pthread_mutex_lock(&MsdkXcoder::va_mutex_);
    if (m_bRender_ == false) {
        if (NULL != MsdkXcoder::va_dpy_) {
            vaTerminate(MsdkXcoder::va_dpy_);
            MsdkXcoder::va_dpy_ = NULL;
        }
        if (dri_fd_) {
            close(dri_fd_);
            dri_fd_ = -1;
        }
    } else {
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
        if (hwdev_) {
            hwdev_->Close();
            delete hwdev_;
            hwdev_ = NULL;
        }
#endif
        MsdkXcoder::va_dpy_ = NULL;
    }
    pthread_mutex_unlock(&MsdkXcoder::va_mutex_);
    return;
}

MFXVideoSession* MsdkXcoder::GenMsdkSession()
{
    Locker<Mutex> lock(sess_mutex);

    mfxIMPL impl = MFX_IMPL_AUTO_ANY;
    mfxVersion ver = {{3, 1}};
    mfxStatus sts = MFX_ERR_NONE;

    MFXVideoSession * pSession = new MFXVideoSession;
    if (!pSession) {
        return NULL;
    }
    sts = pSession->Init(impl, &ver);
    if (MFX_ERR_NONE != sts) {
        delete pSession;
        return NULL;
    }
    sts = pSession->SetHandle((mfxHandleType)MFX_HANDLE_VA_DISPLAY, MsdkXcoder::va_dpy_);
    if (MFX_ERR_NONE != sts) {
        delete pSession;
        return NULL;
    }

    session_list_.push_back(pSession);
    return pSession;
}

void MsdkXcoder::CloseMsdkSession(MFXVideoSession *session)
{
    Locker<Mutex> lock(sess_mutex);
    if (!session) {
        printf("Invalid input session\n");
        return;
    }
    std::list<MFXVideoSession*>::iterator sess_i;
    for (sess_i = session_list_.begin(); \
            sess_i != session_list_.end(); \
            ++sess_i) {
        if (*sess_i == session) {
            (*sess_i)->Close();
            delete (*sess_i);
            (*sess_i) = NULL;
            session_list_.erase(sess_i);
            break;
        }
    }
    return;
}

void MsdkXcoder::CreateSessionAllocator(MFXVideoSession **session, GeneralAllocator **allocator)
{
    mfxStatus sts = MFX_ERR_NONE;
    *session = NULL;
    *allocator = NULL;

    GeneralAllocator *pAllocator = new GeneralAllocator;
    if (!pAllocator) {
        printf("Create allocator failed\n");
        return;
    }

    sts = pAllocator->Init(&(MsdkXcoder::va_dpy_));
    if (sts != MFX_ERR_NONE) {
        printf("Init allocator failed\n");
        delete pAllocator;
        return;
    }
    m_pAllocArray.push_back(pAllocator);

    MFXVideoSession *pSession = GenMsdkSession();
    if (!pSession) {
        printf("Init session failed\n");
        delete pAllocator;
        return;
    }

    sts = pSession->SetFrameAllocator(pAllocator);
    if (sts != MFX_ERR_NONE) {
        printf("Set frame allocator failed\n");
        delete pAllocator;
        CloseMsdkSession(pSession);
        return;
    }

    *session = pSession;
    *allocator = pAllocator;
    return;
}

void MsdkXcoder::ReadDecConfig(DecOptions *dec_cfg, void *cfg)
{
    ElementCfg *decCfg = static_cast<ElementCfg *>(cfg);
    if (dec_cfg->input_codec_type == CODEC_TYPE_VIDEO_AVC) {
        decCfg->DecParams.mfx.CodecId = MFX_CODEC_AVC;
        decCfg->input_stream = dec_cfg->inputStream;
    } else if (dec_cfg->input_codec_type == CODEC_TYPE_VIDEO_VP8) {
        decCfg->DecParams.mfx.CodecId = MFX_CODEC_VP8;
        decCfg->input_stream = dec_cfg->inputStream;
    } else if (dec_cfg->input_codec_type == CODEC_TYPE_VIDEO_STRING) {
        decCfg->DecParams.mfx.CodecId = MFX_CODEC_STRING;
        decCfg->input_str = dec_cfg->inputString;
    } else if (dec_cfg->input_codec_type == CODEC_TYPE_VIDEO_PICTURE) {
        decCfg->DecParams.mfx.CodecId = MFX_CODEC_PICTURE;
        decCfg->input_pic = dec_cfg->inputPicture;
    }
    decCfg->measuremnt = dec_cfg->measuremnt;
}

void MsdkXcoder::ReadVppConfig(VppOptions *vpp_cfg, void *cfg)
{
    ElementCfg *vppCfg = static_cast<ElementCfg *>(cfg);
    vppCfg->VppParams.vpp.Out.CropW = vpp_cfg->out_width;
    vppCfg->VppParams.vpp.Out.CropH = vpp_cfg->out_height;
    vppCfg->VppParams.vpp.Out.FourCC = vpp_cfg->colorFormat;
    vppCfg->measuremnt = vpp_cfg->measuremnt;
}

void MsdkXcoder::ReadEncConfig(EncOptions *enc_cfg, void *cfg)
{
    ElementCfg *encCfg = static_cast<ElementCfg *>(cfg);
    encCfg->output_stream = enc_cfg->outputStream;
    if (enc_cfg->output_codec_type == CODEC_TYPE_VIDEO_AVC) {
        encCfg->EncParams.mfx.CodecId = MFX_CODEC_AVC;
        encCfg->EncParams.mfx.CodecProfile = enc_cfg->profile;
        encCfg->EncParams.mfx.NumRefFrame = enc_cfg->numRefFrame;
        encCfg->EncParams.mfx.TargetUsage = enc_cfg->targetUsage;
        encCfg->EncParams.mfx.GopRefDist = enc_cfg->gopRefDist;
        encCfg->EncParams.mfx.IdrInterval= enc_cfg->idrInterval;
        encCfg->EncParams.mfx.GopPicSize = enc_cfg->intraPeriod;
        encCfg->extParams.nMaxSliceSize = enc_cfg->nMaxSliceSize;
    } else if (enc_cfg->output_codec_type == CODEC_TYPE_VIDEO_VP8) {
        encCfg->EncParams.mfx.CodecId = MFX_CODEC_VP8;
        encCfg->EncParams.Protected = enc_cfg->vp8OutFormat;
    }
    encCfg->EncParams.mfx.TargetKbps = enc_cfg->bitrate;
    encCfg->measuremnt = enc_cfg->measuremnt;
}

int MsdkXcoder::Init(DecOptions *dec_cfg, VppOptions *vpp_cfg, EncOptions *enc_cfg)
{
    Locker<Mutex> lock(mutex);
    if (!dec_cfg || !vpp_cfg || (!enc_cfg && (vpp_cfg->colorFormat != MFX_FOURCC_RGB4))) {
        printf("Init parameter invalid, return\n");
        return -1;
    }
    dec_cfg->DecHandle = NULL;
    vpp_cfg->VppHandle = NULL;
    if (enc_cfg) {
        enc_cfg->EncHandle = NULL;
    }

    m_bRender_ = (vpp_cfg->colorFormat == MFX_FOURCC_RGB4) ? true : false;
    if (0 != InitVaDisp()) {
        printf("Init VA display failed\n");
        return -1;
    }

    ElementCfg decCfg;
    memset(&decCfg, 0, sizeof(decCfg));
    ReadDecConfig(dec_cfg, &decCfg);
    int in_file_type = decCfg.DecParams.mfx.CodecId;

    ElementCfg vppCfg;
    memset(&vppCfg, 0, sizeof(vppCfg));
    ReadVppConfig(vpp_cfg, &vppCfg);

    int out_file_type = 0;
    ElementCfg renderCfg;
    ElementCfg encCfg;
    if (m_bRender_) {
        memset(&renderCfg, 0, sizeof(renderCfg));
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
        renderCfg.hwdev = hwdev_;
#endif
    } else {
        memset(&encCfg, 0, sizeof(encCfg));
        ReadEncConfig(enc_cfg, &encCfg);
        out_file_type = encCfg.EncParams.mfx.CodecId;
    }


    main_session_ = GenMsdkSession();
    if (!main_session_) {
        printf("Init Session failed\n");
        return -1;
    }

    //decoder session and allocator
    MFXVideoSession *dec_session = NULL;
    GeneralAllocator *pAllocatorDec = NULL;
    CreateSessionAllocator(&dec_session, &pAllocatorDec);
    if (!dec_session || !pAllocatorDec) {
        return -1;
    }

    //vpp session and allocator
    MFXVideoSession *vpp_session = NULL;
    GeneralAllocator *pAllocatorVpp = NULL;
    CreateSessionAllocator(&vpp_session, &pAllocatorVpp);
    if (!vpp_session || !pAllocatorVpp) {
        return -1;
    }

    //encoder session and allocator
    MFXVideoSession *enc_session = NULL;
    GeneralAllocator *pAllocatorEnc = NULL;
    //render session and allocator
    MFXVideoSession *render_session = NULL;
    GeneralAllocator *pAllocatorRender = NULL;
    if (!m_bRender_) {
        CreateSessionAllocator(&enc_session, &pAllocatorEnc);
        if (!enc_session || !pAllocatorEnc) {
            return -1;
        }
    } else {
        CreateSessionAllocator(&render_session, &pAllocatorRender);
        if (!render_session || !pAllocatorRender) {
            return -1;
        }
    }


    if (MFX_CODEC_VP8 == in_file_type) {
        dec_ = new MSDKCodec(ELEMENT_VP8_DEC, dec_session, pAllocatorDec);
    } else if (MFX_CODEC_STRING == in_file_type) {
        dec_ = new MSDKCodec(ELEMENT_STRING_DEC, dec_session, pAllocatorDec);
    } else if (MFX_CODEC_PICTURE == in_file_type) {
        dec_ = new MSDKCodec(ELEMENT_BMP_DEC, dec_session, pAllocatorDec);
    } else {
        dec_ = new MSDKCodec(ELEMENT_DECODER, dec_session, pAllocatorDec);
    }
    if (dec_) {
        dec_->Init(&decCfg, ELEMENT_MODE_ACTIVE);
    } else {
        return -1;
    }

    dec_dis_ = new Dispatcher;
    if (dec_dis_) {
        dec_dis_->Init(NULL, ELEMENT_MODE_PASSIVE);
    } else {
        return -1;
    }

    dec_list_.push_back(dec_);
    dec_dis_map_[dec_] = dec_dis_;
    main_session_->JoinSession(*dec_session);

    vpp_ = new MSDKCodec(ELEMENT_VPP, vpp_session, pAllocatorVpp);
    if (vpp_) {
        vpp_->Init(&vppCfg, ELEMENT_MODE_ACTIVE);
        vpp_->SetStrInfo(dec_->GetStrInfo());
        vpp_->SetPicInfo(dec_->GetPicInfo());
    } else {
        return -1;
    }

    vpp_dis_ = new Dispatcher;
    if (vpp_dis_) {
        vpp_dis_->Init(NULL, ELEMENT_MODE_PASSIVE);
    } else {
        return -1;
    }

    vpp_list_.push_back(vpp_);
    vpp_dis_map_[vpp_] = vpp_dis_;
    main_session_->JoinSession(*vpp_session);

    if (!m_bRender_) {
        ElementType type = GetElementType(ENCODER, out_file_type, enc_cfg->swCodecPref);
        enc_ = new MSDKCodec(type, enc_session, pAllocatorEnc);

        if (enc_) {
            enc_->Init(&encCfg, ELEMENT_MODE_ACTIVE);
        } else {
            return -1;
        }

        enc_multimap_.insert(std::make_pair(vpp_, enc_));
        main_session_->JoinSession(*enc_session);
    } else {
        render_ = new MSDKCodec(ELEMENT_RENDER, render_session, pAllocatorRender);
        main_session_->JoinSession(*render_session);
        render_->Init(&renderCfg, ELEMENT_MODE_PASSIVE);

        main_session_->JoinSession(*render_session);
    }

    //to link the elements
    dec_->LinkNextElement(dec_dis_);
    dec_dis_->LinkNextElement(vpp_);
    vpp_->LinkNextElement(vpp_dis_);
    if (m_bRender_) {
        vpp_dis_->LinkNextElement(render_);
    } else {
        vpp_dis_->LinkNextElement(enc_);
    }

    dec_cfg->DecHandle = dec_;
    vpp_cfg->VppHandle = vpp_;
    if (!m_bRender_) {
        enc_cfg->EncHandle = enc_;
    }

    done_init_ = true;
    printf("[%s] MsdkXcoder Init Done.\n", __FUNCTION__);
    return 0;
}

int MsdkXcoder::ForceKeyFrame(void *output_handle)
{
    Locker<Mutex> lock(mutex);

    if (!done_init_) {
        printf("Force key frame before initialization\n");
        return -1;
    }

    MSDKCodec *enc = NULL;
    std::multimap<MSDKCodec*, MSDKCodec*>::iterator it_enc;
    for(it_enc = enc_multimap_.begin(); it_enc != enc_multimap_.end(); ++it_enc) {
        if(it_enc->second == (MSDKCodec *)output_handle) {
            enc = it_enc->second;
            enc->SetForceKeyFrame();
            break;
        }
    }
    assert(it_enc != enc_multimap_.end());

    return 0;
}

int MsdkXcoder::SetBitrate(void *output_handle, unsigned short bitrate)
{
    Locker<Mutex> lock(mutex);
    if (!done_init_) {
        printf("Force change bitrate before initialization\n");
        return -1;
    }

    MSDKCodec *enc = NULL;
    std::multimap<MSDKCodec*, MSDKCodec*>::iterator it_enc;
    for(it_enc = enc_multimap_.begin(); it_enc != enc_multimap_.end(); ++it_enc) {
        if(it_enc->second == (MSDKCodec *)output_handle) {
            enc = it_enc->second;
            enc->SetBitrate(bitrate);
            enc->SetResetBitrateFlag();
            break;
        }
    }
    assert(it_enc != enc_multimap_.end());

    return 0;
}

/*
 * Insert LTR frame for encoder "encHandle". If encHandle is null, apply this to all ELEMENT_ENCODER
 * Returns:  0 - successful
 *          -1 - XCoder is not initialized yet
 *          -2 - failed, invalid call
 *          -3 - failed with other cause.
 *          -4 - failed, invalid encoder handle
 */
int MsdkXcoder::MarkLTR(void *encHandle)
{
    Locker<Mutex> lock(mutex);
    if (!done_init_) {
        printf("Err: MsdkXcoder is not initialized yet\n");
        return -1;
    }

    MSDKCodec *enc = NULL;
    int ret_val = 0;
    std::multimap<MSDKCodec*, MSDKCodec*>::iterator it_enc;
    for(it_enc = enc_multimap_.begin(); it_enc != enc_multimap_.end(); ++it_enc) {
        if (encHandle) {
            if (encHandle == (void *)(it_enc->second)) {
                enc = it_enc->second;
                //1. check if encoder is ELEMENT_ENCODER
                if (enc->GetCodecType() != ELEMENT_ENCODER) {
                    printf("Err: this API is for H264 hw encoder only\n");
                    return -2;
                }

                //2. call MSDKCodec::MarkLTR()
                ret_val = enc->MarkLTR();
                if (ret_val) {
                    printf("Err: failed.\n");
                    return -3;
                } else {
                    break;
                }
            }
        } else {
            //apply to all H264 ELEMENT_ENCODER
            enc = it_enc->second;
            if (enc->GetCodecType() != ELEMENT_ENCODER) {
                printf("Info: encoder %p is not H264 HW encoder, can't mark LTR\n", encHandle);
                continue;
            } else {
                ret_val = enc->MarkLTR();
                if (ret_val != 0) {
                    printf("Err: failed to mark LTR frame for encoder %p\n", encHandle);
                }
            }
        }
    }

    if (encHandle && it_enc == enc_multimap_.end()) {
        printf("Err: invalid input encHandle %p\n", encHandle);
        return -4;
    }

    return 0;
}

/*
 * Set combo type for vpp_handle.
 * If (type == COMBO_MASTER), then "master" points to the master input
 * Returns:  0 - Successful
 *          -1 - invalid inputs
 */

int MsdkXcoder::SetComboType(ComboType type, void *vpp_handle, void* master)
{
    Locker<Mutex> lock(mutex);

    int ret_val = 0;

    if (!vpp_handle || (type == COMBO_MASTER && !master)) {
        printf("Err: invalid inputs\n");
        return -1;
    }

    //map the input decoder handle (master) to its dispatcher
    Dispatcher *dec_dis = NULL;
    if (master) {
        std::map<MSDKCodec*, Dispatcher*>::iterator it_dec_dis;
        assert(dec_dis_map_.size());
        for(it_dec_dis = dec_dis_map_.begin(); it_dec_dis != dec_dis_map_.end(); ++it_dec_dis) {
            if (master == it_dec_dis->first) {
                dec_dis = it_dec_dis->second;
                break;
            }
        }

        if (dec_dis == NULL) {
            printf("Err: invalid input\n");
            assert(0);
            return -1;
        }
    }

    std::list<MSDKCodec*>::iterator vpp_it;
    for (vpp_it = vpp_list_.begin(); vpp_it != vpp_list_.end(); ++vpp_it) {
        if (*vpp_it == vpp_handle) {
            ret_val = (*vpp_it)->ConfigVppCombo(type, dec_dis);
            assert(ret_val == 0);
            break;
        }
    }
    if (vpp_it == vpp_list_.end()) {
        printf("Err: invalid vpp handle\n");
        assert(0);
        return -1;
    }

    return 0;
}

int MsdkXcoder::DetachInput(void* input_handle)
{
    Locker<Mutex> lock(mutex);
    bool dec_found = false;
    bool dec_dis_found = false;
    Dispatcher *dec_dis = NULL;
    MSDKCodec *dec = NULL;
    std::list<MSDKCodec*>::iterator it_dec;
    std::map<MSDKCodec*, Dispatcher*>::iterator it_dec_dis;

    if (!done_init_) {
        printf("[%s]Detach new stream before initialization\n", __FUNCTION__);
        return -1;
    }

    if (!input_handle) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return -1;
    }

    if (1 == dec_list_.size()) {
        printf("[%s]The last input handle, please stop the pipeline\n", __FUNCTION__);
        return -1;
    }

    dec = static_cast<MSDKCodec*>(input_handle);
    for(it_dec = dec_list_.begin(); \
            it_dec != dec_list_.end(); \
            ++it_dec) {
        if (dec == *it_dec) {
            dec_list_.erase(it_dec);
            dec_found = true;
            break;
        }
    }

    if (dec_found && dec) {
        printf("[%s]Stopping decoder...\n", __FUNCTION__);
        dec->Stop();
    } else {
        printf("[%s]Can't find the input handle", __FUNCTION__);
        return -1;
    }

    for(it_dec_dis = dec_dis_map_.begin(); \
            it_dec_dis != dec_dis_map_.end(); \
            ++it_dec_dis) {
        if (dec == it_dec_dis->first) {
            dec_dis = it_dec_dis->second;
            dec_dis_map_.erase(it_dec_dis);
            dec_dis_found = true;
            break;
        }
    }

    if (dec_dis_found && dec_dis) {
        printf("[%s]Stopping decoder dis...\n", __FUNCTION__);
        dec_dis->Stop();
    } else {
        printf("[%s]Can't find the input dispatch handle", __FUNCTION__);
        return -1;
    }

    //make sure decoder is stopped before unlinking them.
    //make sure dec_dis and vpp is unlinked in advance, so vpp can return the queued buffers
    printf("[%s]Unlinking elements dec_dis and vpp ...\n", __FUNCTION__);
    dec_dis->UnlinkNextElement();
    printf("[%s]Unlinking elements dec and dec_dis ...\n", __FUNCTION__);
    dec->UnlinkNextElement();

    if (dec_dis) {
        delete dec_dis;
        dec_dis = NULL;
    }

    if (dec) {
        del_dec_list_.push_back(dec);
    }

    printf("[%s]Detach Decoder Done\n", __FUNCTION__);
    return 0;
}

int MsdkXcoder::AttachInput(DecOptions *dec_cfg, void *vppHandle)
{
    Locker<Mutex> lock(mutex);
    bool vpp_found = false;
    std::list<MSDKCodec*>::iterator vpp_it;

    if (!done_init_) {
        printf("[%s]Attach new stream before initialization\n", __FUNCTION__);
        return -1;
    }

    if (!dec_cfg) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return -1;
    }
    dec_cfg->DecHandle = NULL;

    MSDKCodec *del_dec = NULL;
    for(std::list<MSDKCodec*>::iterator it_del_dec = del_dec_list_.begin();
            it_del_dec != del_dec_list_.end();) {
        del_dec = *it_del_dec;
        it_del_dec = del_dec_list_.erase(it_del_dec);
        printf("Deleting dec %p...\n",del_dec);
        delete  del_dec;
    }

    MSDKCodec *vpp = static_cast<MSDKCodec*>(vppHandle);
    if (vpp) {
        for (vpp_it = vpp_list_.begin(); \
                vpp_it != vpp_list_.end(); \
                ++vpp_it) {
            if (vpp == (*vpp_it)) {
                vpp_found = true;
                break;
            }
        }
        if (!vpp_found) {
            printf("[%s]Can't find the vpp handle\n", __FUNCTION__);
            return -1;
        }
        if (is_running_ && !vpp->is_running_) {
            printf("[%s]Vpp has been exit, can't attach input\n", __FUNCTION__);
            return -1;
        }
    } else {
        for (vpp_it = vpp_list_.begin(); \
                vpp_it != vpp_list_.end(); \
                ++vpp_it) {
            if (is_running_ && !((*vpp_it)->is_running_)) {
                printf("[%s]One vpp has been exit, can't attach input\n", __FUNCTION__);
                return -1;
            }
        }
    }

    ElementCfg decCfg;
    memset(&decCfg, 0, sizeof(decCfg));
    ReadDecConfig(dec_cfg, &decCfg);

    GeneralAllocator *pAllocatorDec = NULL;
    MFXVideoSession *dec_session = NULL;
    CreateSessionAllocator(&dec_session, &pAllocatorDec);
    if (!dec_session || !pAllocatorDec) {
        printf("[%s]Create decoder session or allocator failed\n", __FUNCTION__);
        return -1;
    }

    dec_ = NULL;
    if (MFX_CODEC_VP8 == decCfg.DecParams.mfx.CodecId) {
        dec_ = new MSDKCodec(ELEMENT_VP8_DEC, dec_session, pAllocatorDec);
    } else if (MFX_CODEC_STRING == decCfg.DecParams.mfx.CodecId) {
        dec_ = new MSDKCodec(ELEMENT_STRING_DEC, dec_session, pAllocatorDec);
    } else if (MFX_CODEC_PICTURE == decCfg.DecParams.mfx.CodecId) {
        dec_ = new MSDKCodec(ELEMENT_BMP_DEC, dec_session, pAllocatorDec);
    } else {
        dec_ = new MSDKCodec(ELEMENT_DECODER, dec_session, pAllocatorDec);
    }
    if (dec_) {
        dec_->Init(&decCfg, ELEMENT_MODE_ACTIVE);
    } else {
        printf("[%s]Create decoder failed\n", __FUNCTION__);
        return -1;
    }

    dec_dis_ = NULL;
    dec_dis_ = new Dispatcher;
    if (dec_dis_) {
        dec_dis_->Init(NULL, ELEMENT_MODE_PASSIVE);
    } else {
        printf("[%s]Create decoder dispatch failed\n", __FUNCTION__);
        return -1;
    }

    dec_list_.push_back(dec_);
    dec_dis_map_[dec_] = dec_dis_;

    main_session_->JoinSession(*dec_session);

    dec_->LinkNextElement(dec_dis_);
    if (vpp) {
        vpp->SetStrInfo(dec_->GetStrInfo());
        vpp->SetPicInfo(dec_->GetPicInfo());
        dec_dis_->LinkNextElement(vpp);
    } else {
        for (vpp_it = vpp_list_.begin(); \
                vpp_it != vpp_list_.end(); \
                ++vpp_it) {
            if (*vpp_it) {
                (*vpp_it)->SetStrInfo(dec_->GetStrInfo());
                (*vpp_it)->SetPicInfo(dec_->GetPicInfo());
                dec_dis_->LinkNextElement(*vpp_it);
            }
        }
    }

    if (is_running_) {
        dec_dis_->Start();
        dec_->Start();
    }

    dec_cfg->DecHandle = dec_;
    printf("[%s]Attach Decoder:%p Done.\n", __FUNCTION__, dec_);
    return 0;
}

int MsdkXcoder::DetachVpp(void* vpp_handle)
{
    Locker<Mutex> lock(mutex);

    if (m_bRender_) {
        printf("Xcoder is in local rendering mode\n");
        return -1;
    }

    MSDKCodec *vpp = NULL;
    bool vpp_found = false;
    Dispatcher *vpp_dis = NULL;
    std::list<MSDKCodec*>::iterator vpp_it;
    std::map<MSDKCodec*, Dispatcher*>::iterator it_vpp_dis;

    if (!done_init_) {
        printf("[%s]Detach new stream before initialization\n", __FUNCTION__);
        return -1;
    }

    if (!vpp_handle) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return -1;
    }

    if (1 == vpp_list_.size()) {
        printf("[%s]The last vpp handle, please stop the pipeline\n", __FUNCTION__);
        return -1;
    }

    vpp = static_cast<MSDKCodec*>(vpp_handle);
    for (vpp_it = vpp_list_.begin(); \
            vpp_it != vpp_list_.end(); \
            ++vpp_it) {
        if (vpp == *vpp_it) {
            vpp_list_.erase(vpp_it);
            vpp_found = true;
            break;
        }
    }
    if (!vpp_found) {
        printf("[%s]Can't find the vpp handle\n", __FUNCTION__);
        return -1;
    }

    printf("[%s]Stopping vpp %p...\n", __FUNCTION__, vpp);
    vpp->Stop();
    printf("[%s]vpp->UnlinkPrevElement...\n", __FUNCTION__);
    vpp->UnlinkPrevElement();
    //don't unlink vpp with the following elements at this point.
    //Or else its surfaces can't be recycled

    for(it_vpp_dis = vpp_dis_map_.begin(); \
            it_vpp_dis != vpp_dis_map_.end(); \
            ++it_vpp_dis) {
        if (vpp == it_vpp_dis->first) {
            vpp_dis = it_vpp_dis->second;
            vpp_dis_map_.erase(it_vpp_dis);
            break;
        }
    }
    if (!vpp_dis) {
        printf("[%s]Can't find the vpp dispatch handle\n", __FUNCTION__);
        return -1;
    }

    printf("[%s]Stopping vpp dis...\n", __FUNCTION__);
    vpp_dis->Stop();

    BaseElement *enc = NULL;
    typedef std::multimap<MSDKCodec*, MSDKCodec*>::iterator enc_multimap_it;
    std::pair<enc_multimap_it,enc_multimap_it> p = enc_multimap_.equal_range(vpp);
    for(enc_multimap_it k = p.first; k != p.second; k++)  {
        enc = k->second;
        printf("[%s]Stopping encoder ...\n", __FUNCTION__);
        enc->Stop();
    }
    enc_multimap_.erase(vpp);

    //It's safe to unlink the elements now as encoder is stopped already.
    printf("[%s]Unlinking vpp_dis' next element ...\n", __FUNCTION__);
    vpp_dis->UnlinkNextElement();
    printf("[%s]Vpp->UnlinkNextElement ...\n", __FUNCTION__);
    vpp->UnlinkNextElement();

    delete vpp;
    vpp = NULL;
    delete vpp_dis;
    vpp_dis = NULL;
    printf("[%s]Deleting enc...\n", __FUNCTION__);
    delete enc;
    enc = NULL;

    printf("[%s]Detach VPP Done\n", __FUNCTION__);
    return 0;
}

int MsdkXcoder::AttachVpp(DecOptions *dec_cfg, VppOptions *vpp_cfg, EncOptions *enc_cfg)
{
    Locker<Mutex> lock(mutex);

    if (m_bRender_) {
        printf("Xcoder is in local rendering mode\n");
        return -1;
    }

    if (!done_init_) {
        printf("[%s]Attach new vpp before initialization\n", __FUNCTION__);
        return -1;
    }

    if (!vpp_cfg || !enc_cfg || !dec_cfg) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return -1;
    }
    dec_cfg->DecHandle = NULL;
    vpp_cfg->VppHandle = NULL;
    enc_cfg->EncHandle = NULL;

    std::list<MSDKCodec*>::iterator vpp_it;
    for (vpp_it = vpp_list_.begin(); \
            vpp_it != vpp_list_.end(); \
            ++vpp_it) {
        if (is_running_ && !((*vpp_it)->is_running_)) {
            printf("[%s]One vpp have been exit, shouldn't attach new vpp\n", __FUNCTION__);
            return -1;
        }
    }

    ElementCfg vppCfg;
    memset(&vppCfg, 0, sizeof(vppCfg));
    ReadVppConfig(vpp_cfg, &vppCfg);

    ElementCfg encCfg;
    memset(&encCfg, 0, sizeof(encCfg));
    ReadEncConfig(enc_cfg, &encCfg);

    ElementCfg decCfg;
    memset(&decCfg, 0, sizeof(decCfg));
    ReadDecConfig(dec_cfg, &decCfg);

    /*vpp allocator, session*/
    GeneralAllocator *pAllocatorVpp = NULL;
    MFXVideoSession *vpp_session = NULL;
    CreateSessionAllocator(&vpp_session, &pAllocatorVpp);
    if (!vpp_session || !pAllocatorVpp) {
        printf("[%s]Create vpp session or allocator failed\n", __FUNCTION__);
        return -1;
    }

    /*enc allocator, session*/
    GeneralAllocator *pAllocatorEnc = NULL;
    MFXVideoSession *enc_session = NULL;
    CreateSessionAllocator(&enc_session, &pAllocatorEnc);
    if (!enc_session || !pAllocatorEnc) {
        printf("[%s]Create encoder session or allocator failed\n", __FUNCTION__);
        return -1;
    }

    /* dec allocator, session*/
    GeneralAllocator *pAllocatorDec = NULL;
    MFXVideoSession *dec_session = NULL;
    CreateSessionAllocator(&dec_session, &pAllocatorDec);
    if (!dec_session || !pAllocatorDec) {
        printf("[%s]Create decoder session or allocator failed\n", __FUNCTION__);
        return -1;
    }

    vpp_ = NULL;
    vpp_ = new MSDKCodec(ELEMENT_VPP, vpp_session, pAllocatorVpp);
    if (vpp_) {
        vpp_->Init(&vppCfg, ELEMENT_MODE_ACTIVE);
    } else {
        printf("[%s]Create vpp failed\n", __FUNCTION__);
        return -1;
    }

    vpp_dis_ = NULL;
    vpp_dis_ = new Dispatcher;
    if (vpp_dis_) {
        vpp_dis_->Init(NULL, ELEMENT_MODE_PASSIVE);
    } else {
        printf("[%s]Create vpp dispatch failed\n", __FUNCTION__);
        return -1;
    }

    int out_file_type = encCfg.EncParams.mfx.CodecId;
    ElementType type = GetElementType(ENCODER, out_file_type, enc_cfg->swCodecPref);
    enc_ = NULL;
    enc_ = new MSDKCodec(type, enc_session, pAllocatorEnc);
    if (enc_) {
        enc_->Init(&encCfg, ELEMENT_MODE_ACTIVE);
    } else {
        printf("[%s]Create encoder failed\n", __FUNCTION__);
        return -1;
    }

    dec_ = NULL;
    if (MFX_CODEC_VP8 == decCfg.DecParams.mfx.CodecId) {
        dec_ = new MSDKCodec(ELEMENT_VP8_DEC, dec_session, pAllocatorDec);
    } else if (MFX_CODEC_STRING == decCfg.DecParams.mfx.CodecId) {
        dec_ = new MSDKCodec(ELEMENT_STRING_DEC, dec_session, pAllocatorDec);
    } else if (MFX_CODEC_PICTURE == decCfg.DecParams.mfx.CodecId) {
        dec_ = new MSDKCodec(ELEMENT_BMP_DEC, dec_session, pAllocatorDec);
    } else {
        dec_ = new MSDKCodec(ELEMENT_DECODER, dec_session, pAllocatorDec);
    }
    if (dec_) {
        dec_->Init(&decCfg, ELEMENT_MODE_ACTIVE);
    } else {
        printf("[%s]Create decoder failed\n", __FUNCTION__);
        return -1;
    }

    dec_dis_ = NULL;
    dec_dis_ = new Dispatcher;
    if (dec_dis_) {
        dec_dis_->Init(NULL, ELEMENT_MODE_PASSIVE);
    } else {
        printf("[%s]Create decoder dispatch failed\n", __FUNCTION__);
        return -1;
    }

    dec_list_.push_back(dec_);
    dec_dis_map_[dec_] = dec_dis_;
    vpp_list_.push_back(vpp_);
    vpp_dis_map_[vpp_] = vpp_dis_;
    enc_multimap_.insert(std::make_pair(vpp_, enc_));

    main_session_->JoinSession(*vpp_session);
    main_session_->JoinSession(*enc_session);
    main_session_->JoinSession(*dec_session);

    dec_->LinkNextElement(dec_dis_);
    dec_dis_->LinkNextElement(vpp_);
    vpp_->LinkNextElement(vpp_dis_);
    vpp_dis_->LinkNextElement(enc_);

    if (is_running_) {
        enc_->Start();
        vpp_dis_->Start();
        vpp_->Start();
        dec_dis_->Start();
        dec_->Start();
    }

    dec_cfg->DecHandle = dec_;
    vpp_cfg->VppHandle = vpp_;
    enc_cfg->EncHandle = enc_;
    printf("[%s]Attach VPP:%p, Encoder:%p Done.\n", __FUNCTION__, vpp_, enc_);
    return 0;
}

int MsdkXcoder::DetachOutput(void* output_handle)
{
    Locker<Mutex> lock(mutex);
    bool enc_found = false;
    std::multimap<MSDKCodec*, MSDKCodec*>::iterator it_enc;

    if (!done_init_) {
        printf("[%s]Detach stream before initialization\n", __FUNCTION__);
        return -1;
    }

    if (!output_handle) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return -1;
    }

    if (1 == enc_multimap_.size()) {
        printf("[%s]The last output handle, please stop the pipeline\n", __FUNCTION__);
        return -1;
    }

    MSDKCodec *enc = static_cast<MSDKCodec*>(output_handle);
    for(it_enc = enc_multimap_.begin(); \
            it_enc != enc_multimap_.end(); \
            ++it_enc) {
        if (enc == it_enc->second) {
            if (1 == enc_multimap_.count(it_enc->first)) {
                printf("[%s]Last enc:%p detach frome vpp:%p, please stop this vpp\n", \
                    __FUNCTION__, enc, it_enc->first);
                return -1;
            } else {
                enc_multimap_.erase(it_enc);
                printf("[%s]Stopping encoder ...\n", __FUNCTION__);
                enc->Stop();
                printf("[%s]Unlinking element ...\n", __FUNCTION__);
                //don't unlink enc from previous elements before it's stopped.
                enc->UnlinkPrevElement();
                printf("[%s]Deleting enc...\n", __FUNCTION__);
                delete enc;
                enc = NULL;
            }
            enc_found = true;
            break;
        }
    }

    if (!enc_found) {
        printf("[%s]Can't find the output handle\n", __FUNCTION__);
        return -1;
    }

    printf("[%s]Detach Encoder Done\n", __FUNCTION__);
    return 0;
}

int MsdkXcoder::AttachOutput(EncOptions *enc_cfg, void *vppHandle)
{
    Locker<Mutex> lock(mutex);

    if (m_bRender_) {
        printf("Xcoder is in local rendering mode\n");
        return -1;
    }

    bool vpp_found = false;
    bool vpp_dis_found = false;
    std::list<MSDKCodec*>::iterator vpp_it;
    std::map<MSDKCodec*, Dispatcher*>::iterator vpp_dis_it;

    if (!done_init_) {
        printf("[%s]Attach new stream before initialization\n", __FUNCTION__);
        return -1;
    }

    if (!enc_cfg || !vppHandle) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return -1;
    }
    enc_cfg->EncHandle = NULL;

    ElementCfg encCfg;
    memset(&encCfg, 0, sizeof(encCfg));
    ReadEncConfig(enc_cfg, &encCfg);

    MSDKCodec *vpp = static_cast<MSDKCodec*>(vppHandle);
    if (vpp) {
        for (vpp_it = vpp_list_.begin(); vpp_it != vpp_list_.end(); ++vpp_it) {
            if (vpp == (*vpp_it)) {
                vpp_found = true;
                break;
            }
        }
    }
    if (!vpp_found) {
        printf("[%s]Can't find the vpp handle\n", __FUNCTION__);
        return -1;
    }
    if (is_running_ && !vpp->is_running_) {
        printf("[%s]Vpp has been exit, can't attach output\n", __FUNCTION__);
        return -1;
    }

    Dispatcher *vpp_dis = NULL;
    for (vpp_dis_it = vpp_dis_map_.begin(); \
            vpp_dis_it != vpp_dis_map_.end(); \
            ++vpp_dis_it) {
        if (vpp == vpp_dis_it->first) {
            vpp_dis = vpp_dis_it->second;
            vpp_dis_found = true;
        }
    }
    if (!vpp_dis_found || !vpp_dis) {
        printf("[%s]Can't find the vpp dispatch handle\n", __FUNCTION__);
        return -1;
    }

    GeneralAllocator *pAllocatorEnc = NULL;
    MFXVideoSession *enc_session = NULL;
    CreateSessionAllocator(&enc_session, &pAllocatorEnc);
    if (!enc_session || !pAllocatorEnc) {
        printf("[%s]Create encoder session or allocator failed\n", __FUNCTION__);
        return -1;
    }

    unsigned int out_file_type = encCfg.EncParams.mfx.CodecId;
    ElementType type = GetElementType(ENCODER, out_file_type, enc_cfg->swCodecPref);
    enc_ = NULL;
    enc_ = new MSDKCodec(type, enc_session, pAllocatorEnc);
    if (enc_) {
        enc_->Init(&encCfg, ELEMENT_MODE_ACTIVE);
    } else {
        printf("[%s]Create encoder failed\n", __FUNCTION__);
        return -1;
    }

    enc_multimap_.insert(std::make_pair(vpp, enc_));

    main_session_->JoinSession(*enc_session);

    vpp_dis->LinkNextElement(enc_);
    if (is_running_) {
        enc_->Start();
    }

    enc_cfg->EncHandle = enc_;
    printf("[%s]Attach Encoder:%p Done.\n", __FUNCTION__, enc_);
    return 0;
}
/***********************************************************************/
/* time: begins from transcoding start, and unit is second */
/* decCfg: includes point to string info that user want to blend */
/**********************************************************************/
void MsdkXcoder::StringOperateAttach(DecOptions *dec_cfg, void *vppHandle, unsigned long time)
{
    bool vpp_found = false;
    MSDKCodec *vpp = NULL;

    if (!dec_cfg || !vppHandle) {
        printf("Invalid input parameters\n");
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        std::list<MSDKCodec*>::iterator vpp_it;
        vpp_it = vpp_list_.begin();
        vpp = static_cast<MSDKCodec*>(vppHandle);
        for (; vpp_it != vpp_list_.end(); ++vpp_it) {
            if (vpp == *vpp_it) {
                vpp_found = true;
            }
        }
        if (!vpp_found) {
            printf("Can't find VPP, please check it\n");
            return;
        }
    }

    int stream_cnt = vpp->QueryStreamCnt();
    if (stream_cnt == 0) {
        printf("There is no video stream attached on this vpp, so can't attach string now.\n");
        return;
    }

    unsigned int fps_n = vpp->GetVppOutFpsN();
    unsigned int fps_d = vpp->GetVppOutFpsD();
    float frame_rate = (1.0 * fps_n) / fps_d;
    unsigned long frame_num = 0;
    frame_num = (unsigned long)(time * frame_rate);
    unsigned long vppframe_num = vpp->GetNumOfVppFrm();

    if (frame_num <= vppframe_num) {
        printf("Vpp process %lu frames, User wants to attach a new string on %lu frames.\n", vppframe_num, frame_num);
        printf("Attach at once.\n");

        AttachInput(dec_cfg, vppHandle);
    } else {
        printf("Vpp process %lu frames, User wants to attach a new string on %lu frames.\n", vppframe_num, frame_num);
        printf("Wait for attach.\n");
        while (frame_num > (vppframe_num + 1)) {
            usleep(1000);
            vppframe_num = vpp->GetNumOfVppFrm();
        }
        printf("Can attach now.\n");

        AttachInput(dec_cfg, vppHandle);
    }
    return;
}

/******************************************************************/
/* time: begins from transcoding start, and unit is second */
/* decCfg: includes point to string info that user want to detach */
/******************************************************************/
void MsdkXcoder::StringOperateDetach(DecOptions *dec_cfg, void *vppHandle, unsigned long time)
{
    bool vpp_found = false;
    MSDKCodec *vpp = NULL;
    void *old_dec = NULL;

    if (!dec_cfg || !vppHandle) {
        printf("Invalid input parameters\n");
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        std::list<MSDKCodec*>::iterator vpp_it;
        vpp_it = vpp_list_.begin();
        vpp = static_cast<MSDKCodec*>(vppHandle);
        for (; vpp_it != vpp_list_.end(); ++vpp_it) {
            if (vpp == *vpp_it) {
                vpp_found = true;
            }
        }
        if (!vpp_found) {
            printf("Can't find VPP, please check it\n");
            return;
        }
    }

    old_dec = dec_cfg->DecHandle;
    if (!old_dec) {
        printf("Invalid old dec parameter\n");
        return;
    }

    unsigned int fps_n = vpp->GetVppOutFpsN();
    unsigned int fps_d = vpp->GetVppOutFpsD();
    float frame_rate = (1.0 * fps_n) / fps_d;
    unsigned long frame_num = 0;
    frame_num = (unsigned long)(time * frame_rate);
    unsigned long vppframe_num = vpp->GetNumOfVppFrm();

    if (frame_num <= vppframe_num) {
        printf("Vpp process %lu frames, User wants to detach a existed string on %lu frames.\n", vppframe_num, frame_num);
        printf("Detach at once.\n");

        DetachInput(old_dec);
    } else {
        printf("Vpp process %lu frames, User wants to detach a existed string on %lu frames.\n", vppframe_num, frame_num);
        printf("Wait for detach.\n");
        while (frame_num > (vppframe_num + 1)) {
            usleep(1000);
            vppframe_num = vpp->GetNumOfVppFrm();
        }
        printf("Can detach now.\n");

        DetachInput(old_dec);
    }
    return;
}

/******************************************************************************************************/
/* time: begins from transcoding start, and unit is second */
/* decCfg: includes point to string info that user want to some existed string to be changed into */
/******************************************************************************************************/
void MsdkXcoder::StringOperateChange(DecOptions *dec_cfg, void *vppHandle, unsigned long time)
{
    bool vpp_found = false;
    MSDKCodec *vpp = NULL;
    void *old_dec = NULL;

    if (!dec_cfg || !vppHandle) {
        printf("Invalid input parameters\n");
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        std::list<MSDKCodec*>::iterator vpp_it;
        vpp_it = vpp_list_.begin();
        vpp = static_cast<MSDKCodec*>(vppHandle);
        for (; vpp_it != vpp_list_.end(); ++vpp_it) {
            if (vpp == *vpp_it) {
                vpp_found = true;
            }
        }
        if (!vpp_found) {
            printf("Can't find VPP, please check it\n");
            return;
        }
    }

    int stream_cnt = vpp->QueryStreamCnt();
    if (stream_cnt == 0) {
        printf("There is no video stream attached on this vpp, so can't change string now.\n");
        return;
    }

    old_dec = dec_cfg->DecHandle;
    if (!old_dec) {
        printf("Invalid old dec parameter\n");
        return;
    }

    unsigned int fps_n = vpp->GetVppOutFpsN();
    unsigned int fps_d = vpp->GetVppOutFpsD();
    float frame_rate = (1.0 * fps_n) / fps_d;
    unsigned long frame_num = 0;
    frame_num = (unsigned long)(time * frame_rate);
    unsigned long vppframe_num = vpp->GetNumOfVppFrm();

    if (frame_num <= vppframe_num) {
        printf("Vpp process %lu frames, User wants to changed on %lu frames.\n", vppframe_num, frame_num);
        printf("Change at once.\n");

        DetachInput(old_dec);
        usleep(10000); //to make sure vpp can re-init
        AttachInput(dec_cfg, vppHandle);
        vpp->SetCompInfoChe(true);
    } else {
        printf("Vpp process %lu frames, User wants to changed on %lu frames.\n", vppframe_num, frame_num);
        printf("Wait for change.\n");
        while (frame_num > (vppframe_num + 1)) {
            usleep(1000);
            vppframe_num = vpp->GetNumOfVppFrm();
        }
        printf("Can change now.\n");

        DetachInput(old_dec);
        usleep(10000);
        AttachInput(dec_cfg, vppHandle);
        vpp->SetCompInfoChe(true);
    }
    return;
}

/***********************************************************************/
/* time: begins from transcoding start, and unit is second */
/* decCfg: includes point to picture info that user want to blend */
/**********************************************************************/
void MsdkXcoder::PicOperateAttach(DecOptions *dec_cfg, void *vppHandle, unsigned long time)
{
    bool vpp_found = false;
    MSDKCodec *vpp = NULL;

    if (!dec_cfg || !vppHandle) {
        printf("Invalid input parameters\n");
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        std::list<MSDKCodec*>::iterator vpp_it;
        vpp_it = vpp_list_.begin();
        vpp = static_cast<MSDKCodec*>(vppHandle);
        for (; vpp_it != vpp_list_.end(); ++vpp_it) {
            if (vpp == *vpp_it) {
                vpp_found = true;
            }
        }
        if (!vpp_found) {
            printf("Can't find VPP, please check it\n");
            return;
        }
    }

    int stream_cnt = vpp->QueryStreamCnt();
    if (stream_cnt == 0) {
        printf("There is no video stream attached on this vpp, so can't attach picture now.\n");
        return;
    }
    unsigned int fps_n = vpp->GetVppOutFpsN();
    unsigned int fps_d = vpp->GetVppOutFpsD();
    float frame_rate = (1.0 * fps_n) / fps_d;
    unsigned long frame_num = 0;
    frame_num = (unsigned long)(time * frame_rate);
    unsigned long vppframe_num = vpp->GetNumOfVppFrm();

    if (frame_num <= vppframe_num) {
        printf("Vpp process %lu frames, User wants to attach a new picture on %lu frames.\n", vppframe_num, frame_num);
        printf("Attach at once.\n");

        AttachInput(dec_cfg, vppHandle);
    } else {
        printf("Vpp process %lu frames, User wants to attach a new picture on %lu frames.\n", vppframe_num, frame_num);
        printf("Wait for attach.\n");
        while (frame_num > (vppframe_num + 1)) {
            usleep(1000);
            vppframe_num = vpp->GetNumOfVppFrm();
        }
        printf("Can attach now.\n");

        AttachInput(dec_cfg, vppHandle);
    }
    return;
}

/******************************************************************/
/* time: begins from transcoding start, and unit is second */
/* decCfg: includes point to picture info that user want to detach */
/******************************************************************/
void MsdkXcoder::PicOperateDetach(DecOptions *dec_cfg, void *vppHandle, unsigned long time)
{
    bool vpp_found = false;
    MSDKCodec *vpp = NULL;
    void *old_dec = NULL;

    if (!dec_cfg || !vppHandle) {
        printf("invalid decCfg address.\n");
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        std::list<MSDKCodec*>::iterator vpp_it;
        vpp_it = vpp_list_.begin();
        vpp = static_cast<MSDKCodec*>(vppHandle);
        for (; vpp_it != vpp_list_.end(); ++vpp_it) {
            if (vpp == *vpp_it) {
                vpp_found = true;
            }
        }
        if (!vpp_found) {
            printf("Can't find VPP, please check it\n");
            return;
        }
    }

    old_dec = dec_cfg->DecHandle;
    if (!old_dec) {
        printf("Invalid old dec parameter\n");
        return;
    }

    unsigned int fps_n = vpp->GetVppOutFpsN();
    unsigned int fps_d = vpp->GetVppOutFpsD();
    float frame_rate = (1.0 * fps_n) / fps_d;
    unsigned long frame_num = 0;
    frame_num = (unsigned long)(time * frame_rate);
    unsigned long vppframe_num = vpp->GetNumOfVppFrm();

    if (frame_num <= vppframe_num) {
        printf("Vpp process %lu frames, User wants to detach a existed picture on %lu frames.\n", vppframe_num, frame_num);
        printf("Detach at once.\n");

        DetachInput(old_dec);
    } else {
        printf("Vpp process %lu frames, User wants to detach a existed picture on %lu frames.\n", vppframe_num, frame_num);
        printf("Wait for detach.\n");
        while (frame_num > (vppframe_num + 1)) {
            usleep(1000);
            vppframe_num = vpp->GetNumOfVppFrm();
        }
        printf("Can detach now.\n");

        DetachInput(old_dec);
    }
    return;
}

/******************************************************************************************************/
/* time: begins from transcoding start, and unit is second */
/* decCfg: includes point to picture info that user want to some existed picture to be changed into */
/******************************************************************************************************/
void MsdkXcoder::PicOperateChange(DecOptions *dec_cfg, void *vppHandle, unsigned long time)
{
    bool vpp_found = false;
    MSDKCodec *vpp = NULL;
    void *old_dec = NULL;

    if (!dec_cfg || !vppHandle) {
        printf("Invalid input parameters\n");
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        std::list<MSDKCodec*>::iterator vpp_it;
        vpp_it = vpp_list_.begin();
        vpp = static_cast<MSDKCodec*>(vppHandle);
        for (; vpp_it != vpp_list_.end(); ++vpp_it) {
            if (vpp == *vpp_it) {
                vpp_found = true;
            }
        }
        if (!vpp_found) {
            printf("Can't find VPP, please check it\n");
            return;
        }
    }

    int stream_cnt = vpp->QueryStreamCnt();
    if (stream_cnt == 0) {
        printf("There is no video stream attached on this vpp, so can't change picture now.\n");
        return;
    }

    old_dec = dec_cfg->DecHandle;
    if (!old_dec) {
        printf("Invalid old dec parameter\n");
        return;
    }

    unsigned int fps_n = vpp->GetVppOutFpsN();
    unsigned int fps_d = vpp->GetVppOutFpsD();
    float frame_rate = (1.0 * fps_n) / fps_d;
    unsigned long frame_num = 0;
    frame_num = (unsigned long)(time * frame_rate);
    unsigned long vppframe_num = vpp->GetNumOfVppFrm();

    if (frame_num <= vppframe_num) {
        printf("Vpp process %lu frames, User wants to changed on %lu frames.\n", vppframe_num, frame_num);
        printf("Change at once.\n");

        DetachInput(old_dec);
        usleep(10000);
        AttachInput(dec_cfg, vppHandle);
        vpp->SetCompInfoChe(true);
    } else {
        printf("Vpp process %lu frames, User wants to changed on %lu frames.\n", vppframe_num, frame_num);
        printf("Wait for change.\n");
        while (frame_num > (vppframe_num + 1)) {
            usleep(1000);
            vppframe_num = vpp->GetNumOfVppFrm();
        }
        printf("Can change now.\n");

        DetachInput(old_dec);
        usleep(10000);
        AttachInput(dec_cfg, vppHandle);
        vpp->SetCompInfoChe(true);
    }
    return;
}

bool MsdkXcoder::Start()
{
    Locker<Mutex> lock(mutex);
    std::list<MSDKCodec*>::iterator dec_itor;
    std::map<MSDKCodec*, Dispatcher*>::iterator dec_dis_itor;
    std::list<MSDKCodec*>::iterator vpp_itor;
    std::map<MSDKCodec*, Dispatcher*>::iterator vpp_dis_itor;
    std::multimap<MSDKCodec*, MSDKCodec*>::iterator enc_itor;

    bool res = true;
    printf("Starting Xcoder...\n");

    if (m_bRender_) {
        assert(render_);
        render_->Start();
    }

    /*start encoder*/
    for (enc_itor = enc_multimap_.begin(); \
            enc_itor != enc_multimap_.end(); \
            ++enc_itor) {
        res &= (enc_itor->second)->Start();
    }

    for (vpp_dis_itor = vpp_dis_map_.begin(); \
            vpp_dis_itor != vpp_dis_map_.end(); \
            ++vpp_dis_itor) {
        res &= (vpp_dis_itor->second)->Start();
    }

    for (vpp_itor = vpp_list_.begin(); \
            vpp_itor != vpp_list_.end(); \
            ++vpp_itor) {
        res &= (*vpp_itor)->Start();
    }

    for (dec_dis_itor = dec_dis_map_.begin(); \
            dec_dis_itor != dec_dis_map_.end(); \
            ++dec_dis_itor) {
        res &= (dec_dis_itor->second)->Start();
    }

    for (dec_itor = dec_list_.begin(); \
            dec_itor != dec_list_.end(); \
            ++dec_itor) {
        res &= (*dec_itor)->Start();
    }

    is_running_ = true;

    return res;
}


void MsdkXcoder::CleanUpResource()
{
    Locker<Mutex> lock(mutex);
    if (is_running_) {
        this->Stop();
    }

    std::list<MSDKCodec*>::iterator item;
    std::map<MSDKCodec*, Dispatcher*>::iterator dec_dis_itor;
    std::list<MSDKCodec*>::iterator vpp_itor;
    std::map<MSDKCodec*, Dispatcher*>::iterator vpp_dis_itor;
    std::multimap<MSDKCodec*, MSDKCodec*>::iterator enc_itor;
    std::list<MFXVideoSession*>::reverse_iterator sess_i;
    std::vector<GeneralAllocator*>::iterator pAllocArray_i;

    if (m_bRender_) {
        delete render_;
        render_ = NULL;
    }

    for (enc_itor = enc_multimap_.begin(); \
            enc_itor != enc_multimap_.end(); \
            ++enc_itor) {
        if (enc_itor->second) {
            if (enc_itor->second == enc_) {
                delete enc_;
                enc_ = NULL;
            } else {
                delete (enc_itor->second);
            }
        }
    }
    enc_multimap_.clear();

    for (vpp_dis_itor = vpp_dis_map_.begin(); \
            vpp_dis_itor != vpp_dis_map_.end(); \
            ++vpp_dis_itor) {
        if (vpp_dis_itor->second) {
            if (vpp_dis_itor->second == vpp_dis_) {
                delete vpp_dis_;
                vpp_dis_ = NULL;
            } else {
                delete (vpp_dis_itor->second);
            }
        }
    }
    vpp_dis_map_.clear();

    for (vpp_itor = vpp_list_.begin(); \
            vpp_itor != vpp_list_.end(); \
            ++vpp_itor) {
        if (*vpp_itor) {
            if (*vpp_itor == vpp_) {
                delete vpp_;
                vpp_ = NULL;
            } else {
                delete (*vpp_itor);
            }
        }
    }
    vpp_list_.clear();

    for (dec_dis_itor = dec_dis_map_.begin(); \
            dec_dis_itor != dec_dis_map_.end(); \
            ++dec_dis_itor) {
        if (dec_dis_itor->second) {
            if (dec_dis_itor->second == dec_dis_) {
                delete dec_dis_;
                dec_dis_ = NULL;
            } else {
                delete (dec_dis_itor->second);
            }
        }
    }
    dec_dis_map_.clear();

    for (item = dec_list_.begin(); \
            item != dec_list_.end(); \
            ++item) {
        if (*item) {
            if (*item == dec_) {
                delete dec_;
                dec_ = NULL;
            } else {
                delete (*item);
            }
        }
    }
    dec_list_.clear();

    for (item = del_dec_list_.begin(); \
            item != del_dec_list_.end(); \
            ++item) {
        if (*item) {
            delete (*item);
        }
    }
    del_dec_list_.clear();

    for (sess_i = session_list_.rbegin();
            sess_i != session_list_.rend();
            ++sess_i) {
        if (*sess_i) {
            (*sess_i)->Close();
            delete (*sess_i);
            (*sess_i) = NULL;
        }
    }
    session_list_.clear();

    for (pAllocArray_i = m_pAllocArray.begin(); \
            pAllocArray_i != m_pAllocArray.end(); \
            ++pAllocArray_i) {
        if (*pAllocArray_i) {
            delete (*pAllocArray_i);
            (*pAllocArray_i) = NULL;
        }
    }
    m_pAllocArray.clear();

    DestroyVaDisp();
}


bool MsdkXcoder::Join()
{
    Locker<Mutex> lock(mutex);
    std::list<MSDKCodec *>::iterator dec_itor;
    std::map<MSDKCodec*, Dispatcher*>::iterator dec_dis_itor;
    std::list<MSDKCodec*>::iterator vpp_itor;
    std::map<MSDKCodec*, Dispatcher*>::iterator vpp_dis_itor;
    std::multimap<MSDKCodec*, MSDKCodec*>::iterator enc_itor;
    bool res  = true;

    for (dec_itor = dec_list_.begin(); \
            dec_itor != dec_list_.end(); \
            dec_itor++) {
        res &= (*dec_itor)->Join();
    }

    for (dec_dis_itor = dec_dis_map_.begin(); \
            dec_dis_itor != dec_dis_map_.end(); \
            ++dec_dis_itor) {
        res &= (dec_dis_itor->second)->Join();
    }

    for (vpp_itor = vpp_list_.begin(); \
            vpp_itor != vpp_list_.end(); \
            ++vpp_itor) {
        res &= (*vpp_itor)->Join();
    }

    for (vpp_dis_itor = vpp_dis_map_.begin(); \
            vpp_dis_itor != vpp_dis_map_.end(); \
            ++vpp_dis_itor) {
        res &= (vpp_dis_itor->second)->Join();
    }

    for (enc_itor = enc_multimap_.begin(); \
            enc_itor != enc_multimap_.end(); \
            enc_itor++) {
        res &= (enc_itor->second)->Join();
    }

    is_running_ = false;
    return true;
}

bool MsdkXcoder::Stop()
{
    Locker<Mutex> lock(mutex);
    std::list<MSDKCodec *>::iterator dec_itor;
    std::map<MSDKCodec*, Dispatcher*>::iterator dec_dis_itor;
    std::list<MSDKCodec*>::iterator vpp_itor;
    std::map<MSDKCodec*, Dispatcher*>::iterator vpp_dis_itor;
    std::multimap<MSDKCodec*, MSDKCodec*>::iterator enc_itor;
    bool res  = true;

    printf("Stopping decoders..., decoder size: %lu\n", dec_list_.size());
    for (dec_itor = dec_list_.begin(); \
            dec_itor != dec_list_.end(); \
            dec_itor++) {
        res &= (*dec_itor)->Stop();
    }

    printf("Stopping dec dis..., dec dis size: %lu\n", dec_dis_map_.size());
    for (dec_dis_itor = dec_dis_map_.begin(); \
            dec_dis_itor != dec_dis_map_.end(); \
            ++dec_dis_itor) {
        res &= (dec_dis_itor->second)->Stop();
    }

    printf("Stopping vpp..., vpp size: %lu\n", vpp_list_.size());
    for (vpp_itor = vpp_list_.begin(); \
            vpp_itor != vpp_list_.end(); \
            ++vpp_itor) {
        res &= (*vpp_itor)->Stop();
    }

    printf("Stopping vpp dis..., vpp dis size: %lu\n", vpp_dis_map_.size());
    for (vpp_dis_itor = vpp_dis_map_.begin(); \
            vpp_dis_itor != vpp_dis_map_.end(); \
            ++vpp_dis_itor) {
        res &= (vpp_dis_itor->second)->Stop();
    }

    printf("Stopping encoders..., encoder size: %lu\n", enc_multimap_.size());
    for (enc_itor = enc_multimap_.begin();
            enc_itor != enc_multimap_.end();
            enc_itor++) {
        res &= (enc_itor->second)->Stop();
    }

    printf("----- Done!!!!\n");
    is_running_ = false;

    return true;
}

/*
 * Reset output resolution.
 * Returns: -1 - xcoder is not initialized, failed.
 *          -2 - invalid input parameters
 *           0 - Successful
 */
int MsdkXcoder::SetResolution(void *vppHandle, unsigned int width, unsigned int height)
{
    Locker<Mutex> lock(mutex);

    if (!done_init_) {
        printf("Force change resolution before initializaion\n");
        return -1;
    }

    //Check if the inputs are valid
    if (!vppHandle || !width || !height) {
        printf("Err: invalid input parameters.\n");
        return -2;
    }


    //reset vpp to new res, encoder will be reset when new frame arrives
    //TODO: to reset desired vpp but not all of them.
    MSDKCodec *vpp = static_cast<MSDKCodec*>(vppHandle);
    std::list<MSDKCodec*>::iterator it_vpp;
    for (it_vpp = vpp_list_.begin(); it_vpp != vpp_list_.end(); ++it_vpp) {
        if (*it_vpp == vpp) {
            (*it_vpp)->SetRes(width, height);
            return 0;
        }
    }

    printf("Err: specified vppHandle is not in vpp_list_\n");
    return -2;
}

/*
 * Set layout for CUSTOM mode
 * Returns: 0 - Successful
 *          1 - Warning, it does't apply to non-vpp element
 *          2 - Warning, VPP is not in COMBO_CUSTOM mode
 *          3 - Warnign, vpp has not finished the last request.
 *         -1 - Invalid inputs
 */
int MsdkXcoder::SetCustomLayout(void *vppHandle, const CustomLayout* layout)
{
    Locker<Mutex> lock(mutex);
    int ret_val = 0;

    //Check if the inputs are null
    if (!vppHandle || !layout) {
        printf("Err: null input\n");
        return -1;
    }

    //Check if the vppHandle is valid
    MSDKCodec *vpp = static_cast<MSDKCodec *>(vppHandle);
    std::list<MSDKCodec*>::iterator it_vpp;
    for (it_vpp = vpp_list_.begin(); it_vpp != vpp_list_.end(); ++it_vpp) {
        if (*it_vpp == vpp) {
            break;
        }
    }
    if (it_vpp == vpp_list_.end()) {
        printf("Err: input vppHandle %p is not valid\n", vppHandle);
        return -1;
    }

    //The input parameter "layout" maps Region to "decoder handle",
    //Create one CustomLayout instance to map Region to "decoder dispatcher handle".
    CustomLayout dis_layout;
    for(CustomLayout::const_iterator it = layout->begin(); it != layout->end(); ++it) {
        if(dec_dis_map_.find((MSDKCodec *)it->handle) == dec_dis_map_.end()) {
            printf("handle %p is not found in dec_dis_map_\n", it->handle);
            continue;
        }
        //check the region information
        Region info = it->region;
        if (info.left < 0.0 || info.left >= 1.0 ||
            info.top < 0.0 || info.top >= 1.0 ||
            info.width_ratio <= 0.0 || info.width_ratio > 1.0 ||
            info.height_ratio <= 0.0 || info.height_ratio > 1.0 ||
            info.left + info.width_ratio > 1.0 ||
            info.top + info.height_ratio > 1.0) {
            printf("War: invalid Region infomation, left:%.2f, top:%.2f, width_ratio:%.2f, height_ratio:%.2f\n", \
                info.left, info.top, info.width_ratio, info.height_ratio);
        }

        CompRegion input = {dec_dis_map_[(MSDKCodec *)it->handle], it->region};
        dis_layout.push_back(input);
    }

    ret_val = vpp->SetCompRegion(&dis_layout);
    assert(ret_val >= 0);

    return ret_val;
}

int MsdkXcoder::SetBackgroundColor(void *vppHandle, BgColor *bgColor)
{
    Locker<Mutex> lock(mutex);

    if (!done_init_) {
        printf("[%s]set background color before initialization\n", __FUNCTION__);
        return -1;
    }

    if (!vppHandle || !bgColor) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return -1;
    }

    MSDKCodec *vpp = static_cast<MSDKCodec*>(vppHandle);
    std::list<MSDKCodec*>::iterator it_vpp;
    for (it_vpp = vpp_list_.begin(); it_vpp != vpp_list_.end(); ++it_vpp) {
        if (*it_vpp == vpp) {
            BgColorInfo bgColorInfo;
            bgColorInfo.Y = bgColor->Y;
            bgColorInfo.U = bgColor->U;
            bgColorInfo.V = bgColor->V;
            int ret = (*it_vpp)->SetBgColor(&bgColorInfo);
            if (ret != 0) {
                printf("[%s]Set background error\n", __FUNCTION__);
                return -1;
            } else {
                return 0;
            }
        }
    }

    printf("[%s]Can't find the vpp handle\n", __FUNCTION__);
    return -1;
}

/*
 * To get the element type to use.
 */
ElementType MsdkXcoder::GetElementType(CoderType type, unsigned int codec_type, bool swCodecPref)
{
    ElementType ret;

    if (type == ENCODER) {
        switch (codec_type) {
        case MFX_CODEC_VP8:
            //check if vp8 hw codec exists
#ifdef MFX_DISPATCHER_EXPOSED_PREFIX
            if (!swCodecPref) {
                assert(main_session_);
                mfxPluginUID uid = MFX_PLUGINID_VP8E_HW;
                mfxStatus sts = MFXVideoUSER_Load(*main_session_, &uid, 1/*version*/);
                if (sts == MFX_ERR_NONE) {
                    ret = ELEMENT_ENCODER;
                    mfxPluginUID uid = MFX_PLUGINID_VP8E_HW;
                    MFXVideoUSER_UnLoad(*main_session_, &uid);
                } else
                ret = ELEMENT_VP8_ENC;
            } else
#endif
                ret = ELEMENT_VP8_ENC;
            break;
        case MFX_CODEC_AVC:
            ret = ELEMENT_ENCODER;
            break;
        default:
            ret = ELEMENT_ENCODER;
            break;
        }
    } else {
        switch (codec_type) {
        case MFX_CODEC_VP8:
            ret = ELEMENT_VP8_DEC;
            break;
        case MFX_CODEC_AVC:
            ret = ELEMENT_DECODER;
            break;
        default:
            ret = ELEMENT_DECODER;
            break;
        }
    }

    return ret;
}
