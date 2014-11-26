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
    if (NULL == MsdkXcoder::va_dpy_) {
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
    }
    pthread_mutex_unlock(&MsdkXcoder::va_mutex_);

    return 0;
}

void MsdkXcoder::DestroyVaDisp()
{
    pthread_mutex_lock(&MsdkXcoder::va_mutex_);
    if (NULL != MsdkXcoder::va_dpy_) {
        vaTerminate(MsdkXcoder::va_dpy_);
        MsdkXcoder::va_dpy_ = NULL;
    }
    if (dri_fd_) {
        close(dri_fd_);
        dri_fd_ = -1;
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
    if (!dec_cfg || !vpp_cfg || !enc_cfg) {
        printf("Init parameter invalid, return\n");
        return -1;
    }
    dec_cfg->DecHandle = NULL;
    vpp_cfg->VppHandle = NULL;
    enc_cfg->EncHandle = NULL;

    if (0 != InitVaDisp()) {
        printf("Init VA display failed\n");
        return -1;
    }

    ElementCfg decCfg;
    memset(&decCfg, 0, sizeof(decCfg));
    ReadDecConfig(dec_cfg, &decCfg);

    ElementCfg vppCfg;
    memset(&vppCfg, 0, sizeof(vppCfg));
    ReadVppConfig(vpp_cfg, &vppCfg);

    ElementCfg encCfg;
    memset(&encCfg, 0, sizeof(encCfg));
    ReadEncConfig(enc_cfg, &encCfg);

    int in_file_type = decCfg.DecParams.mfx.CodecId;
    int out_file_type = encCfg.EncParams.mfx.CodecId;

    main_session_ = GenMsdkSession();
    if (!main_session_) {
        printf("Init Session failed\n");
        DestroyVaDisp();
        return -1;
    }

    //decoder session and allocator
    MFXVideoSession *dec_session = NULL;
    GeneralAllocator *pAllocatorDec = NULL;
    CreateSessionAllocator(&dec_session, &pAllocatorDec);
    if (!dec_session || !pAllocatorDec) {
        CloseMsdkSession(main_session_);
        DestroyVaDisp();
        return -1;
    }

    //vpp session and allocator
    MFXVideoSession *vpp_session = NULL;
    GeneralAllocator *pAllocatorVpp = NULL;
    CreateSessionAllocator(&vpp_session, &pAllocatorVpp);
    if (!vpp_session || !pAllocatorVpp) {
        delete pAllocatorDec;
        CloseMsdkSession(dec_session);
        CloseMsdkSession(main_session_);
        DestroyVaDisp();
        return -1;
    }

    //encoder session and allocator
    MFXVideoSession *enc_session = NULL;
    GeneralAllocator *pAllocatorEnc = NULL;
    CreateSessionAllocator(&enc_session, &pAllocatorEnc);
    if (!enc_session || !pAllocatorEnc) {
        delete pAllocatorVpp;
        delete pAllocatorDec;
        CloseMsdkSession(vpp_session);
        CloseMsdkSession(dec_session);
        CloseMsdkSession(main_session_);
        DestroyVaDisp();
        return -1;
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
        delete pAllocatorEnc;
        delete pAllocatorVpp;
        delete pAllocatorDec;
        CloseMsdkSession(enc_session);
        CloseMsdkSession(vpp_session);
        CloseMsdkSession(dec_session);
        CloseMsdkSession(main_session_);
        DestroyVaDisp();
        return -1;
    }

    dec_dis_ = new Dispatcher;
    if (dec_dis_) {
        dec_dis_->Init(NULL, ELEMENT_MODE_PASSIVE);
    } else {
        delete pAllocatorEnc;
        delete pAllocatorVpp;
        delete pAllocatorDec;
        CloseMsdkSession(enc_session);
        CloseMsdkSession(vpp_session);
        CloseMsdkSession(dec_session);
        CloseMsdkSession(main_session_);
        DestroyVaDisp();
        return -1;
    }

    vpp_ = new MSDKCodec(ELEMENT_VPP, vpp_session, pAllocatorVpp);
    if (vpp_) {
        vpp_->Init(&vppCfg, ELEMENT_MODE_ACTIVE);
        vpp_->SetStrInfo(dec_->GetStrInfo());
        vpp_->SetPicInfo(dec_->GetPicInfo());
    } else {
        delete pAllocatorEnc;
        delete pAllocatorVpp;
        delete pAllocatorDec;
        CloseMsdkSession(enc_session);
        CloseMsdkSession(vpp_session);
        CloseMsdkSession(dec_session);
        CloseMsdkSession(main_session_);
        DestroyVaDisp();
        return -1;
    }

    vpp_dis_ = new Dispatcher;
    if (vpp_dis_) {
        vpp_dis_->Init(NULL, ELEMENT_MODE_PASSIVE);
    } else {
        delete pAllocatorEnc;
        delete pAllocatorVpp;
        delete pAllocatorDec;
        CloseMsdkSession(enc_session);
        CloseMsdkSession(vpp_session);
        CloseMsdkSession(dec_session);
        CloseMsdkSession(main_session_);
        DestroyVaDisp();
        return -1;
    }

    if (MFX_CODEC_VP8 == out_file_type) {
        enc_ = new MSDKCodec(ELEMENT_VP8_ENC, enc_session, pAllocatorEnc);
    } else {
        enc_ = new MSDKCodec(ELEMENT_ENCODER, enc_session, pAllocatorEnc);
    }
    if (enc_) {
        enc_->Init(&encCfg, ELEMENT_MODE_ACTIVE);
    } else {
        delete pAllocatorEnc;
        delete pAllocatorVpp;
        delete pAllocatorDec;
        CloseMsdkSession(enc_session);
        CloseMsdkSession(vpp_session);
        CloseMsdkSession(dec_session);
        CloseMsdkSession(main_session_);
        DestroyVaDisp();
        return -1;
    }

    dec_list_.push_back(dec_);
    dec_dis_map_[dec_] = dec_dis_;
    vpp_list_.push_back(vpp_);
    vpp_dis_map_[vpp_] = vpp_dis_;
    enc_multimap_.insert(std::make_pair(vpp_, enc_));

    main_session_->JoinSession(*dec_session);
    main_session_->JoinSession(*vpp_session);
    main_session_->JoinSession(*enc_session);
    m_pAllocArray.push_back(pAllocatorDec);
    m_pAllocArray.push_back(pAllocatorVpp);
    m_pAllocArray.push_back(pAllocatorEnc);

    dec_->LinkNextElement(dec_dis_);
    dec_dis_->LinkNextElement(vpp_);
    vpp_->LinkNextElement(vpp_dis_);
    vpp_dis_->LinkNextElement(enc_);

    dec_cfg->DecHandle = dec_;
    vpp_cfg->VppHandle = vpp_;
    enc_cfg->EncHandle = enc_;

    done_init_ = true;
    printf("[%s]Init Decoder:%p, VPP:%p, Encoder:%p Done.\n", \
        __FUNCTION__, dec_, vpp_, enc_);
    return 0;
}

int MsdkXcoder::ForceKeyFrame(CodecType ctype)
{
    Locker<Mutex> lock(mutex);
    if (!done_init_) {
        printf("Force key frame before initialization\n");
        return -1;
    }

    ElementType type = ELEMENT_ENCODER; //default value

    if (ctype == CODEC_TYPE_VIDEO_VP8) {
        type = ELEMENT_VP8_ENC;
    }

    MSDKCodec *enc = NULL;
    for(std::multimap<MSDKCodec*, MSDKCodec*>::iterator it_enc = enc_multimap_.begin(); \
            it_enc != enc_multimap_.end(); \
            ++it_enc) {
        if((it_enc->second)->GetCodecType() == type) {
            enc = it_enc->second;
            enc->SetForceKeyFrame();
        }
    }

    return 0;
}

int MsdkXcoder::SetBitrate(CodecType ctype, unsigned short bitrate)
{
    Locker<Mutex> lock(mutex);
    if (!done_init_) {
        printf("Force change bitrate before initialization\n");
        return -1;
    }

    ElementType type = ELEMENT_ENCODER; //default value

    if (ctype == CODEC_TYPE_VIDEO_VP8) {
        type = ELEMENT_VP8_ENC;
    }

    MSDKCodec *enc = NULL;
    for(std::multimap<MSDKCodec*, MSDKCodec*>::iterator it_enc = enc_multimap_.begin(); \
            it_enc != enc_multimap_.end(); \
            ++it_enc) {
        if((it_enc->second)->GetCodecType() == type) {
            enc = it_enc->second;
            enc->SetBitrate(bitrate);
            enc->SetResetBitrateFlag();
        }
    }

    return 0;
}

void MsdkXcoder::SetComboType(ComboType type, void* master)
{
    Locker<Mutex> lock(mutex);
    std::list<MSDKCodec*>::iterator vpp_it;
    vpp_it = vpp_list_.begin();
    for (; vpp_it != vpp_list_.end(); ++vpp_it) {
        if (*vpp_it && master) {
            (*vpp_it)->ConfigVppCombo(type, master);
        } else {
            printf("SetComboType is not available before init vpp\n");
        }
    }
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
        printf("[%s]Unlinking element ...\n", __FUNCTION__);
        dec->UnlinkNextElement();
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
        printf("[%s]Unlinking element ...\n", __FUNCTION__);
        dec_dis->UnlinkNextElement();
    } else {
        printf("[%s]Can't find the input dispatch handle", __FUNCTION__);
        return -1;
    }

    if (dec) {
        del_dec_list_.push_back(dec);
    }

    if (dec_dis) {
        delete dec_dis;
        dec_dis = NULL;
    }

    printf("[%s]Detach Decoder Done\n", __FUNCTION__);
    return 0;
}

