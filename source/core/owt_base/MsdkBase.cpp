// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_MSDK

#include "MsdkBase.h"

#include <fcntl.h>
#include <va/va.h>
#include <va/va_drm.h>
#include <vaapi_allocator.h>

namespace owt_base {

DEFINE_LOGGER(MsdkBase, "owt.MsdkBase");

boost::shared_mutex MsdkBase::sSingletonLock;
MsdkBase *MsdkBase::sSingleton = NULL;
std::vector<mfxU32> MsdkBase::sSupportedDecoders;
std::vector<mfxU32> MsdkBase::sSupportedEncoders;

static bool AreGuidsEqual(const mfxPluginUID *guid1, const mfxPluginUID *guid2)
{
    for(size_t i = 0; i != sizeof(mfxPluginUID); i++) {
        if (guid1->Data[i] != guid2->Data[i])
            return false;
    }
    return true;
}

static bool isValidPluginUID(const mfxPluginUID *uid)
{
    return (AreGuidsEqual(uid, &MFX_PLUGINID_HEVCD_HW)
            ||AreGuidsEqual(uid, &MFX_PLUGINID_HEVCE_HW)
            ||AreGuidsEqual(uid, &MFX_PLUGINID_HEVCE_GACC)
           );
}

MsdkBase::MsdkBase()
    : m_fd(-1)
    , m_vaDisp(NULL)
    , m_mainSession(NULL)
    , m_configHevcEncoderGaccPlugin(false)
    , m_configMFETimeout(0)
{
}

bool MsdkBase::init()
{
    static const char *drm_device_paths[] = {
        "/dev/dri/renderD128",
        "/dev/dri/card0",
        NULL
    };
    int major_version, minor_version;
    int ret;

    for (int i = 0; drm_device_paths[i]; i++) {
        m_fd = open(drm_device_paths[i], O_RDWR);
        if (m_fd < 0)
            continue;

        m_vaDisp = vaGetDisplayDRM(m_fd);
        if(!m_vaDisp) {
            close(m_fd);
            m_fd = -1;

            continue;
        }

        ELOG_DEBUG("Open drm device: %s", drm_device_paths[i]);
        break;
    }

    if(!m_vaDisp) {
        ELOG_ERROR("Get VA display failed.");
        return false;
    }

    ret = vaInitialize(m_vaDisp, &major_version, &minor_version);
    if (ret != VA_STATUS_SUCCESS) {
        ELOG_ERROR("Init VA failed, ret %d\n", ret);

        m_vaDisp = NULL;
        close(m_fd);
        m_fd = -1;
        return false;
    }

    ELOG_INFO("VA-API Version: %d.%d", VA_MAJOR_VERSION, VA_MINOR_VERSION);
    if (VA_MAJOR_VERSION != major_version || VA_MINOR_VERSION != minor_version)
        ELOG_WARN("VA-API Runtime Version: %d.%d", major_version, minor_version);

    m_mainSession = createSession_internal();
    if (!m_mainSession) {
        ELOG_ERROR("Create main session failed.");

        m_vaDisp = NULL;
        close(m_fd);
        m_fd = -1;
        return false;
    }

    ELOG_INFO("MFX Version: %d, major(%d), minor(%d)", MFX_VERSION, MFX_VERSION_MAJOR, MFX_VERSION_MINOR);

    mfxVersion ver;
    ret = m_mainSession->QueryVersion(&ver);
    if (ret != MFX_ERR_NONE) {
        ELOG_WARN("QueryVersion failed.");
    } else {
        if (MFX_VERSION_MAJOR != ver.Major || MFX_VERSION_MINOR != ver.Minor)
            ELOG_WARN("MFX Runtime Version: %d, major(%d), minor(%d)", ver.Major * 1000 + ver.Minor, ver.Major, ver.Minor);
    }

    mfxStatus sts = MFX_ERR_NONE;
    mfxVideoParam videoParam;
    mfxPluginUID pluginID;
    std::vector<mfxU32> codecs;

    // supported decoders
    MFXVideoDECODE *dec = new MFXVideoDECODE(*m_mainSession);
    if (dec) {
        codecs.clear();
        codecs.push_back(MFX_CODEC_AVC);
        codecs.push_back(MFX_CODEC_HEVC);

        //disable VPx codec
        //codecs.push_back(MFX_CODEC_VP8);
        //codecs.push_back(MFX_CODEC_VP9);

        for (auto &codec : codecs) {
            memset(&videoParam, 0, sizeof(videoParam));
            videoParam.mfx.CodecId = codec;

            loadDecoderPlugin(videoParam.mfx.CodecId, m_mainSession, &pluginID);
            sts = dec->Query(NULL, &videoParam);
            unLoadPlugin(m_mainSession, &pluginID);

            if (sts == MFX_ERR_NONE) {
                ELOG_INFO("Supported decoder: %c%c%c%c"
                        , codec & 0xff
                        , (codec >> 8) & 0xff
                        , (codec >> 16) & 0xff
                        , (codec >> 24) & 0xff
                        );
               sSupportedDecoders.push_back(codec);
            }
        }
        dec->Close();
        delete dec;
    }

    // supported encoders
    MFXVideoENCODE *enc = new MFXVideoENCODE(*m_mainSession);
    if (enc) {
        codecs.clear();
        codecs.push_back(MFX_CODEC_AVC);
        codecs.push_back(MFX_CODEC_HEVC);

        //disable VPx codec
        //codecs.push_back(MFX_CODEC_VP8);
        //codecs.push_back(MFX_CODEC_VP9);

        for (auto &codec : codecs) {
            memset(&videoParam, 0, sizeof(videoParam));
            videoParam.mfx.CodecId = codec;

            loadEncoderPlugin(videoParam.mfx.CodecId, m_mainSession, &pluginID);
            sts = enc->Query(NULL, &videoParam);
            unLoadPlugin(m_mainSession, &pluginID);

            if (sts == MFX_ERR_NONE) {
                ELOG_INFO("Supported encoder: %c%c%c%c"
                        , codec & 0xff
                        , (codec >> 8) & 0xff
                        , (codec >> 16) & 0xff
                        , (codec >> 24) & 0xff
                        );
               sSupportedEncoders.push_back(codec);
            }
        }
        enc->Close();
        delete enc;
    }

    return true;
}

MsdkBase::~MsdkBase()
{
}

MsdkBase *MsdkBase::get(void)
{
    boost::upgrade_lock<boost::shared_mutex> lock(sSingletonLock);

    if (sSingleton == NULL) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);

