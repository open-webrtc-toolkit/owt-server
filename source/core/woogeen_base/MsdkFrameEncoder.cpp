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

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "MediaUtilities.h"

#include "MsdkBase.h"
#include "MsdkFrameEncoder.h"

#define _MAX_BITSTREAM_BUFFER_ (100 * 1024 * 1024)

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(MsdkFrameEncoder, "woogeen.MsdkFrameEncoder");

class StreamEncoder : public FrameSource
{
    DECLARE_LOGGER()

public:
    StreamEncoder()
        : m_frameCount(0)
        , m_format(FRAME_FORMAT_UNKNOWN)
        , m_width(0)
        , m_height(0)
        , m_bitRateKbps(0)
        , m_dest(NULL)
        , m_setBitRateFlag(false)
        , m_requestKeyFrameFlag(false)
        , m_encSession(NULL)
        , m_enc(NULL)
        , m_vppSession(NULL)
        , m_vpp(NULL)
    {
        initDefaultParam();
    }

    ~StreamEncoder()
    {
        printfFuncEnter;

        removeVideoDestination(m_dest);

        if (m_enc) {
            m_enc->Close();
            delete m_enc;
            m_enc = NULL;
        }

        if (m_encSession) {
            //disjoint
            m_encSession->Close();
            delete m_encSession;
            m_encSession = NULL;
        }

        m_encAllocator.reset();

        m_encParam.reset();
        m_encExtCodingOpt.reset();
        m_encExtCodingOpt2.reset();
        m_encExtParams.clear();

        if (m_bitstream && m_bitstream->Data) {
            free(m_bitstream->Data);
            m_bitstream->Data = NULL;
        }
        m_bitstream.reset();

        closeVpp();

        printfFuncExit;
    }

    void onFeedback(const woogeen_base::FeedbackMsg& msg) {
        if (msg.type == woogeen_base::VIDEO_FEEDBACK) {
            if (msg.cmd == REQUEST_KEY_FRAME) {
                requestKeyFrame();
            } else if (msg.cmd == SET_BITRATE) {
                setBitrate(msg.data.kbps);
            }
        }
    }