void MsdkXcoder::AttachInput(DecOptions *dec_cfg, void *vppHandle)
{
    Locker<Mutex> lock(mutex);
    bool vpp_found = false;
    std::list<MSDKCodec*>::iterator vpp_it;

    if (!done_init_) {
        printf("[%s]Attach new stream before initialization\n", __FUNCTION__);
        return;
    }

    if (!dec_cfg) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return;
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
            return;
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
        return;
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
        delete pAllocatorDec;
        CloseMsdkSession(dec_session);
        return;
    }

    dec_dis_ = NULL;
    dec_dis_ = new Dispatcher;
    if (dec_dis_) {
        dec_dis_->Init(NULL, ELEMENT_MODE_PASSIVE);
    } else {
        printf("[%s]Create decoder dispatch failed\n", __FUNCTION__);
        delete pAllocatorDec;
        CloseMsdkSession(dec_session);
        return;
    }

    dec_list_.push_back(dec_);
    dec_dis_map_[dec_] = dec_dis_;

    main_session_->JoinSession(*dec_session);
    m_pAllocArray.push_back(pAllocatorDec);

    dec_->LinkNextElement(dec_dis_);
    if (vpp) {
        vpp->SetStrInfo(dec_->GetStrInfo());
        vpp->SetPicInfo(dec_->GetPicInfo());
        dec_dis_->LinkNextElement(vpp);
        if (is_running_ && !vpp->is_running_) {
            vpp->Start();
        }
    } else {
        for (vpp_it = vpp_list_.begin(); \
                vpp_it != vpp_list_.end(); \
                ++vpp_it) {
            if (*vpp_it) {
                (*vpp_it)->SetStrInfo(dec_->GetStrInfo());
                (*vpp_it)->SetPicInfo(dec_->GetPicInfo());
                dec_dis_->LinkNextElement(*vpp_it);
                if (is_running_ && !(*vpp_it)->is_running_) {
                    (*vpp_it)->Start();
                }
            }
        }
    }

    if (is_running_) {
        dec_dis_->Start();
        dec_->Start();
    }

    dec_cfg->DecHandle = dec_;
    printf("[%s]Attach Decoder:%p Done.\n", __FUNCTION__, dec_);
    return;
}