        ELOG_DEBUG("Instantiating Singleton.");
        sSingleton = new MsdkBase();
        if(!sSingleton->init()) {
            ELOG_ERROR("Init Singleton failed.");

            delete sSingleton;
            sSingleton = NULL;
        }
    }

    return sSingleton;
}

bool MsdkBase::isSupportedDecoder(mfxU32 codecId)
{
    for (auto &c : sSupportedDecoders)
        if (codecId == c)
            return true;

    return false;
}

bool MsdkBase::isSupportedEncoder(mfxU32 codecId)
{
    for (auto &c : sSupportedEncoders)
        if (codecId == c)
            return true;

    return false;
}

void MsdkBase::setConfigHevcEncoderGaccPlugin(bool hevcEncoderGaccPlugin)
{
    ELOG_DEBUG("Set hevcEncoderGaccPlugin(%d)", hevcEncoderGaccPlugin);
    m_configHevcEncoderGaccPlugin = hevcEncoderGaccPlugin;
}

bool MsdkBase::getConfigHevcEncoderGaccPlugin()
{
    return m_configHevcEncoderGaccPlugin;
}

void MsdkBase::setConfigMFETimeout(uint32_t MFETimeout)
{
    ELOG_DEBUG("Set MFE Timeout(%d)", MFETimeout);
    m_configMFETimeout = MFETimeout < 100 ? MFETimeout : 100;

    if (m_configMFETimeout != MFETimeout)
        ELOG_DEBUG("Constrain MFE Timeout to %d", m_configMFETimeout);
}

uint32_t MsdkBase::getConfigMFETimeout()
{
    return m_configMFETimeout;
}

MFXVideoSession *MsdkBase::createSession_internal(void)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxIMPL impl = MFX_IMPL_HARDWARE_ANY;
    mfxVersion ver = {{3, 1}};

    MFXVideoSession * pSession = new MFXVideoSession;
    if (!pSession) {
        ELOG_ERROR("Create session failed.");

        return NULL;
    }

    sts = pSession->Init(impl, &ver);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Init session failed.");

        delete pSession;
        return NULL;
    }

    sts = pSession->SetHandle((mfxHandleType)MFX_HANDLE_VA_DISPLAY, m_vaDisp);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("SetHandle failed.");

        delete pSession;
        return NULL;
    }

    return pSession;
}

