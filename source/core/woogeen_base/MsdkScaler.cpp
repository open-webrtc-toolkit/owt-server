// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "MsdkBase.h"
#include "MsdkScaler.h"

namespace woogeen_base {

DEFINE_LOGGER(MsdkScaler, "woogeen.MsdkScaler");

MsdkScaler::MsdkScaler()
{
    initVppParam();
    createVpp();
}

MsdkScaler::~MsdkScaler()
{
    destroyVpp();
}

bool MsdkScaler::createVpp()
{
    mfxStatus sts = MFX_ERR_NONE;

    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase == NULL) {
        ELOG_ERROR("(%p)Get MSDK failed.", this);
        return false;
    }

    m_session = msdkBase->createSession();
    if (!m_session ) {
        ELOG_ERROR("(%p)Create session failed.", this);

        destroyVpp();
        return false;
    }

    m_allocator = msdkBase->createFrameAllocator();
    if (!m_allocator) {
        ELOG_ERROR("(%p)Create frame allocator failed.", this);

        destroyVpp();
        return false;
    }

    sts = m_session->SetFrameAllocator(m_allocator.get());
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("(%p)Set frame allocator failed.", this);

        destroyVpp();
        return false;
    }

    m_vpp = new MFXVideoVPP(*m_session);
    if (!m_vpp) {
        ELOG_ERROR("(%p)Create vpp failed.", this);

        destroyVpp();
        return false;
    }

    sts = m_vpp->Init(m_vppParam.get());
    if (sts > 0) {
        ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
    }
    else if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("(%p)mfx init failed, ret %d", this, sts);

        MsdkBase::printfVideoParam(m_vppParam.get(), MFX_VPP);

        destroyVpp();
        return false;
    }

    MsdkBase::printfVideoParam(m_vppParam.get(), MFX_VPP);
    return true;
}

void MsdkScaler::destroyVpp()
{
    if (m_vpp) {
        m_vpp->Close();
        delete m_vpp;
        m_vpp = NULL;
    }

    if (m_session) {
        MsdkBase *msdkBase = MsdkBase::get();
        if (msdkBase) {
            msdkBase->destroySession(m_session);
        }
        m_session = NULL;
    }

    m_allocator.reset();
    m_vppParam.reset();
}

void MsdkScaler::initVppParam()
{
    m_vppParam.reset(new mfxVideoParam);
    memset(m_vppParam.get(), 0, sizeof(mfxVideoParam));

    // mfxVideoParam Common
    m_vppParam->AsyncDepth              = 1;
    m_vppParam->IOPattern               = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    // mfxVideoParam Vpp In
    m_vppParam->vpp.In.FourCC           = MFX_FOURCC_NV12;
    m_vppParam->vpp.In.ChromaFormat     = MFX_CHROMAFORMAT_YUV420;
    m_vppParam->vpp.In.PicStruct        = MFX_PICSTRUCT_PROGRESSIVE;

    m_vppParam->vpp.In.BitDepthLuma     = 0;
    m_vppParam->vpp.In.BitDepthChroma   = 0;
    m_vppParam->vpp.In.Shift            = 0;

    m_vppParam->vpp.In.AspectRatioW     = 0;
    m_vppParam->vpp.In.AspectRatioH     = 0;

    m_vppParam->vpp.In.FrameRateExtN    = 30;
    m_vppParam->vpp.In.FrameRateExtD    = 1;

    m_vppParam->vpp.In.Width            = 16;
    m_vppParam->vpp.In.Height           = 16;
    m_vppParam->vpp.In.CropX            = 0;
    m_vppParam->vpp.In.CropY            = 0;
    m_vppParam->vpp.In.CropW            = 16;
    m_vppParam->vpp.In.CropH            = 16;

    // mfxVideoParam Vpp Out
    m_vppParam->vpp.Out.FourCC          = MFX_FOURCC_NV12;
    m_vppParam->vpp.Out.ChromaFormat    = MFX_CHROMAFORMAT_YUV420;
    m_vppParam->vpp.Out.PicStruct       = MFX_PICSTRUCT_PROGRESSIVE;

    m_vppParam->vpp.Out.BitDepthLuma    = 0;
    m_vppParam->vpp.Out.BitDepthChroma  = 0;
    m_vppParam->vpp.Out.Shift           = 0;

    m_vppParam->vpp.Out.AspectRatioW    = 0;
    m_vppParam->vpp.Out.AspectRatioH    = 0;

    m_vppParam->vpp.Out.FrameRateExtN   = 30;
    m_vppParam->vpp.Out.FrameRateExtD   = 1;

    m_vppParam->vpp.Out.Width           = 16;
    m_vppParam->vpp.Out.Height          = 16;
    m_vppParam->vpp.Out.CropX           = 0;
    m_vppParam->vpp.Out.CropY           = 0;
    m_vppParam->vpp.Out.CropW           = 16;
    m_vppParam->vpp.Out.CropH           = 16;
}

