/*
 * Copyright 2016 Intel Corporation All Rights Reserved.
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

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(MsdkFrameEncoder, "woogeen.MsdkFrameEncoder");

class StreamEncoder : public FrameSource
{
    DECLARE_LOGGER()

public:
    StreamEncoder()
        : m_frameCount(0)
        , m_width(0)
        , m_height(0)
        , m_format(FRAME_FORMAT_UNKNOWN)
        , m_dest(NULL)
        , m_bitRateKbps(0)
        , m_setBitRateFlag(false)
        , m_requestKeyFrameFlag(false)
        , m_session(NULL)
        , m_enc(NULL)
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

        if (m_session) {
            //disjoint
            m_session->Close();
            delete m_session;
            m_session = NULL;
        }

        m_allocator.reset();

        m_videoParam.reset();
        m_extCodingOpt.reset();
        m_extCodingOpt2.reset();
        m_extParams.clear();

        if (m_bitstream && m_bitstream->Data) {
            free(m_bitstream->Data);
            m_bitstream->Data = NULL;
        }
        m_bitstream.reset();

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

    bool init(FrameFormat format, uint32_t width, uint32_t height, FrameDestination* dest)
    {
        if (!dest) {
            ELOG_ERROR("(%p)Null FrameDestination.", this);
            return false;
        }
        m_format = format;
        m_width = width;
        m_height = height;
        m_dest = dest;
        addVideoDestination(dest);

        updateParam();

        switch (format) {
            case FRAME_FORMAT_H264:
                m_videoParam->mfx.CodecId                = MFX_CODEC_AVC;
                m_videoParam->mfx.CodecProfile           = MFX_PROFILE_AVC_BASELINE;

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

        m_session = msdkBase->createSession();
        if (!m_session ) {
            ELOG_ERROR("(%p)Create session failed.", this);
            return false;
        }

        m_allocator = msdkBase->createFrameAllocator();
        if (!m_allocator) {
            ELOG_ERROR("(%p)Create frame allocator failed.", this);
            return false;
        }

        sts = m_session->SetFrameAllocator(m_allocator.get());
        if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)Set frame allocator failed.", this);
            return false;
        }

        m_enc = new MFXVideoENCODE(*m_session);
        if (!m_enc) {
            ELOG_ERROR("(%p)Create encode failed", this);

            return false;
        }

        sts = m_enc->Init(m_videoParam.get());
        if (sts > 0) {
            ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
        }
        else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)mfx init failed, ret %d", this, sts);

            printfVideoParam(m_videoParam.get(), MFX_ENC);
            return false;
        }

        m_enc->GetVideoParam(m_videoParam.get());
        printfVideoParam(m_videoParam.get(), MFX_ENC);

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

            sts = m_enc->Reset(m_videoParam.get());
            if (sts > 0) {
                ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
            }
            else if (sts != MFX_ERR_NONE) {
                ELOG_ERROR("(%p)mfx reset failed, ret %d", this, sts);
            }

            m_enc->GetVideoParam(m_videoParam.get());
            printfVideoParam(m_videoParam.get(), MFX_ENC);

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
            ELOG_WARN("(%p)Require more bitstream buffer!", this);

            return;
        }
        else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)mfx encode error, ret %d", this, sts);
            return;
        }

        sts = m_session->SyncOperation(syncp, MFX_INFINITE);
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

        deliverFrame(outFrame);

        m_bitstream->DataOffset   = 0;
        m_bitstream->DataLength   = 0;

        //dump(frame.payload, frame.length);
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
        return holder->frame;
    }

    void initDefaultParam(void)
    {
        m_videoParam.reset(new mfxVideoParam);
        memset(m_videoParam.get(), 0, sizeof(mfxVideoParam));

        m_extCodingOpt.reset(new mfxExtCodingOption);
        memset(m_extCodingOpt.get(), 0, sizeof(mfxExtCodingOption));

        m_extCodingOpt2.reset(new mfxExtCodingOption2);
        memset(m_extCodingOpt2.get(), 0, sizeof(mfxExtCodingOption2));

        m_extParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_extCodingOpt.get()));
        m_extParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_extCodingOpt2.get()));

        // FrameInfo
        m_videoParam->mfx.FrameInfo.FourCC          = MFX_FOURCC_NV12;
        m_videoParam->mfx.FrameInfo.ChromaFormat    = MFX_CHROMAFORMAT_YUV420;
        m_videoParam->mfx.FrameInfo.PicStruct       = MFX_PICSTRUCT_PROGRESSIVE;

        m_videoParam->mfx.FrameInfo.BitDepthLuma    = 0;
        m_videoParam->mfx.FrameInfo.BitDepthChroma  = 0;
        m_videoParam->mfx.FrameInfo.Shift           = 0;

        m_videoParam->mfx.FrameInfo.AspectRatioW    = 0;
        m_videoParam->mfx.FrameInfo.AspectRatioH    = 0;

        m_videoParam->mfx.FrameInfo.FrameRateExtN   = 30;
        m_videoParam->mfx.FrameInfo.FrameRateExtD   = 1;

        m_videoParam->mfx.FrameInfo.Width           = ALIGN16(1280);
        m_videoParam->mfx.FrameInfo.Height          = ALIGN16(720);
        m_videoParam->mfx.FrameInfo.CropX           = 0;
        m_videoParam->mfx.FrameInfo.CropY           = 0;
        m_videoParam->mfx.FrameInfo.CropW           = 1280;
        m_videoParam->mfx.FrameInfo.CropH           = 720;

        // mfxVideoParam Common
        m_videoParam->AsyncDepth                    = 1;
        m_videoParam->IOPattern                     = MFX_IOPATTERN_IN_VIDEO_MEMORY;

        //m_videoParam->ExtParam                    = &m_extParams.front(); // vector is stored linearly in memory
        //m_videoParam->NumExtParam                 = m_extParams.size();

        // mfx
        m_videoParam->mfx.LowPower                  = 0;
        m_videoParam->mfx.BRCParamMultiplier        = 0;
        m_videoParam->mfx.CodecId                   = MFX_CODEC_AVC;
        m_videoParam->mfx.CodecProfile              = MFX_PROFILE_AVC_BASELINE;
        m_videoParam->mfx.CodecLevel                = 0;
        m_videoParam->mfx.NumThread                 = 0;

        // mfx Enc
        m_videoParam->mfx.TargetUsage               = 0;
        m_videoParam->mfx.GopPicSize                = 24;
        m_videoParam->mfx.GopRefDist                = 0;
        m_videoParam->mfx.GopOptFlag                = 0;
        m_videoParam->mfx.IdrInterval               = 0;

        m_videoParam->mfx.RateControlMethod         = MFX_RATECONTROL_VBR;

        m_videoParam->mfx.BufferSizeInKB            = 0;
        m_videoParam->mfx.InitialDelayInKB          = 0;
        m_videoParam->mfx.TargetKbps                = calcBitrate(m_videoParam->mfx.FrameInfo.Width, m_videoParam->mfx.FrameInfo.Height);
        m_videoParam->mfx.MaxKbps                   = m_videoParam->mfx.TargetKbps;

        m_videoParam->mfx.NumSlice                  = 0;
        m_videoParam->mfx.NumRefFrame               = 1;
        m_videoParam->mfx.EncodedOrder              = 0;

        // MFX_EXTBUFF_CODING_OPTION
        m_extCodingOpt->Header.BufferId             = MFX_EXTBUFF_CODING_OPTION;
        m_extCodingOpt->Header.BufferSz             = sizeof(*m_extCodingOpt);
        m_extCodingOpt->MaxDecFrameBuffering        = m_videoParam->mfx.NumRefFrame;
        m_extCodingOpt->AUDelimiter                 = MFX_CODINGOPTION_OFF;//No AUD
        m_extCodingOpt->RecoveryPointSEI            = MFX_CODINGOPTION_OFF;//No SEI
        m_extCodingOpt->PicTimingSEI                = MFX_CODINGOPTION_OFF;
        m_extCodingOpt->VuiNalHrdParameters         = MFX_CODINGOPTION_OFF;

        // MFX_EXTBUFF_CODING_OPTION2
        m_extCodingOpt2->Header.BufferId            = MFX_EXTBUFF_CODING_OPTION2;
        m_extCodingOpt2->Header.BufferSz            = sizeof(*m_extCodingOpt2);
        m_extCodingOpt2->RepeatPPS                  = MFX_CODINGOPTION_OFF;//No repeat pps
        m_extCodingOpt2->MBBRC                      = 0;//Disable
        m_extCodingOpt2->LookAheadDepth             = 0;//For MFX_RATECONTROL_LA
        m_extCodingOpt2->MaxSliceSize               = 0;
    }

    void updateParam(void)
    {
        m_videoParam->mfx.FrameInfo.Width   = ALIGN16(m_width);
        m_videoParam->mfx.FrameInfo.Height  = ALIGN16(m_height);
        m_videoParam->mfx.FrameInfo.CropX   = 0;
        m_videoParam->mfx.FrameInfo.CropY   = 0;
        m_videoParam->mfx.FrameInfo.CropW   = m_width;
        m_videoParam->mfx.FrameInfo.CropH   = m_height;

        m_videoParam->mfx.TargetKbps        = m_bitRateKbps ? m_bitRateKbps : calcBitrate(m_width, m_height);
        m_videoParam->mfx.MaxKbps           = m_videoParam->mfx.TargetKbps;
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
        static const char dumpFile[] = "~/webrtc-msdk-dump.264";

        FILE *fp = fopen(dumpFile, "ab");
        if (fp) {
            fwrite(buf, 1, len, fp);
            fclose(fp);
            ELOG_DEBUG("Dump bitstream into %s", dumpFile);
        } else {
            ELOG_DEBUG("Dump failed, can not open file %s", dumpFile);
        }
    }

private:
    //format
    uint32_t m_frameCount;

    uint32_t m_width;
    uint32_t m_height;
    FrameFormat m_format;

    FrameDestination *m_dest;

    uint32_t m_bitRateKbps;

    bool m_setBitRateFlag;
    bool m_requestKeyFrameFlag;

    //encoder
    MFXVideoSession *m_session;
    MFXVideoENCODE *m_enc;

    boost::shared_ptr<mfxFrameAllocator> m_allocator;

    boost::scoped_ptr<mfxVideoParam> m_videoParam;

    std::vector<mfxExtBuffer *> m_extParams;
    boost::scoped_ptr<mfxExtCodingOption> m_extCodingOpt;
    boost::scoped_ptr<mfxExtCodingOption2> m_extCodingOpt2;

    boost::scoped_ptr<mfxBitstream> m_bitstream;
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

int32_t MsdkFrameEncoder::generateStream(uint32_t width, uint32_t height, woogeen_base::FrameDestination* dest)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);

    boost::shared_ptr<StreamEncoder> stream(new StreamEncoder());
    if (!stream->init(m_encodeFormat, width, height, dest)) {
        ELOG_ERROR("generateStream failed: {.width=%d, .height=%d}", width, height);
        return -1;
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);

    m_streams[m_id] = stream;

    ELOG_DEBUG("generateStream[%d]: {.width=%d, .height=%d}", m_id, width, height);

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