MFXVideoSession *MsdkBase::createSession()
{
    mfxStatus sts = MFX_ERR_NONE;
    MFXVideoSession *pSession = NULL;

    pSession = createSession_internal();
    if (!pSession) {
        return NULL;
    }

    sts = m_mainSession->JoinSession(*pSession);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Join main session failed");
        return NULL;
    }

    return pSession;
}

void MsdkBase::destroySession(MFXVideoSession *pSession)
{
    if (pSession) {
        pSession->DisjoinSession();
        pSession->Close();
        delete pSession;
    }
}

bool MsdkBase::loadDecoderPlugin(uint32_t codecId, MFXVideoSession *pSession, mfxPluginUID *pluginID)
{
    mfxStatus sts = MFX_ERR_NONE;

    switch (codecId) {
        case MFX_CODEC_HEVC:
            ELOG_DEBUG("Load plugin MFX_PLUGINID_HEVCD_HW");
            sts = MFXVideoUSER_Load(*pSession, &MFX_PLUGINID_HEVCD_HW, 1);
            if (sts != MFX_ERR_NONE) {
                ELOG_ERROR("Failed to load codec plugin.");
                return false;
            }

            memcpy(pluginID, &MFX_PLUGINID_HEVCD_HW, sizeof(mfxPluginUID));
            return true;

        case MFX_CODEC_AVC:
            return true;

        case MFX_CODEC_VP8:
            ELOG_DEBUG("Load plugin MFX_PLUGINID_VP8D_HW");
            sts = MFXVideoUSER_Load(*pSession, &MFX_PLUGINID_VP8D_HW, 1);
            if (sts != MFX_ERR_NONE) {
                ELOG_ERROR("Failed to load codec plugin.");
                return false;
            }

            memcpy(pluginID, &MFX_PLUGINID_VP8D_HW, sizeof(mfxPluginUID));
            return true;

        default:
            return false;
    }

    return true;
}

bool MsdkBase::loadEncoderPlugin(uint32_t codecId, MFXVideoSession *pSession, mfxPluginUID *pluginID)
{
    mfxStatus sts = MFX_ERR_NONE;

    switch (codecId) {
        case MFX_CODEC_HEVC:
            mfxPluginUID id;

            if (m_configHevcEncoderGaccPlugin) {
                ELOG_DEBUG("Load plugin MFX_PLUGINID_HEVCE_GACC");
                memcpy(&id, &MFX_PLUGINID_HEVCE_GACC, sizeof(mfxPluginUID));
            } else {
                ELOG_DEBUG("Load plugin MFX_PLUGINID_HEVCE_HW");
                memcpy(&id, &MFX_PLUGINID_HEVCE_HW, sizeof(mfxPluginUID));
            }

            sts = MFXVideoUSER_Load(*pSession, &id, 1);
            if (sts != MFX_ERR_NONE) {
            // If fail to load requested GACC plugin, we fallback to HW
                if (AreGuidsEqual(pluginID, &MFX_PLUGINID_HEVCE_GACC)) {
                    memcpy(&id, &MFX_PLUGINID_HEVCE_HW, sizeof(mfxPluginUID));
                    sts = MFXVideoUSER_Load(*pSession, &id, 1);
                    if (sts != MFX_ERR_NONE) {
                       ELOG_ERROR("Failed to load alternative codec plugin.");
                       return false;
                    }
                } else {
                    ELOG_ERROR("Failed to load codec plugin.");
                    return false;
                }
            }

            memcpy(pluginID, &id, sizeof(mfxPluginUID));
            return true;

        case MFX_CODEC_AVC:
            return true;

        default:
            return false;
    }

    return true;
}

void MsdkBase::unLoadPlugin(MFXVideoSession *pSession, mfxPluginUID *pluginID)
{
    if (isValidPluginUID(pluginID)) {
        ELOG_DEBUG("UnLoad plugin");
        MFXVideoUSER_UnLoad(*pSession, pluginID);
    }
}