int MsdkXcoder::DetachVpp(void* vpp_handle)
{
    Locker<Mutex> lock(mutex);
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

    printf("[%s]Stopping vpp...\n", __FUNCTION__);
    vpp->Stop();
    printf("[%s]Unlinking element ...\n", __FUNCTION__);
    vpp->UnlinkPrevElement();
    vpp->UnlinkNextElement();

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
    printf("[%s]Unlinking element ...\n", __FUNCTION__);
    vpp_dis->UnlinkNextElement();

    BaseElement *enc = NULL;
    typedef std::multimap<MSDKCodec*, MSDKCodec*>::iterator enc_multimap_it;
    std::pair<enc_multimap_it,enc_multimap_it> p = enc_multimap_.equal_range(vpp);
    for(enc_multimap_it k = p.first; k != p.second; k++)  {
        enc = k->second;
        printf("[%s]Stopping encoder ...\n", __FUNCTION__);
        enc->Stop();
        printf("[%s]Deleting enc...\n", __FUNCTION__);
        delete enc;
        enc = NULL;
    }
    enc_multimap_.erase(vpp);

    delete vpp;
    vpp = NULL;
    delete vpp_dis;
    vpp_dis = NULL;

    printf("[%s]Detach VPP Done\n", __FUNCTION__);
    return 0;
}