    bool init(FrameFormat format, uint32_t width, uint32_t height, uint32_t bitrateKbps, FrameDestination* dest)
    {
        if (!dest) {
            ELOG_ERROR("(%p)Null FrameDestination.", this);
            return false;
        }
        m_format        = format;
        m_width         = width;
        m_height        = height;
        m_bitRateKbps   = bitrateKbps;
        m_dest          = dest;
        addVideoDestination(dest);

        updateParam();

        switch (format) {
            case FRAME_FORMAT_H264:
                m_encParam->mfx.CodecId                = MFX_CODEC_AVC;
                m_encParam->mfx.CodecProfile           = MFX_PROFILE_AVC_BASELINE;

                ELOG_DEBUG("(%p)Created H.264 encoder.", this);
                break;

            case FRAME_FORMAT_VP8:
            default:
                ELOG_ERROR("(%p)Unspported video frame format %d", this, format);
                return false;
        }

        mfxStatus sts = MFX_ERR_NONE;

        MsdkBase *msdkBase = MsdkBase::get();
        if(msdkBase == NULL) {
            ELOG_ERROR("(%p)Get MSDK failed.", this);
            return false;
        }

        m_encSession = msdkBase->createSession();
        if (!m_encSession ) {
            ELOG_ERROR("(%p)Create session failed.", this);
            return false;
        }

        m_encAllocator = msdkBase->createFrameAllocator();
        if (!m_encAllocator) {
            ELOG_ERROR("(%p)Create frame allocator failed.", this);
            return false;
        }

        sts = m_encSession->SetFrameAllocator(m_encAllocator.get());
        if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)Set frame allocator failed.", this);
            return false;
        }

        m_enc = new MFXVideoENCODE(*m_encSession);
        if (!m_enc) {
            ELOG_ERROR("(%p)Create encode failed", this);

            return false;
        }

        sts = m_enc->Init(m_encParam.get());
        if (sts > 0) {
            ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
        }
        else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)mfx init failed, ret %d", this, sts);

            MsdkBase::printfVideoParam(m_encParam.get(), MFX_ENC);
            return false;
        }

        m_enc->GetVideoParam(m_encParam.get());
        MsdkBase::printfVideoParam(m_encParam.get(), MFX_ENC);

        ELOG_TRACE("(%p)Init bistream buffer, %dx%d", this,
                m_width,
                m_height
                );

        int size = m_width * m_height * 3 / 2;

        if (size == 0)
            size = 1 * 1024 * 1024;

        m_bitstream.reset(new mfxBitstream);
        memset((void *)m_bitstream.get(), 0, sizeof(mfxBitstream));

        m_bitstream->Data         = (mfxU8 *)malloc(size);
        m_bitstream->MaxLength    = size;
        m_bitstream->DataOffset   = 0;
        m_bitstream->DataLength   = 0;

        return true;
    }

    void onFrame(const woogeen_base::Frame& frame)
    {
        mfxStatus sts = MFX_ERR_NONE;
        mfxSyncPoint syncp;
        boost::scoped_ptr<mfxEncodeCtrl> ctrl;

        boost::shared_ptr<MsdkFrame> inFrame = convert(frame);

        if (m_setBitRateFlag) {
            ELOG_INFO("(%p)Do set bitrate!", this);

            updateParam();

            sts = m_enc->Reset(m_encParam.get());
            if (sts > 0) {
                ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
            }
            else if (sts != MFX_ERR_NONE) {
                ELOG_ERROR("(%p)mfx reset failed, ret %d", this, sts);
            }

            m_enc->GetVideoParam(m_encParam.get());
            MsdkBase::printfVideoParam(m_encParam.get(), MFX_ENC);

            m_setBitRateFlag = false;
        }

        if (m_requestKeyFrameFlag) {
            ELOG_INFO("(%p)Do requeset key frame!", this);

            ctrl.reset(new mfxEncodeCtrl);

            memset(ctrl.get(), 0, sizeof(mfxEncodeCtrl));
            ctrl->FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;

            m_requestKeyFrameFlag = false;
        }

retry:
        sts = m_enc->EncodeFrameAsync(ctrl.get(), inFrame->getSurface(), m_bitstream.get(), &syncp);
        if (sts == MFX_WRN_DEVICE_BUSY) {
            ELOG_TRACE("(%p)Device busy, retry!", this);

            usleep(1000); //1ms
            goto retry;
        }
        else if (sts == MFX_ERR_NOT_ENOUGH_BUFFER) {
            ELOG_WARN("(%p)Require more bitstream buffer, %d!", this, m_bitstream->MaxLength);

            uint32_t newSize = m_bitstream->MaxLength * 2;
            if (newSize > _MAX_BITSTREAM_BUFFER_) {
                ELOG_ERROR("(%p)Exceed max bitstream buffer size(%d), %d!", this, _MAX_BITSTREAM_BUFFER_, newSize);
                return;
            }

            ELOG_WARN("(%p)bitstream buffer need to remalloc %d -> %d"
                    , this
                    , m_bitstream->MaxLength
                    , newSize
                    );

            mfxU8 *newBuffer = (mfxU8 *)malloc(newSize);
            if (m_bitstream->DataLength > 0)
                memcpy(newBuffer, m_bitstream->Data, m_bitstream->DataLength);

            free(m_bitstream->Data);
            m_bitstream->Data         = newBuffer;
            m_bitstream->MaxLength    = newSize;

            goto retry;
        }
        else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)mfx encode error, ret %d", this, sts);
            return;
        }

        sts = m_encSession->SyncOperation(syncp, MFX_INFINITE);
        if(sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)SyncOperation failed, ret %d", this, sts);
            return;
        }

        if (m_bitstream->DataLength <= 0) {
            ELOG_ERROR("(%p)No output bitstream buffer", this);
            return;
        }

        ELOG_TRACE("(%p)Output FrameType %s(0x%x), DataOffset %d , DataLength %d", this,
                getFrameType(m_bitstream->FrameType),
                m_bitstream->FrameType,
                m_bitstream->DataOffset,
                m_bitstream->DataLength
              );

        Frame outFrame;
        memset(&outFrame, 0, sizeof(outFrame));
        outFrame.format = m_format;
        outFrame.payload = m_bitstream->Data + m_bitstream->DataOffset;
        outFrame.length = m_bitstream->DataLength;
        outFrame.additionalInfo.video.width = m_width;
        outFrame.additionalInfo.video.height = m_height;

        outFrame.timeStamp = (m_frameCount++) * 1000 / 30 * 90;

        //ELOG_TRACE("timeStamp %u", frame.timeStamp);

        //dump(outFrame.payload, outFrame.length);

        deliverFrame(outFrame);

        m_bitstream->DataOffset   = 0;
        m_bitstream->DataLength   = 0;
    }

    void setBitrate(unsigned short kbps)
    {
        ELOG_DEBUG("(%p)setBitrate %d", this, kbps);

        m_bitRateKbps = kbps;
        m_setBitRateFlag = true;
    }

    void requestKeyFrame()
    {
        ELOG_DEBUG("(%p)requestKeyFrame", this);

        m_requestKeyFrameFlag = true;
    }