boost::shared_ptr<mfxFrameAllocator> MsdkBase::createFrameAllocator(void)
{
    mfxStatus sts = MFX_ERR_NONE;
    vaapiFrameAllocator *pAlloc = NULL;
    struct vaapiAllocatorParams p;

    pAlloc = new vaapiFrameAllocator();
    p.m_dpy = m_vaDisp;

    sts = pAlloc->Init(&p);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Init frame allocator failed");
        return NULL;
    }

    return boost::shared_ptr<mfxFrameAllocator>(pAlloc);
}

void MsdkBase::destroyFrameAllocator(mfxFrameAllocator *pAlloc)
{
    delete pAlloc;
}

void MsdkBase::printfFrameInfo(mfxFrameInfo *pFrameInfo)
{
    if (!ELOG_IS_DEBUG_ENABLED())
        return;

    printf("%s++++++++++\n", __FUNCTION__);

    printf("\t BitDepthLuma %d\n",             (int)pFrameInfo->BitDepthLuma);
    printf("\t BitDepthChroma %d\n",           (int)pFrameInfo->BitDepthChroma);
    printf("\t Shift %d\n",                    (int)pFrameInfo->Shift);
    printf("\t FourCC 0x%x(%c%c%c%c)\n",       (int)pFrameInfo->FourCC,
            pFrameInfo->FourCC & 0xff,
            (pFrameInfo->FourCC >> 8) & 0xff,
            (pFrameInfo->FourCC >> 16) & 0xff,
            (pFrameInfo->FourCC >> 24) & 0xff
            );
    printf("\t Width %d\n",                    (int)pFrameInfo->Width);
    printf("\t Height %d\n",                   (int)pFrameInfo->Height);
    printf("\t CropX %d\n",                    (int)pFrameInfo->CropX);
    printf("\t CropY %d\n",                    (int)pFrameInfo->CropY);
    printf("\t CropW %d\n",                    (int)pFrameInfo->CropW);
    printf("\t CropH %d\n",                    (int)pFrameInfo->CropH);
    printf("\t FrameRateExtN %d\n",            (int)pFrameInfo->FrameRateExtN);
    printf("\t FrameRateExtD %d\n",            (int)pFrameInfo->FrameRateExtD);
    printf("\t AspectRatioW %d\n",             (int)pFrameInfo->AspectRatioW);
    printf("\t AspectRatioH %d\n",             (int)pFrameInfo->AspectRatioH);
    printf("\t PicStruct %d\n",                (int)pFrameInfo->PicStruct);
    printf("\t ChromaFormat %d\n",             (int)pFrameInfo->ChromaFormat);

    printf("%s----------\n", __FUNCTION__);
}

