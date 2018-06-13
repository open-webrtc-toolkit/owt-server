/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#ifdef ENABLE_MSDK

#include "MsdkBase.h"

#include <fcntl.h>
#include <va/va.h>
#include <va/va_drm.h>
#include <vaapi_allocator.h>

namespace woogeen_base {

DEFINE_LOGGER(MsdkBase, "woogeen.MsdkBase");

boost::shared_mutex MsdkBase::sSingletonLock;
MsdkBase *MsdkBase::sSingleton = NULL;

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

    ELOG_INFO("VA-API version: %d.%d", major_version, minor_version);

    m_mainSession = createSession_internal();
    if (!m_mainSession) {
        ELOG_ERROR("Create main session failed.");

        m_vaDisp = NULL;
        close(m_fd);
        m_fd = -1;
        return false;
    }

    mfxVersion ver;
    ret = m_mainSession->QueryVersion(&ver);
    if (ret != MFX_ERR_NONE) {
        ELOG_WARN("QueryVersion failed.");
    } else {
        ELOG_INFO("Msdk version: %d.%d(%d)", ver.Major, ver.Minor, ver.Version);
    }

    return true;
}

MsdkBase::~MsdkBase()
{
    printfFuncEnter;
    printfFuncExit;
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

void MsdkBase::setConfigHevcEncoderGaccPlugin(bool hevcEncoderGaccPlugin)
{
    ELOG_DEBUG("Set hevcEncoderGaccPlugin(%d)", hevcEncoderGaccPlugin);
    m_configHevcEncoderGaccPlugin = hevcEncoderGaccPlugin;
}

bool MsdkBase::getConfigHevcEncoderGaccPlugin()
{
    return m_configHevcEncoderGaccPlugin;
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
                ELOG_ERROR("Failed to load codec plugin.");
                return false;
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

    printf("NumExtParam %d\n",              (int)pVideoParam->NumExtParam);
    for(int i = 0; i < pVideoParam->NumExtParam; i++) {
        printf("\t %d - mfxExtBuffer: 0x%x(%c%c%c%c)\n", i, pVideoParam->ExtParam[i]->BufferId,
                pVideoParam->ExtParam[i]->BufferId & 0xff,
                (pVideoParam->ExtParam[i]->BufferId >> 8) & 0xff,
                (pVideoParam->ExtParam[i]->BufferId >> 16) & 0xff,
                (pVideoParam->ExtParam[i]->BufferId >> 24) & 0xff
              );

        switch (pVideoParam->ExtParam[i]->BufferId) {
            case MFX_EXTBUFF_VPP_COMPOSITE:
                {
                    mfxExtVPPComposite *pComposite = (mfxExtVPPComposite *)pVideoParam->ExtParam[i];

                    printf("\t\t bg Y 0x%x\n",              (int)pComposite->Y);
                    printf("\t\t bg U 0x%x\n",              (int)pComposite->U);
                    printf("\t\t bg V 0x%x\n",              (int)pComposite->V);

                    printf("\t\t NumInputStream %d\n",      (int)pComposite->NumInputStream);
                    for (int i = 0; i < pComposite->NumInputStream; i++) {
                        printf("\t\t\t %d - DstX %d\n",      i, (int)pComposite->InputStream[i].DstX);
                        printf("\t\t\t %d - DstY %d\n",      i, (int)pComposite->InputStream[i].DstY);
                        printf("\t\t\t %d - DstW %d\n",      i, (int)pComposite->InputStream[i].DstW);
                        printf("\t\t\t %d - DstH %d\n",      i, (int)pComposite->InputStream[i].DstH);
                    }

                }
                break;

            case MFX_EXTBUFF_CODING_OPTION:
                {
                    mfxExtCodingOption *pCodingOption = (mfxExtCodingOption *)pVideoParam->ExtParam[i];

                    printf("\t\t RateDistortionOpt %d\n", (int)pCodingOption->RateDistortionOpt);
                    printf("\t\t MECostType %d\n", (int)pCodingOption->MECostType);
                    printf("\t\t MESearchType %d\n", (int)pCodingOption->MESearchType);
                    printf("\t\t MVSearchWindow x(%d), y(%d)\n", (int)pCodingOption->MVSearchWindow.x, (int)pCodingOption->MVSearchWindow.y);
                    printf("\t\t EndOfSequence %d\n", (int)pCodingOption->EndOfSequence);
                    printf("\t\t FramePicture %d\n", (int)pCodingOption->FramePicture);

                    printf("\t\t CAVLC %d\n", (int)pCodingOption->CAVLC);
                    printf("\t\t RecoveryPointSEI %d\n", (int)pCodingOption->RecoveryPointSEI);
                    printf("\t\t ViewOutput %d\n", (int)pCodingOption->ViewOutput);
                    printf("\t\t NalHrdConformance %d\n", (int)pCodingOption->NalHrdConformance);
                    printf("\t\t SingleSeiNalUnit %d\n", (int)pCodingOption->SingleSeiNalUnit);
                    printf("\t\t VuiVclHrdParameters %d\n", (int)pCodingOption->VuiVclHrdParameters);

                    printf("\t\t RefPicListReordering %d\n", (int)pCodingOption->RefPicListReordering);
                    printf("\t\t ResetRefList %d\n", (int)pCodingOption->ResetRefList);
                    printf("\t\t RefPicMarkRep %d\n", (int)pCodingOption->RefPicMarkRep);
                    printf("\t\t FieldOutput %d\n", (int)pCodingOption->FieldOutput);

                    printf("\t\t IntraPredBlockSize %d\n", (int)pCodingOption->IntraPredBlockSize);
                    printf("\t\t InterPredBlockSize %d\n", (int)pCodingOption->InterPredBlockSize);
                    printf("\t\t MVPrecision %d\n", (int)pCodingOption->MVPrecision);
                    printf("\t\t MaxDecFrameBuffering %d\n", (int)pCodingOption->MaxDecFrameBuffering);

                    printf("\t\t AUDelimiter %d\n", (int)pCodingOption->AUDelimiter);
                    printf("\t\t EndOfStream %d\n", (int)pCodingOption->EndOfStream);
                    printf("\t\t PicTimingSEI %d\n", (int)pCodingOption->PicTimingSEI);
                    printf("\t\t VuiNalHrdParameters %d\n", (int)pCodingOption->VuiNalHrdParameters);
                }
                break;

            case MFX_EXTBUFF_CODING_OPTION2:
                {
                    mfxExtCodingOption2 *pCodingOption2 = (mfxExtCodingOption2 *)pVideoParam->ExtParam[i];

                    printf("\t\t IntRefType %d\n", (int)pCodingOption2->IntRefType);
                    printf("\t\t IntRefCycleSize %d\n", (int)pCodingOption2->IntRefCycleSize);
                    printf("\t\t IntRefQPDelta %d\n", (int)pCodingOption2->IntRefQPDelta);

                    printf("\t\t MaxFrameSize %d\n", (int)pCodingOption2->MaxFrameSize);
                    printf("\t\t MaxSliceSize %d\n", (int)pCodingOption2->MaxSliceSize);

                    printf("\t\t BitrateLimit %d\n", (int)pCodingOption2->BitrateLimit);
                    printf("\t\t MBBRC %d\n", (int)pCodingOption2->MBBRC);
                    printf("\t\t ExtBRC %d\n", (int)pCodingOption2->ExtBRC);
                    printf("\t\t LookAheadDepth %d\n", (int)pCodingOption2->LookAheadDepth);
                    printf("\t\t Trellis %d\n", (int)pCodingOption2->Trellis);
                    printf("\t\t RepeatPPS %d\n", (int)pCodingOption2->RepeatPPS);
                    printf("\t\t BRefType %d\n", (int)pCodingOption2->BRefType);
                    printf("\t\t AdaptiveI %d\n", (int)pCodingOption2->AdaptiveI);
                    printf("\t\t AdaptiveB %d\n", (int)pCodingOption2->AdaptiveB);
                    printf("\t\t LookAheadDS %d\n", (int)pCodingOption2->LookAheadDS);
                    printf("\t\t NumMbPerSlice %d\n", (int)pCodingOption2->NumMbPerSlice);
                    printf("\t\t SkipFrame %d\n", (int)pCodingOption2->SkipFrame);
                    printf("\t\t MinQPI %d\n", (int)pCodingOption2->MinQPI);
                    printf("\t\t MaxQPI %d\n", (int)pCodingOption2->MaxQPI);
                    printf("\t\t MinQPP %d\n", (int)pCodingOption2->MinQPP);
                    printf("\t\t MaxQPP %d\n", (int)pCodingOption2->MaxQPP);
                    printf("\t\t MinQPB %d\n", (int)pCodingOption2->MinQPB);
                    printf("\t\t MaxQPB %d\n", (int)pCodingOption2->MaxQPB);
                    printf("\t\t FixedFrameRate %d\n", (int)pCodingOption2->FixedFrameRate);
                    printf("\t\t DisableDeblockingIdc %d\n", (int)pCodingOption2->DisableDeblockingIdc);
                    printf("\t\t DisableVUI %d\n", (int)pCodingOption2->DisableVUI);
                    printf("\t\t BufferingPeriodSEI %d\n", (int)pCodingOption2->BufferingPeriodSEI);
                    printf("\t\t EnableMAD %d\n", (int)pCodingOption2->EnableMAD);
                    printf("\t\t UseRawRef %d\n", (int)pCodingOption2->UseRawRef);
                }
                break;

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

const char *mfxStatusToStr(const mfxStatus sts)
{
    switch(sts) {
        case MFX_ERR_NONE:
            return "MFX_ERR_NONE";

        case MFX_ERR_UNKNOWN:
            return "MFX_ERR_UNKNOWN";

        case MFX_ERR_NULL_PTR:
            return "MFX_ERR_NULL_PTR";
        case MFX_ERR_UNSUPPORTED:
            return "MFX_ERR_UNSUPPORTED";
        case MFX_ERR_MEMORY_ALLOC:
            return "MFX_ERR_MEMORY_ALLOC";
        case MFX_ERR_NOT_ENOUGH_BUFFER:
            return "MFX_ERR_NOT_ENOUGH_BUFFER";
        case MFX_ERR_INVALID_HANDLE:
            return "MFX_ERR_INVALID_HANDLE";
        case MFX_ERR_LOCK_MEMORY:
            return "MFX_ERR_LOCK_MEMORY";
        case MFX_ERR_NOT_INITIALIZED:
            return "MFX_ERR_NOT_INITIALIZED";
        case MFX_ERR_NOT_FOUND:
            return "MFX_ERR_NOT_FOUND";
        case MFX_ERR_MORE_DATA:
            return "MFX_ERR_MORE_DATA";
        case MFX_ERR_MORE_SURFACE:
            return "MFX_ERR_MORE_SURFACE";
        case MFX_ERR_ABORTED:
            return "MFX_ERR_ABORTED";
        case MFX_ERR_DEVICE_LOST:
            return "MFX_ERR_DEVICE_LOST";
        case MFX_ERR_INCOMPATIBLE_VIDEO_PARAM:
            return "MFX_ERR_INCOMPATIBLE_VIDEO_PARAM";
        case MFX_ERR_INVALID_VIDEO_PARAM:
            return "MFX_ERR_INVALID_VIDEO_PARAM";
        case MFX_ERR_UNDEFINED_BEHAVIOR:
            return "MFX_ERR_UNDEFINED_BEHAVIOR";
        case MFX_ERR_DEVICE_FAILED:
            return "MFX_ERR_DEVICE_FAILED";
        case MFX_ERR_MORE_BITSTREAM:
            return "MFX_ERR_MORE_BITSTREAM";
        case MFX_ERR_INCOMPATIBLE_AUDIO_PARAM:
            return "MFX_ERR_INCOMPATIBLE_AUDIO_PARAM";
        case MFX_ERR_INVALID_AUDIO_PARAM:
            return "MFX_ERR_INVALID_AUDIO_PARAM";

        case MFX_WRN_IN_EXECUTION:
            return "MFX_WRN_IN_EXECUTION";
        case MFX_WRN_DEVICE_BUSY:
            return "MFX_WRN_DEVICE_BUSY";
        case MFX_WRN_VIDEO_PARAM_CHANGED:
            return "MFX_WRN_VIDEO_PARAM_CHANGED";
        case MFX_WRN_PARTIAL_ACCELERATION:
            return "MFX_WRN_PARTIAL_ACCELERATION";
        case MFX_WRN_INCOMPATIBLE_VIDEO_PARAM:
            return "MFX_WRN_INCOMPATIBLE_VIDEO_PARAM";
        case MFX_WRN_VALUE_NOT_CHANGED:
            return "MFX_WRN_VALUE_NOT_CHANGED";
        case MFX_WRN_OUT_OF_RANGE:
            return "MFX_WRN_OUT_OF_RANGE";
        case MFX_WRN_FILTER_SKIPPED:
            return "MFX_WRN_FILTER_SKIPPED";
        case MFX_WRN_INCOMPATIBLE_AUDIO_PARAM:
            return "MFX_WRN_INCOMPATIBLE_AUDIO_PARAM";

        default:
            return "?";
    }
}

}//namespace woogeen_base

#endif /* ENABLE_MSDK */