protected:
    boost::shared_ptr<MsdkFrame> convert(const woogeen_base::Frame& frame)
    {
        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
        boost::shared_ptr<MsdkFrame> msdkFrame = holder->frame;

        if(m_width != frame.additionalInfo.video.width || m_height != frame.additionalInfo.video.height) {
            if (!m_vpp) {
                ELOG_DEBUG("(%p)Init vpp for frame scaling, %dx%d -> %dx%d"
                        , this
                        , frame.additionalInfo.video.width
                        , frame.additionalInfo.video.height
                        , m_width
                        , m_height
                        );

                initVpp(frame.additionalInfo.video.width, frame.additionalInfo.video.height);
                if (!m_vpp) {
                    ELOG_ERROR("(%p)Init vpp failed", this);
                    return NULL;
                }
            }

            msdkFrame = doScale(msdkFrame);
        }

        return msdkFrame;
    }

    void initDefaultParam(void)
    {
        m_encParam.reset(new mfxVideoParam);
        memset(m_encParam.get(), 0, sizeof(mfxVideoParam));

        m_encExtCodingOpt.reset(new mfxExtCodingOption);
        memset(m_encExtCodingOpt.get(), 0, sizeof(mfxExtCodingOption));

        m_encExtCodingOpt2.reset(new mfxExtCodingOption2);
        memset(m_encExtCodingOpt2.get(), 0, sizeof(mfxExtCodingOption2));

        m_encExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_encExtCodingOpt.get()));
        m_encExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_encExtCodingOpt2.get()));

        // FrameInfo
        m_encParam->mfx.FrameInfo.FourCC          = MFX_FOURCC_NV12;
        m_encParam->mfx.FrameInfo.ChromaFormat    = MFX_CHROMAFORMAT_YUV420;
        m_encParam->mfx.FrameInfo.PicStruct       = MFX_PICSTRUCT_PROGRESSIVE;

        m_encParam->mfx.FrameInfo.BitDepthLuma    = 0;
        m_encParam->mfx.FrameInfo.BitDepthChroma  = 0;
        m_encParam->mfx.FrameInfo.Shift           = 0;

        m_encParam->mfx.FrameInfo.AspectRatioW    = 0;
        m_encParam->mfx.FrameInfo.AspectRatioH    = 0;

        m_encParam->mfx.FrameInfo.FrameRateExtN   = 30;
        m_encParam->mfx.FrameInfo.FrameRateExtD   = 1;

        m_encParam->mfx.FrameInfo.Width           = ALIGN16(1280);
        m_encParam->mfx.FrameInfo.Height          = ALIGN16(720);
        m_encParam->mfx.FrameInfo.CropX           = 0;
        m_encParam->mfx.FrameInfo.CropY           = 0;
        m_encParam->mfx.FrameInfo.CropW           = 1280;
        m_encParam->mfx.FrameInfo.CropH           = 720;

        // mfxVideoParam Common
        m_encParam->AsyncDepth                    = 1;
        m_encParam->IOPattern                     = MFX_IOPATTERN_IN_VIDEO_MEMORY;

        //m_encParam->ExtParam                    = &m_encExtParams.front(); // vector is stored linearly in memory
        //m_encParam->NumExtParam                 = m_encExtParams.size();

        // mfx
        m_encParam->mfx.LowPower                  = 0;
        m_encParam->mfx.BRCParamMultiplier        = 0;
        m_encParam->mfx.CodecId                   = MFX_CODEC_AVC;
        m_encParam->mfx.CodecProfile              = MFX_PROFILE_AVC_BASELINE;
        m_encParam->mfx.CodecLevel                = 0;
        m_encParam->mfx.NumThread                 = 0;

        // mfx Enc
        m_encParam->mfx.TargetUsage               = 0;
        m_encParam->mfx.GopPicSize                = 24;
        m_encParam->mfx.GopRefDist                = 0;
        m_encParam->mfx.GopOptFlag                = 0;
        m_encParam->mfx.IdrInterval               = 0;

        m_encParam->mfx.RateControlMethod         = MFX_RATECONTROL_VBR;

        m_encParam->mfx.BufferSizeInKB            = 0;
        m_encParam->mfx.InitialDelayInKB          = 0;
        m_encParam->mfx.TargetKbps                = calcBitrate(m_encParam->mfx.FrameInfo.Width, m_encParam->mfx.FrameInfo.Height);
        m_encParam->mfx.MaxKbps                   = m_encParam->mfx.TargetKbps;

        m_encParam->mfx.NumSlice                  = 0;
        m_encParam->mfx.NumRefFrame               = 1;
        m_encParam->mfx.EncodedOrder              = 0;

        // MFX_EXTBUFF_CODING_OPTION
        m_encExtCodingOpt->Header.BufferId             = MFX_EXTBUFF_CODING_OPTION;
        m_encExtCodingOpt->Header.BufferSz             = sizeof(*m_encExtCodingOpt);
        m_encExtCodingOpt->MaxDecFrameBuffering        = m_encParam->mfx.NumRefFrame;
        m_encExtCodingOpt->AUDelimiter                 = MFX_CODINGOPTION_OFF;//No AUD
        m_encExtCodingOpt->RecoveryPointSEI            = MFX_CODINGOPTION_OFF;//No SEI
        m_encExtCodingOpt->PicTimingSEI                = MFX_CODINGOPTION_OFF;
        m_encExtCodingOpt->VuiNalHrdParameters         = MFX_CODINGOPTION_OFF;

        // MFX_EXTBUFF_CODING_OPTION2
        m_encExtCodingOpt2->Header.BufferId            = MFX_EXTBUFF_CODING_OPTION2;
        m_encExtCodingOpt2->Header.BufferSz            = sizeof(*m_encExtCodingOpt2);
        m_encExtCodingOpt2->RepeatPPS                  = MFX_CODINGOPTION_OFF;//No repeat pps
        m_encExtCodingOpt2->MBBRC                      = 0;//Disable
        m_encExtCodingOpt2->LookAheadDepth             = 0;//For MFX_RATECONTROL_LA
        m_encExtCodingOpt2->MaxSliceSize               = 0;
    }

    void updateParam(void)
    {
        m_encParam->mfx.FrameInfo.Width   = ALIGN16(m_width);
        m_encParam->mfx.FrameInfo.Height  = ALIGN16(m_height);
        m_encParam->mfx.FrameInfo.CropX   = 0;
        m_encParam->mfx.FrameInfo.CropY   = 0;
        m_encParam->mfx.FrameInfo.CropW   = m_width;
        m_encParam->mfx.FrameInfo.CropH   = m_height;

        m_encParam->mfx.TargetKbps        = m_bitRateKbps ? m_bitRateKbps : calcBitrate(m_width, m_height);
        m_encParam->mfx.MaxKbps           = m_encParam->mfx.TargetKbps;
    }

    char *getFrameType(uint16_t frameType)
    {
        static char FrameType[128];

        FrameType[0] = '\0';

        if (frameType & MFX_FRAMETYPE_I) {
            snprintf(FrameType, 128, "%s", "I");
        }

        if (frameType & MFX_FRAMETYPE_P) {
            snprintf(FrameType, 128, "%s-%s", FrameType, "P");
        }

        if (frameType & MFX_FRAMETYPE_B) {
            snprintf(FrameType, 128, "%s-%s", FrameType, "B");
        }

        if (frameType & MFX_FRAMETYPE_S) {
            snprintf(FrameType, 128, "%s-%s", FrameType, "S");
        }

        if (frameType & MFX_FRAMETYPE_REF) {
            snprintf(FrameType, 128, "%s-%s", FrameType, "REF");
        }

        if (frameType & MFX_FRAMETYPE_IDR) {
            snprintf(FrameType, 128, "%s-%s", FrameType, "IDR");
        }

        return FrameType;
    }

    void dump(uint8_t *buf, int len)
    {
        static const char dumpFile[] = "/tmp/webrtc-msdk-dump.h264";

        FILE *fp = fopen(dumpFile, "ab");
        if (fp) {
            fwrite(buf, 1, len, fp);
            fclose(fp);
            ELOG_DEBUG("Dump bitstream into %s, len %d", dumpFile, len);
        } else {
            ELOG_DEBUG("Dump failed, can not open file %s", dumpFile);
        }
    }

    void initVppParam(int inputWidth, int inputHeight)
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

        m_vppParam->vpp.In.Width            = ALIGN16(inputWidth);
        m_vppParam->vpp.In.Height           = ALIGN16(inputHeight);
        m_vppParam->vpp.In.CropX            = 0;
        m_vppParam->vpp.In.CropY            = 0;
        m_vppParam->vpp.In.CropW            = inputWidth;
        m_vppParam->vpp.In.CropH            = inputHeight;

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

        m_vppParam->vpp.Out.Width           = ALIGN16(m_width);
        m_vppParam->vpp.Out.Height          = ALIGN16(m_height);

        if (inputWidth * m_height > m_width * inputHeight) {
            m_vppParam->vpp.Out.CropW   = m_width;
            m_vppParam->vpp.Out.CropH   = inputHeight * m_width / inputWidth;

            m_vppParam->vpp.Out.CropX   = 0;
            m_vppParam->vpp.Out.CropY   = (m_height - m_vppParam->vpp.Out.CropH) / 2;
        } else {
            m_vppParam->vpp.Out.CropW   = inputWidth * m_height / inputHeight;
            m_vppParam->vpp.Out.CropH   = m_height;

            m_vppParam->vpp.Out.CropX   = (m_width - m_vppParam->vpp.Out.CropW) / 2;
            m_vppParam->vpp.Out.CropY   = 0;
        }
    }

    bool initVpp(int inputWidth, int inputHeight)
    {
        mfxStatus sts = MFX_ERR_NONE;

        initVppParam(inputWidth, inputHeight);

        MsdkBase *msdkBase = MsdkBase::get();
        if(msdkBase == NULL) {
            ELOG_ERROR("(%p)Get MSDK failed.", this);

            closeVpp();
            return false;
        }

        m_vppSession = msdkBase->createSession();
        if (!m_vppSession ) {
            ELOG_ERROR("(%p)Create session failed.", this);

            closeVpp();
            return false;
        }

        m_vppAllocator = msdkBase->createFrameAllocator();
        if (!m_vppAllocator) {
            ELOG_ERROR("(%p)Create frame allocator failed.", this);

            closeVpp();
            return false;
        }

        sts = m_vppSession->SetFrameAllocator(m_vppAllocator.get());
        if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)Set frame allocator failed.", this);

            closeVpp();
            return false;
        }

        m_vpp = new MFXVideoVPP(*m_vppSession);
        if (!m_vpp) {
            ELOG_ERROR("(%p)Create vpp failed.", this);

            closeVpp();
            return false;
        }

        sts = m_vpp->Init(m_vppParam.get());
        if (sts > 0) {
            ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
        }
        else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)mfx init failed, ret %d", this, sts);

            MsdkBase::printfVideoParam(m_vppParam.get(), MFX_VPP);

            closeVpp();
            return false;
        }

        MsdkBase::printfVideoParam(m_vppParam.get(), MFX_VPP);

        mfxFrameAllocRequest Request[2];
        memset(&Request, 0, sizeof(mfxFrameAllocRequest) * 2);

        sts = m_vpp->QueryIOSurf(m_vppParam.get(), Request);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts || MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts)
        {
            ELOG_TRACE("(%p)Ignore warning!", this);
        }
        if (MFX_ERR_NONE != sts)
        {
            ELOG_ERROR("(%p)mfx QueryIOSurf() failed, ret %d", this, sts);

            closeVpp();
            return false;
        }

        ELOG_TRACE("(%p)mfx QueryIOSurf: In(%d), Out(%d)", this, Request[0].NumFrameSuggested, Request[1].NumFrameSuggested);

        m_vppFramePool.reset(new MsdkFramePool(m_vppAllocator, Request[1]));
        if (!m_vppFramePool->init()) {
            ELOG_ERROR("(%p)Frame pool init failed, ret %d", this, sts);

            closeVpp();
            return false;
        }

        return true;
    }

    void closeVpp()
    {
        if (m_vpp) {
            m_vpp->Close();
            delete m_vpp;
            m_vpp = NULL;
        }

        if (m_vppSession) {
            //disjoint
            m_vppSession->Close();
            delete m_vppSession;
            m_vppSession = NULL;
        }

        m_vppAllocator.reset();

        m_vppParam.reset();

        m_vppFramePool.reset();
    }

    boost::shared_ptr<MsdkFrame> doScale(boost::shared_ptr<MsdkFrame> src)
    {
        mfxStatus sts = MFX_ERR_UNKNOWN;//MFX_ERR_NONE;

        mfxSyncPoint syncP;

        boost::shared_ptr<MsdkFrame> dst = m_vppFramePool->getFreeFrame();
        if (!dst) {
            ELOG_WARN("(%p)No frame available", this);
            return NULL;
        }

retry:
        sts = m_vpp->RunFrameVPPAsync(src->getSurface(), dst->getSurface(), NULL, &syncP);
        if (sts == MFX_WRN_DEVICE_BUSY) {
            ELOG_TRACE("(%p)Device busy, retry!", this);

            usleep(1000); //1ms
            goto retry;
        }
        else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)mfx vpp error, ret %d", this, sts);
            return NULL;
        }

        dst->setSyncPoint(syncP);
        dst->setSyncFlag(true);