void MsdkScaler::updateVppParam(int inWidth, int inHeight, int inCropX, int inCropY, int inCropW, int inCropH,
        int outWidth, int outHeight, int outCropX, int outCropY, int outCropW, int outCropH)
{
    ELOG_DEBUG("%dx%d(%d-%d-%d-%d) -> %dx%d(%d-%d-%d-%d)"
            , inWidth, inHeight, inCropX, inCropY, inCropW, inCropH
            , outWidth, outHeight, outCropX, outCropY, outCropW, outCropH
            );

    m_vppParam->vpp.In.Width            = inWidth;
    m_vppParam->vpp.In.Height           = inHeight;
    m_vppParam->vpp.In.CropX            = inCropX;
    m_vppParam->vpp.In.CropY            = inCropY;
    m_vppParam->vpp.In.CropW            = inCropW;
    m_vppParam->vpp.In.CropH            = inCropH;

    m_vppParam->vpp.Out.Width           = outWidth;
    m_vppParam->vpp.Out.Height          = outHeight;
    m_vppParam->vpp.Out.CropX           = outCropX;
    m_vppParam->vpp.Out.CropY           = outCropY;
    m_vppParam->vpp.Out.CropW           = outCropW;
    m_vppParam->vpp.Out.CropH           = outCropH;
}

bool MsdkScaler::setupVpp(int inWidth, int inHeight, int inCropX, int inCropY, int inCropW, int inCropH,
            int outWidth, int outHeight, int outCropX, int outCropY, int outCropW, int outCropH)
{
    if (!m_vpp)
        return false;

    if (m_vppParam->vpp.In.Width != inWidth
            || m_vppParam->vpp.In.Height != inHeight
            || m_vppParam->vpp.In.CropX != inCropX
            || m_vppParam->vpp.In.CropY != inCropY
            || m_vppParam->vpp.In.CropW != inCropW
            || m_vppParam->vpp.In.CropH != inCropH
            || m_vppParam->vpp.Out.Width != outWidth
            || m_vppParam->vpp.Out.Height != outHeight
            || m_vppParam->vpp.Out.CropX != outCropX
            || m_vppParam->vpp.Out.CropY != outCropY
            || m_vppParam->vpp.Out.CropW != outCropW
            || m_vppParam->vpp.Out.CropH != outCropH
            ) {
        updateVppParam(inWidth, inHeight, inCropX, inCropY, inCropW, inCropH
            , outWidth, outHeight, outCropX, outCropY, outCropW, outCropH);

        mfxStatus sts = m_vpp->Reset(m_vppParam.get());
        if (sts > 0) {
            ELOG_TRACE("Ignore mfx warning, ret %d", sts);
        } else if (sts != MFX_ERR_NONE) {
            ELOG_TRACE("mfx reset failed, ret %d. Try to reinitialize.", sts);

            m_vpp->Close();
            sts = m_vpp->Init(m_vppParam.get());
            if (sts > 0) {
                ELOG_TRACE("Ignore mfx warning, ret %d", sts);
            } else if (sts != MFX_ERR_NONE) {
                MsdkBase::printfVideoParam(m_vppParam.get(), MFX_VPP);

                ELOG_ERROR("mfx init failed, ret %d", sts);

                destroyVpp();
                return false;
            }
        }

        m_vpp->GetVideoParam(m_vppParam.get());
        MsdkBase::printfVideoParam(m_vppParam.get(), MFX_VPP);
    }

    return true;
}