void MsdkXcoder::AttachVpp(VppOptions *vpp_cfg, EncOptions *enc_cfg)
{
    Locker<Mutex> lock(mutex);

    if (!done_init_) {
        printf("[%s]Attach new vpp before initialization\n", __FUNCTION__);
        return;
    }

    if (!vpp_cfg || !enc_cfg) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return;
    }
    vpp_cfg->VppHandle = NULL;
    enc_cfg->EncHandle = NULL;

    ElementCfg vppCfg;
    memset(&vppCfg, 0, sizeof(vppCfg));
    ReadVppConfig(vpp_cfg, &vppCfg);

    ElementCfg encCfg;
    memset(&encCfg, 0, sizeof(encCfg));
    ReadEncConfig(enc_cfg, &encCfg);

    /*vpp allocator, session*/
    GeneralAllocator *pAllocatorVpp = NULL;
    MFXVideoSession *vpp_session = NULL;
    CreateSessionAllocator(&vpp_session, &pAllocatorVpp);
    if (!vpp_session || !pAllocatorVpp) {
        printf("[%s]Create vpp session or allocator failed\n", __FUNCTION__);
        return;
    }

    /*enc allocator, session*/
    GeneralAllocator *pAllocatorEnc = NULL;
    MFXVideoSession *enc_session = NULL;
    CreateSessionAllocator(&enc_session, &pAllocatorEnc);
    if (!enc_session || !pAllocatorEnc) {
        printf("[%s]Create encoder session or allocator failed\n", __FUNCTION__);
        delete pAllocatorVpp;
        CloseMsdkSession(vpp_session);
        return;
    }

    vpp_ = NULL;
    vpp_ = new MSDKCodec(ELEMENT_VPP, vpp_session, pAllocatorVpp);
    if (vpp_) {
        vpp_->Init(&vppCfg, ELEMENT_MODE_ACTIVE);
    } else {
        printf("[%s]Create vpp failed\n", __FUNCTION__);
        delete pAllocatorEnc;
        delete pAllocatorVpp;
        CloseMsdkSession(enc_session);
        CloseMsdkSession(vpp_session);
        return;
    }

    vpp_dis_ = NULL;
    vpp_dis_ = new Dispatcher;
    if (vpp_dis_) {
        vpp_dis_->Init(NULL, ELEMENT_MODE_PASSIVE);
    } else {
        printf("[%s]Create vpp dispatch failed\n", __FUNCTION__);
        delete pAllocatorEnc;
        delete pAllocatorVpp;
        CloseMsdkSession(enc_session);
        CloseMsdkSession(vpp_session);
        return;
    }

    int out_file_type = encCfg.EncParams.mfx.CodecId;
    enc_ = NULL;
    if (MFX_CODEC_VP8 == out_file_type) {
        enc_ = new MSDKCodec(ELEMENT_VP8_ENC, enc_session, pAllocatorEnc);
    } else {
        enc_ = new MSDKCodec(ELEMENT_ENCODER, enc_session, pAllocatorEnc);
    }
    if (enc_) {
        enc_->Init(&encCfg, ELEMENT_MODE_ACTIVE);
    } else {
        printf("[%s]Create encoder failed\n", __FUNCTION__);
        delete pAllocatorEnc;
        delete pAllocatorVpp;
        CloseMsdkSession(enc_session);
        CloseMsdkSession(vpp_session);
        return;
    }

    vpp_list_.push_back(vpp_);
    vpp_dis_map_[vpp_] = vpp_dis_;
    enc_multimap_.insert(std::make_pair(vpp_, enc_));

    main_session_->JoinSession(*vpp_session);
    m_pAllocArray.push_back(pAllocatorVpp);
    main_session_->JoinSession(*enc_session);
    m_pAllocArray.push_back(pAllocatorEnc);

    std::map<MSDKCodec*, Dispatcher *>::iterator dec_dis_i;
    for (dec_dis_i = dec_dis_map_.begin(); dec_dis_i != dec_dis_map_.end(); ++dec_dis_i) {
        (dec_dis_i->second)->LinkNextElement(vpp_);
    }
    vpp_->LinkNextElement(vpp_dis_);
    vpp_dis_->LinkNextElement(enc_);

    if (is_running_) {
        enc_->Start();
        vpp_dis_->Start();
        vpp_->Start();
    }

    vpp_cfg->VppHandle = vpp_;
    enc_cfg->EncHandle = enc_;
    printf("[%s]Attach VPP:%p, Encoder:%p Done.\n", __FUNCTION__, vpp_, enc_);
    return;
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
                printf("[%s]Unlinking element ...\n", __FUNCTION__);
                enc->UnlinkPrevElement();
                printf("[%s]Stopping encoder ...\n", __FUNCTION__);
                enc->Stop();
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