#if 0
        sts = m_vppSession->SyncOperation(syncP, MFX_INFINITE);
        if(sts != MFX_ERR_NONE)
        {
            ELOG_ERROR("(%p)SyncOperation failed, ret %d", this, sts);
            return NULL;
        }
#endif

        return dst;
    }

private:
    //format
    uint32_t m_frameCount;

    FrameFormat m_format;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_bitRateKbps;
    FrameDestination *m_dest;

    bool m_setBitRateFlag;
    bool m_requestKeyFrameFlag;

    //encoder
    MFXVideoSession *m_encSession;
    MFXVideoENCODE *m_enc;

    boost::shared_ptr<mfxFrameAllocator> m_encAllocator;

    boost::scoped_ptr<mfxVideoParam> m_encParam;

    std::vector<mfxExtBuffer *> m_encExtParams;
    boost::scoped_ptr<mfxExtCodingOption> m_encExtCodingOpt;
    boost::scoped_ptr<mfxExtCodingOption2> m_encExtCodingOpt2;

    boost::scoped_ptr<mfxBitstream> m_bitstream;

    //vpp
    MFXVideoSession *m_vppSession;
    MFXVideoVPP *m_vpp;

    boost::shared_ptr<mfxFrameAllocator> m_vppAllocator;

    boost::scoped_ptr<mfxVideoParam> m_vppParam;

    boost::scoped_ptr<MsdkFramePool> m_vppFramePool;
};