void MsdkBase::printfVideoParam(mfxVideoParam *pVideoParam, DumpType type)
{
    if (!ELOG_IS_DEBUG_ENABLED())
        return;

    printf("%s - %s++++++++++\n", __FUNCTION__, type == MFX_DEC ? "DECODE" : (type == MFX_VPP) ? "VPP" : "ENCODE");

    printf("\t AsyncDepth %d\n",               (int)pVideoParam->AsyncDepth);
    printf("\t Protected %d\n",                (int)pVideoParam->Protected);
    printf("\t IOPattern %d\n",                (int)pVideoParam->IOPattern);

    if (type == MFX_DEC || type == MFX_ENC) {

        printf("\t mfx LowPower %d\n",             (int)pVideoParam->mfx.LowPower);
        printf("\t mfx BRCParamMultiplier %d\n",   (int)pVideoParam->mfx.BRCParamMultiplier);
        printf("\t mfx CodecId 0x%x(%c%c%c%c)\n",  (int)pVideoParam->mfx.CodecId,
                pVideoParam->mfx.CodecId & 0xff,
                (pVideoParam->mfx.CodecId >> 8) & 0xff,
                (pVideoParam->mfx.CodecId >> 16) & 0xff,
                (pVideoParam->mfx.CodecId >> 24) & 0xff
                );
        printf("\t mfx CodecProfile %d\n",         (int)pVideoParam->mfx.CodecProfile);
        printf("\t mfx CodecLevel %d\n",           (int)pVideoParam->mfx.CodecLevel);
        printf("\t mfx NumThread %d\n",            (int)pVideoParam->mfx.NumThread);

        printfFrameInfo(&pVideoParam->mfx.FrameInfo);

        if (type == MFX_DEC) {
            printf("\t Decoding Options++++++++++\n");

            printf("\t\t DecodedOrder %d\n",             (int)pVideoParam->mfx.DecodedOrder);
            printf("\t\t ExtendedPicStruct %d\n",        (int)pVideoParam->mfx.ExtendedPicStruct);
            printf("\t\t TimeStampCalc %d\n",            (int)pVideoParam->mfx.TimeStampCalc);
            printf("\t\t SliceGroupsPresent %d\n",       (int)pVideoParam->mfx.SliceGroupsPresent);
            printf("\t\t MaxDecFrameBuffering %d\n",     (int)pVideoParam->mfx.MaxDecFrameBuffering);

            printf("\t Decoding Options----------\n");
        }
        else {
            printf("\t Encoding Options++++++++++\n");

            printf("\t TargetUsage %d\n",                     (int)pVideoParam->mfx.TargetUsage);
            printf("\t GopPicSize %d\n",                      (int)pVideoParam->mfx.GopPicSize);
            printf("\t GopRefDist %d\n",                      (int)pVideoParam->mfx.GopRefDist);
            printf("\t GopOptFlag %d\n",                      (int)pVideoParam->mfx.GopOptFlag);
            printf("\t IdrInterval %d\n",                     (int)pVideoParam->mfx.IdrInterval);

            printf("\t RateControlMethod %d\n",               (int)pVideoParam->mfx.RateControlMethod);
            printf("\t InitialDelayInKB-QPI-Accuracy %d\n",   (int)pVideoParam->mfx.InitialDelayInKB);

            printf("\t BufferSizeInKB %d\n",                  (int)pVideoParam->mfx.BufferSizeInKB);
            printf("\t TargetKbps-QPP-ICQQuality %d\n",       (int)pVideoParam->mfx.TargetKbps);
            printf("\t MaxKbps-QPB-Convergence %d\n",         (int)pVideoParam->mfx.MaxKbps);

            printf("\t NumSlice %d\n",                        (int)pVideoParam->mfx.NumSlice);
            printf("\t NumRefFrame %d\n",                     (int)pVideoParam->mfx.NumRefFrame);
            printf("\t EncodedOrder %d\n",                    (int)pVideoParam->mfx.EncodedOrder);

            printf("\t Encoding Options----------\n");
        }
    }
    else if (type == MFX_VPP) {
        printf("\t Vpp Options++++++++++\n");

        printf("\t\t In++++++++++\n");
        printfFrameInfo(&pVideoParam->vpp.In);
        printf("\t\t In----------\n");

        printf("\t\t Out++++++++++\n");
        printfFrameInfo(&pVideoParam->vpp.Out);
        printf("\t\t Out++++++++++\n");

        printf("\t Vpp Options----------\n");
    }
    else
    {
        printf("Invalid dump type, %d\n", type);
    }

    printf("\t NumExtParam %d\n",              (int)pVideoParam->NumExtParam);
    for(int i = 0; i < pVideoParam->NumExtParam; i++) {
        printf("\t\t %d - mfxExtBuffer: 0x%x(%c%c%c%c)\n", i, pVideoParam->ExtParam[i]->BufferId,
                pVideoParam->ExtParam[i]->BufferId & 0xff,
                (pVideoParam->ExtParam[i]->BufferId >> 8) & 0xff,
                (pVideoParam->ExtParam[i]->BufferId >> 16) & 0xff,
                (pVideoParam->ExtParam[i]->BufferId >> 24) & 0xff
              );

        switch (pVideoParam->ExtParam[i]->BufferId) {
            case MFX_EXTBUFF_VPP_COMPOSITE:
                {
                    mfxExtVPPComposite *pComposite = (mfxExtVPPComposite *)pVideoParam->ExtParam[i];

                    printf("\t\t\t bg Y 0x%x\n",              (int)pComposite->Y);
                    printf("\t\t\t bg U 0x%x\n",              (int)pComposite->U);
                    printf("\t\t\t bg V 0x%x\n",              (int)pComposite->V);

                    printf("\t\t\t NumInputStream %d\n",      (int)pComposite->NumInputStream);
                    for (int i = 0; i < pComposite->NumInputStream; i++) {
                        printf("\t\t\t\t %d - DstX %d\n",      i, (int)pComposite->InputStream[i].DstX);
                        printf("\t\t\t\t %d - DstY %d\n",      i, (int)pComposite->InputStream[i].DstY);
                        printf("\t\t\t\t %d - DstW %d\n",      i, (int)pComposite->InputStream[i].DstW);
                        printf("\t\t\t\t %d - DstH %d\n",      i, (int)pComposite->InputStream[i].DstH);
                    }

                }
                break;

            case MFX_EXTBUFF_CODING_OPTION:
                {
                    mfxExtCodingOption *pCodingOption = (mfxExtCodingOption *)pVideoParam->ExtParam[i];

                    printf("\t\t\t RateDistortionOpt %d\n", (int)pCodingOption->RateDistortionOpt);
                    printf("\t\t\t MECostType %d\n", (int)pCodingOption->MECostType);
                    printf("\t\t\t MESearchType %d\n", (int)pCodingOption->MESearchType);
                    printf("\t\t\t MVSearchWindow x(%d), y(%d)\n", (int)pCodingOption->MVSearchWindow.x, (int)pCodingOption->MVSearchWindow.y);
                    printf("\t\t\t EndOfSequence %d\n", (int)pCodingOption->EndOfSequence);
                    printf("\t\t\t FramePicture %d\n", (int)pCodingOption->FramePicture);

                    printf("\t\t\t CAVLC %d\n", (int)pCodingOption->CAVLC);
                    printf("\t\t\t RecoveryPointSEI %d\n", (int)pCodingOption->RecoveryPointSEI);
                    printf("\t\t\t ViewOutput %d\n", (int)pCodingOption->ViewOutput);
                    printf("\t\t\t NalHrdConformance %d\n", (int)pCodingOption->NalHrdConformance);
                    printf("\t\t\t SingleSeiNalUnit %d\n", (int)pCodingOption->SingleSeiNalUnit);
                    printf("\t\t\t VuiVclHrdParameters %d\n", (int)pCodingOption->VuiVclHrdParameters);

                    printf("\t\t\t RefPicListReordering %d\n", (int)pCodingOption->RefPicListReordering);
                    printf("\t\t\t ResetRefList %d\n", (int)pCodingOption->ResetRefList);
                    printf("\t\t\t RefPicMarkRep %d\n", (int)pCodingOption->RefPicMarkRep);
                    printf("\t\t\t FieldOutput %d\n", (int)pCodingOption->FieldOutput);

                    printf("\t\t\t IntraPredBlockSize %d\n", (int)pCodingOption->IntraPredBlockSize);
                    printf("\t\t\t InterPredBlockSize %d\n", (int)pCodingOption->InterPredBlockSize);
                    printf("\t\t\t MVPrecision %d\n", (int)pCodingOption->MVPrecision);
                    printf("\t\t\t MaxDecFrameBuffering %d\n", (int)pCodingOption->MaxDecFrameBuffering);

                    printf("\t\t\t AUDelimiter %d\n", (int)pCodingOption->AUDelimiter);
                    printf("\t\t\t EndOfStream %d\n", (int)pCodingOption->EndOfStream);
                    printf("\t\t\t PicTimingSEI %d\n", (int)pCodingOption->PicTimingSEI);
                    printf("\t\t\t VuiNalHrdParameters %d\n", (int)pCodingOption->VuiNalHrdParameters);
                }
                break;

            case MFX_EXTBUFF_CODING_OPTION2:
                {
                    mfxExtCodingOption2 *pCodingOption2 = (mfxExtCodingOption2 *)pVideoParam->ExtParam[i];

                    printf("\t\t\t IntRefType %d\n", (int)pCodingOption2->IntRefType);
                    printf("\t\t\t IntRefCycleSize %d\n", (int)pCodingOption2->IntRefCycleSize);
                    printf("\t\t\t IntRefQPDelta %d\n", (int)pCodingOption2->IntRefQPDelta);

                    printf("\t\t\t MaxFrameSize %d\n", (int)pCodingOption2->MaxFrameSize);
                    printf("\t\t\t MaxSliceSize %d\n", (int)pCodingOption2->MaxSliceSize);

                    printf("\t\t\t BitrateLimit %d\n", (int)pCodingOption2->BitrateLimit);
                    printf("\t\t\t MBBRC %d\n", (int)pCodingOption2->MBBRC);
                    printf("\t\t\t ExtBRC %d\n", (int)pCodingOption2->ExtBRC);
                    printf("\t\t\t LookAheadDepth %d\n", (int)pCodingOption2->LookAheadDepth);
                    printf("\t\t\t Trellis %d\n", (int)pCodingOption2->Trellis);
                    printf("\t\t\t RepeatPPS %d\n", (int)pCodingOption2->RepeatPPS);
                    printf("\t\t\t BRefType %d\n", (int)pCodingOption2->BRefType);
                    printf("\t\t\t AdaptiveI %d\n", (int)pCodingOption2->AdaptiveI);
                    printf("\t\t\t AdaptiveB %d\n", (int)pCodingOption2->AdaptiveB);
                    printf("\t\t\t LookAheadDS %d\n", (int)pCodingOption2->LookAheadDS);
                    printf("\t\t\t NumMbPerSlice %d\n", (int)pCodingOption2->NumMbPerSlice);
                    printf("\t\t\t SkipFrame %d\n", (int)pCodingOption2->SkipFrame);
                    printf("\t\t\t MinQPI %d\n", (int)pCodingOption2->MinQPI);
                    printf("\t\t\t MaxQPI %d\n", (int)pCodingOption2->MaxQPI);
                    printf("\t\t\t MinQPP %d\n", (int)pCodingOption2->MinQPP);
                    printf("\t\t\t MaxQPP %d\n", (int)pCodingOption2->MaxQPP);
                    printf("\t\t\t MinQPB %d\n", (int)pCodingOption2->MinQPB);
                    printf("\t\t\t MaxQPB %d\n", (int)pCodingOption2->MaxQPB);
                    printf("\t\t\t FixedFrameRate %d\n", (int)pCodingOption2->FixedFrameRate);
                    printf("\t\t\t DisableDeblockingIdc %d\n", (int)pCodingOption2->DisableDeblockingIdc);
                    printf("\t\t\t DisableVUI %d\n", (int)pCodingOption2->DisableVUI);
                    printf("\t\t\t BufferingPeriodSEI %d\n", (int)pCodingOption2->BufferingPeriodSEI);
                    printf("\t\t\t EnableMAD %d\n", (int)pCodingOption2->EnableMAD);
                    printf("\t\t\t UseRawRef %d\n", (int)pCodingOption2->UseRawRef);
                }
                break;

            case MFX_EXTBUFF_CODING_OPTION3:
                {
                    mfxExtCodingOption3 *pCodingOption3 = (mfxExtCodingOption3 *)pVideoParam->ExtParam[i];

#if (MFX_VERSION >= 1026)
                    printf("\t\t\t ExtBrcAdaptiveLTR %d\n", (int)pCodingOption3->ExtBrcAdaptiveLTR);
#endif
                }
                break;

#if (MFX_VERSION >= 1025)
            case MFX_EXTBUFF_MULTI_FRAME_PARAM:
                {
                    mfxExtMultiFrameParam *pMultiFrameParam = (mfxExtMultiFrameParam *)pVideoParam->ExtParam[i];

                    printf("\t\t\t MFMode %d\n", (int)pMultiFrameParam->MFMode);
                    printf("\t\t\t MaxNumFrames %d\n", (int)pMultiFrameParam->MaxNumFrames);
                }
                break;

            case MFX_EXTBUFF_MULTI_FRAME_CONTROL:
                {
                    mfxExtMultiFrameControl *pMultiFrameControl = (mfxExtMultiFrameControl *)pVideoParam->ExtParam[i];

                    printf("\t\t\t Timeout %d\n", (int)pMultiFrameControl->Timeout);
                    printf("\t\t\t Flush %d\n", (int)pMultiFrameControl->Flush);
                }
                break;
#endif

            default:
                break;
        }
    }

    printf("%s - %s----------\n", __FUNCTION__, type == MFX_DEC ? "DECODE" : (type == MFX_VPP) ? "VPP" : "ENCODE");
}

void MsdkBase::printfFrameAllocRequest(mfxFrameAllocRequest *pRequest)
{
    if (!ELOG_IS_DEBUG_ENABLED())
        return;

    printf("%s++++++++++\n", __FUNCTION__);

    printf("\t AllocId %d\n",               (int)pRequest->AllocId);

    printfFrameInfo(&pRequest->Info);

    printf("\t Type 0x%x\n",                (int)pRequest->Type);
    printf("\t NumFrameMin %d\n",           (int)pRequest->NumFrameMin);
    printf("\t NumFrameSuggested %d\n",     (int)pRequest->NumFrameSuggested);

    printf("%s----------\n", __FUNCTION__);
}

}//namespace owt_base

#endif /* ENABLE_MSDK */