bool MsdkScaler::doVppConvert(MsdkFrame *srcMsdkFrame, MsdkFrame *dstMsdkFrame)
{
    mfxStatus sts = MFX_ERR_UNKNOWN;//MFX_ERR_NONE;
    mfxSyncPoint syncP;

retry:
    sts = m_vpp->RunFrameVPPAsync(srcMsdkFrame->getSurface(), dstMsdkFrame->getSurface(), NULL, &syncP);
    if (sts == MFX_WRN_DEVICE_BUSY) {
        ELOG_TRACE("(%p)Device busy, retry!", this);

        usleep(1000); //1ms
        goto retry;
    }
    else if (sts != MFX_ERR_NONE) {
        MsdkBase::printfVideoParam(m_vppParam.get(), MFX_VPP);

        ELOG_ERROR("src(%p), %dx%d(%d-%d-%d-%d), locked %d"
                , srcMsdkFrame
                , srcMsdkFrame->getWidth(), srcMsdkFrame->getHeight()
                , srcMsdkFrame->getCropX(), srcMsdkFrame->getCropY()
                , srcMsdkFrame->getCropW(), srcMsdkFrame->getCropH()
                , srcMsdkFrame->getLockedCount());
        ELOG_ERROR("dst(%p), %dx%d(%d-%d-%d-%d), locked %d"
                , dstMsdkFrame
                , dstMsdkFrame->getWidth(), dstMsdkFrame->getHeight()
                , dstMsdkFrame->getCropX(), dstMsdkFrame->getCropY()
                , dstMsdkFrame->getCropW(), dstMsdkFrame->getCropH()
                , dstMsdkFrame->getLockedCount());

        ELOG_ERROR("(%p)mfx vpp error, ret %d", this, sts);
        return false;
    }

    dstMsdkFrame->setSyncPoint(syncP);
    dstMsdkFrame->setSyncFlag(true);

#if 0
    sts = m_session->SyncOperation(syncP, MFX_INFINITE);
    if(sts != MFX_ERR_NONE) {
        ELOG_ERROR("(%p)SyncOperation failed, ret %d", this, sts);
        return false;
    }
#endif

    return true;
}

bool MsdkScaler::convert(MsdkFrame *srcMsdkFrame, MsdkFrame *dstMsdkFrame)
{
    if (!setupVpp(srcMsdkFrame->getWidth(), srcMsdkFrame->getHeight()
                , srcMsdkFrame->getCropX(), srcMsdkFrame->getCropY()
                , srcMsdkFrame->getCropW(), srcMsdkFrame->getCropH()
                , dstMsdkFrame->getWidth(), dstMsdkFrame->getHeight()
                , dstMsdkFrame->getCropX(), dstMsdkFrame->getCropY()
                , dstMsdkFrame->getCropW(), dstMsdkFrame->getCropH()
                )) {
        return false;
    }

    return doVppConvert(srcMsdkFrame, dstMsdkFrame);
}

bool MsdkScaler::convert(MsdkFrame *srcMsdkFrame, int srcCropX, int srcCropY, int srcCropW, int srcCropH
        , MsdkFrame *dstMsdkFrame, int dstCropX, int dstCropY, int dstCropW, int dstCropH)
{
    if (!setupVpp(srcMsdkFrame->getWidth(), srcMsdkFrame->getHeight()
                , srcCropX, srcCropY
                , srcCropW, srcCropH
                , dstMsdkFrame->getWidth(), dstMsdkFrame->getHeight()
                , dstCropX, dstCropY
                , dstCropW, dstCropH
                )) {
        return false;
    }

    return doVppConvert(srcMsdkFrame, dstMsdkFrame);
}

}//namespace woogeen_base