void MsdkXcoder::AttachOutput(EncOptions *enc_cfg, void *vppHandle)
{
    Locker<Mutex> lock(mutex);
    bool vpp_found = false;
    bool vpp_dis_found = false;
    std::list<MSDKCodec*>::iterator vpp_it;
    std::map<MSDKCodec*, Dispatcher*>::iterator vpp_dis_it;

    if (!done_init_) {
        printf("[%s]Attach new stream before initialization\n", __FUNCTION__);
        return;
    }

    if (!enc_cfg || !vppHandle) {
        printf("[%s]Invalid input parameters\n", __FUNCTION__);
        return;
    }
    enc_cfg->EncHandle = NULL;

    ElementCfg encCfg;
    memset(&encCfg, 0, sizeof(encCfg));
    ReadEncConfig(enc_cfg, &encCfg);

    MSDKCodec *vpp = static_cast<MSDKCodec*>(vppHandle);
    for (vpp_it = vpp_list_.begin(); vpp_it != vpp_list_.end(); ++vpp_it) {
        if (vpp == (*vpp_it)) {
            vpp_found = true;
            break;
        }
    }
    if (!vpp_found || !vpp) {
        printf("[%s]Can't find the vpp handle\n", __FUNCTION__);
        return;
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
        return;
    }

    GeneralAllocator *pAllocatorEnc = NULL;
    MFXVideoSession *enc_session = NULL;
    CreateSessionAllocator(&enc_session, &pAllocatorEnc);
    if (!enc_session || !pAllocatorEnc) {
        printf("[%s]Create encoder session or allocator failed\n", __FUNCTION__);
        return;
    }

    int out_file_type = encCfg.EncParams.mfx.CodecId;

    enc_ = NULL;
    if (MFX_CODEC_VP8 == out_file_type) {
        enc_ = new MSDKCodec(ELEMENT_VP8_ENC, enc_session, pAllocatorEnc);
    } else {
        enc_ = new MSDKCodec(ELEMENT_ENCODER, enc_session, pAllocatorEnc);
    }
    if (enc_) {
        enc_->Init(&encCfg, ELEMENT_MODE_ACTIVE);
    } else {
        printf("[%s]Create encoder failed\n", __FUNCTION__);
        delete pAllocatorEnc;
        CloseMsdkSession(enc_session);
        return;
    }

    enc_multimap_.insert(std::make_pair(vpp, enc_));

    main_session_->JoinSession(*enc_session);
    m_pAllocArray.push_back(pAllocatorEnc);

    vpp_dis->LinkNextElement(enc_);
    if (is_running_) {
        enc_->Start();
    }

    enc_cfg->EncHandle = enc_;
    printf("[%s]Attach Encoder:%p Done.\n", __FUNCTION__, enc_);
    return;
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