DEFINE_LOGGER(StreamEncoder, "woogeen.StreamEncoder");

MsdkFrameEncoder::MsdkFrameEncoder(woogeen_base::FrameFormat format, bool useSimulcast)
    : m_encodeFormat(format)
    , m_useSimulcast(useSimulcast)
    , m_id(0)
{
}

MsdkFrameEncoder::~MsdkFrameEncoder()
{
    printfFuncEnter;

    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

    m_streams.clear();

    printfFuncExit;
}

bool MsdkFrameEncoder::canSimulcast(FrameFormat format, uint32_t width, uint32_t height)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    return false;
}

bool MsdkFrameEncoder::isIdle()
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    return m_streams.size() == 0;
}

int32_t MsdkFrameEncoder::generateStream(uint32_t width, uint32_t height, uint32_t bitrateKbps, woogeen_base::FrameDestination* dest)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);

    boost::shared_ptr<StreamEncoder> stream(new StreamEncoder());
    if (!stream->init(m_encodeFormat, width, height, bitrateKbps, dest)) {
        ELOG_ERROR("generateStream failed: {.width=%d, .height=%d, .bitrateKbps=%d}", width, height, bitrateKbps);
        return -1;
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);

    m_streams[m_id] = stream;

    ELOG_DEBUG("generateStream[%d]: {.width=%d, .height=%d, .bitrateKbps=%d}", m_id, width, height, bitrateKbps);

    return m_id++;
}

void MsdkFrameEncoder::degenerateStream(int id)
{
    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

    ELOG_DEBUG("degenerateStream[%d]", id);

    m_streams.erase(id);
}

void MsdkFrameEncoder::setBitrate(unsigned short kbps, int id)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG("setBitrate[%d] %d", id, kbps);

    auto it = m_streams.find(id);
    if (it != m_streams.end()) {
        it->second->setBitrate(kbps);
    }
}

void MsdkFrameEncoder::requestKeyFrame(int id)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG("requestKeyFrame[%d]", id);

    auto it = m_streams.find(id);
    if (it != m_streams.end()) {
        it->second->requestKeyFrame();
    }
}

void MsdkFrameEncoder::onFrame(const Frame& frame)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    switch (frame.format) {
        case FRAME_FORMAT_MSDK:
            {
                for (auto it = m_streams.begin(); it != m_streams.end(); ++it) {
                    it->second->onFrame(frame);
                }

                break;
            }

        case FRAME_FORMAT_VP8:
            assert(false);
            break;

        case FRAME_FORMAT_H264:
            assert(false);
            break;

        default:
            break;
    }
}

} // namespace woogeen_base
#endif /* ENABLE_MSDK */
