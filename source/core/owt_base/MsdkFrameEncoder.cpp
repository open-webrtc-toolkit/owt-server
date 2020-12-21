// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_MSDK

#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "MediaUtilities.h"

#include "MsdkBase.h"
#include "MsdkFrameEncoder.h"
#include "MsdkScaler.h"
#include <JobTimer.h>

#define _MAX_BITSTREAM_BUFFER_ (100 * 1024 * 1024)

using namespace webrtc;

namespace owt_base {

DEFINE_LOGGER(MsdkFrameEncoder, "owt.MsdkFrameEncoder");

class StreamEncoder : public FrameSource, public JobTimerListener
{
    DECLARE_LOGGER()

    enum EncoderMode{ENCODER_MODE_NORMAL = 0, ENCODER_MODE_AUTO};

    typedef struct {
        boost::shared_ptr<mfxBitstream> bsBuffer;
        mfxSyncPoint syncp;
    } bsBufferSync_t;

    const uint8_t NumOfAsyncEnc = 3;

public:
    StreamEncoder()
        : m_mode(ENCODER_MODE_NORMAL)
        , m_ready(false)
        , m_frameCount(0)
        , m_format(FRAME_FORMAT_UNKNOWN)
        , m_profile(PROFILE_UNKNOWN)
        , m_width(0)
        , m_height(0)
        , m_bitRateKbps(0)
        , m_dest(NULL)
        , m_setBitRateFlag(false)
        , m_requestKeyFrameFlag(false)
        , m_encSession(NULL)
        , m_enc(NULL)
        , m_pluginID()
        , m_lastWidth(0)
        , m_lastHeight(0)
        , m_cropX(0)
        , m_cropY(0)
        , m_cropW(0)
        , m_cropH(0)
        , m_enableBsDump(false)
        , m_bsDumpfp(NULL)
    {
        initDefaultParam();
        m_keyFrameTimer.reset(new JobTimer(1, this));

        m_srv       = boost::make_shared<boost::asio::io_service>();
        m_srvWork   = boost::make_shared<boost::asio::io_service::work>(*m_srv);
        m_thread    = boost::make_shared<boost::thread>(boost::bind(&boost::asio::io_service::run, m_srv));
    }

    ~StreamEncoder()
    {
        m_keyFrameTimer->stop();

        m_srvWork.reset();
        m_srv->stop();
        m_thread.reset();
        m_srv.reset();

        removeVideoDestination(m_dest);

        if (m_enc) {
            m_enc->Close();
            delete m_enc;
            m_enc = NULL;
        }

        if (m_encSession) {
            MsdkBase *msdkBase = MsdkBase::get();
            if (msdkBase) {
                msdkBase->unLoadPlugin(m_encSession, &m_pluginID);
                msdkBase->destroySession(m_encSession);
            }
        }

        m_encAllocator.reset();

        m_encParam.reset();
        m_encExtCodingOpt.reset();
        m_encExtCodingOpt2.reset();
        m_encExtCodingOpt3.reset();
        m_encExtHevcParam.reset();
#if (MFX_VERSION >= 1025)
        m_encExtMultiFrameParam.reset();
        m_encExtMultiFrameControl.reset();
#endif
        m_encExtParams.clear();

        deinitBitstreamBuffers();
        deinitScaler();

        if (m_bsDumpfp) {
            fclose(m_bsDumpfp);
        }
    }

    void onFeedback(const owt_base::FeedbackMsg& msg) {
        if (msg.type == owt_base::VIDEO_FEEDBACK) {
            if (msg.cmd == REQUEST_KEY_FRAME) {
                requestKeyFrame();
            } else if (msg.cmd == SET_BITRATE) {
                setBitrate(msg.data.kbps);
            }
        }
    }

    void onTimeout() {
        if (m_format == FRAME_FORMAT_H265) {
            //TODO: remove this workaround for H265
            m_requestKeyFrameFlag = true;
        }
    }

    bool init(FrameFormat format, VideoCodecProfile profile, uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, FrameDestination* dest)
    {
        if (!dest) {
            ELOG_ERROR("(%p)Null FrameDestination.", this);
            return false;
        }

        m_mode = (width > 0 && height > 0) ? ENCODER_MODE_NORMAL : ENCODER_MODE_AUTO;
        m_format        = format;
        m_profile       = profile;
        m_width         = width;
        m_height        = height;
        m_frameRate     = frameRate > 0 ? frameRate : 30;
        m_bitRateKbps   = (m_mode == ENCODER_MODE_NORMAL) ? bitrateKbps : 0;
        m_keyFrameIntervalSeconds = keyFrameIntervalSeconds;
        m_dest          = dest;
        addVideoDestination(dest);

        updateParam();

        ELOG_DEBUG_T("Created encoder %s, profile(%d), resolution(%dx%d), frame rate(%d), kbps(%d), keyFrameIntervalSeconds(%d)"
                , getFormatStr(m_format), m_profile, m_width, m_height, m_frameRate, m_bitRateKbps, m_keyFrameIntervalSeconds);

        switch (m_format) {
            case FRAME_FORMAT_H264:

                switch (m_profile) {
                    case PROFILE_AVC_CONSTRAINED_BASELINE:
                    case PROFILE_AVC_BASELINE:
                    case PROFILE_AVC_MAIN:
                    case PROFILE_AVC_HIGH:
                        break;

                    case PROFILE_UNKNOWN:
                    default:
                        ELOG_WARN_T("Unspported video profile(%d), force to Main", m_profile);

                        m_profile = PROFILE_AVC_MAIN;
                        break;
                }

                m_encParam->mfx.CodecId         = MFX_CODEC_AVC;
                m_encParam->mfx.CodecProfile    = m_profile;
                break;
            case FRAME_FORMAT_H265:
                if (m_profile != PROFILE_UNKNOWN) {
                    ELOG_WARN_T("Don't support profile setting(%d)", m_profile);
                }

                m_encParam->mfx.CodecId         = MFX_CODEC_HEVC;
                break;
            default:
                ELOG_ERROR("(%p)Unspported video frame format %s(%d)", this, getFormatStr(m_format), m_format);
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

        if (!msdkBase->loadEncoderPlugin(m_encParam->mfx.CodecId, m_encSession, &m_pluginID)) {
            ELOG_ERROR("(%p)Load plugin failed.", this);
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

        int size = (m_width != 0 && m_height != 0) ?
            m_width * m_height * 3 / 2 :
            1 * 1024 * 1024;

        ELOG_TRACE("(%p)Init bistream buffer, size(%d)", this, size);

        initBitstreamBuffers(size);

        resetEnc();

        if (m_enableBsDump) {
            char dumpFileName[128];

            snprintf(dumpFileName, 128, "/tmp/msdkFrameEncoder-%p.%s", this, getFormatStr(m_format));
            m_bsDumpfp = fopen(dumpFileName, "wb");
            if (m_bsDumpfp) {
                ELOG_DEBUG("Enable bitstream dump, %s", dumpFileName);
            } else {
                ELOG_DEBUG("Can not open dump file, %s", dumpFileName);
            }
        }

        return true;
    }

    void resetEnc()
    {
        mfxStatus sts = MFX_ERR_NONE;

        if (!isValidParam()) {
            m_ready = false;
            return;
        }

        sts = m_enc->Reset(m_encParam.get());
        if (sts > 0) {
            ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
        } else if (sts != MFX_ERR_NONE) {
            ELOG_TRACE("(%p)mfx reset failed, ret %d. Try to reinitialize.", this, sts);

            m_enc->Close();
            sts = m_enc->Init(m_encParam.get());
            if (sts > 0) {
                ELOG_TRACE("(%p)Ignore mfx warning, ret %d", this, sts);
            } else if (sts != MFX_ERR_NONE) {
                MsdkBase::printfVideoParam(m_encParam.get(), MFX_ENC);
                ELOG_ERROR("(%p)mfx init failed, ret %d", this, sts);

                m_ready = false;
                return;
            }
        }

        m_enc->GetVideoParam(m_encParam.get());
        MsdkBase::printfVideoParam(m_encParam.get(), MFX_ENC);

        m_ready = true;
    }

    bool isKeyFrame(uint32_t FrameType)
    {
        return (FrameType & MFX_FRAMETYPE_IDR) || (FrameType & MFX_FRAMETYPE_xIDR);
    }

    void onFrame(const owt_base::Frame& frame)
    {
        mfxStatus sts = MFX_ERR_NONE;
        mfxSyncPoint syncp;
        boost::scoped_ptr<mfxEncodeCtrl> ctrl;

        boost::shared_ptr<mfxBitstream> bsBuffer = getBitstreamBuffer();
        if (!bsBuffer) {
            ELOG_INFO_T("Drop frame, no bitstream buffer available");
            return;
        }

        if (m_mode == ENCODER_MODE_AUTO) {
            if(m_width != frame.additionalInfo.video.width || m_height != frame.additionalInfo.video.height) {
                ELOG_DEBUG("(%p)Encoder resolution changed, %dx%d -> %dx%d", this
                        , m_width, m_height
                        , frame.additionalInfo.video.width, frame.additionalInfo.video.height
                        );

                m_width = frame.additionalInfo.video.width;
                m_height = frame.additionalInfo.video.height;

                updateParam();
                resetEnc();
            }
        }

        if (!m_ready) {
            ELOG_DEBUG("(%p)Encoder not ready!", this);
            return;
        }

        boost::shared_ptr<MsdkFrame> inFrame = convert(frame);

        if (m_setBitRateFlag) {
            ELOG_DEBUG("(%p)Do set bitrate!", this);

            updateParam();
            resetEnc();
            m_setBitRateFlag = false;
        }

        if (m_requestKeyFrameFlag) {
            ELOG_DEBUG("(%p)Do requeset key frame!", this);

            ctrl.reset(new mfxEncodeCtrl);

            memset(ctrl.get(), 0, sizeof(mfxEncodeCtrl));
            ctrl->FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;

            m_requestKeyFrameFlag = false;
        }

retry:
        sts = m_enc->EncodeFrameAsync(ctrl.get(), inFrame->getSurface(), bsBuffer.get(), &syncp);
        if (sts == MFX_WRN_DEVICE_BUSY) {
            ELOG_TRACE("(%p)Device busy, retry!", this);

            usleep(1000); //1ms
            goto retry;
        }
        else if (sts == MFX_ERR_NOT_ENOUGH_BUFFER) {
            ELOG_WARN("(%p)Require more bitstream buffer, %d!", this, bsBuffer->MaxLength);

            uint32_t newSize = bsBuffer->MaxLength * 2;
            while (newSize < bsBuffer->DataLength + frame.length)
                newSize *= 2;

            if (newSize > _MAX_BITSTREAM_BUFFER_) {
                ELOG_ERROR_T("Exceed max bitstream buffer size(%d), %d!",  _MAX_BITSTREAM_BUFFER_, newSize);
                assert(0);
            }

            ELOG_DEBUG_T("bitstream buffer need to remalloc %d -> %d"
                    , bsBuffer->MaxLength
                    , newSize
                    );

            bsBuffer->Data         = (mfxU8 *)realloc(bsBuffer->Data, newSize);
            bsBuffer->MaxLength    = newSize;

            goto retry;
        }
        else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)mfx encode error, ret %d", this, sts);
            return;
        }

        boost::shared_ptr<bsBufferSync_t> bsBufferSync(new bsBufferSync_t);
        bsBufferSync->bsBuffer = bsBuffer;
        bsBufferSync->syncp = syncp;

        m_srv->post(boost::bind(&StreamEncoder::SyncFrame, this, bsBufferSync));
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
    boost::shared_ptr<MsdkFrame> convert(const owt_base::Frame& frame)
    {
        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
        boost::shared_ptr<MsdkFrame> msdkFrame = holder->frame;

        if (m_mode == ENCODER_MODE_AUTO)
            return msdkFrame;

        if(m_width != frame.additionalInfo.video.width || m_height != frame.additionalInfo.video.height) {
            if (!m_scaler) {
                initScaler();
            }

            msdkFrame = doScale(msdkFrame);
        }

        return msdkFrame;
    }

    void initDefaultParam(void)
    {
        m_encParam.reset(new mfxVideoParam);
        memset(m_encParam.get(), 0, sizeof(mfxVideoParam));

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

        m_encParam->mfx.FrameInfo.Width           = ALIGN16(0);
        m_encParam->mfx.FrameInfo.Height          = ALIGN16(0);
        m_encParam->mfx.FrameInfo.CropX           = 0;
        m_encParam->mfx.FrameInfo.CropY           = 0;
        m_encParam->mfx.FrameInfo.CropW           = 0;
        m_encParam->mfx.FrameInfo.CropH           = 0;

        // mfxVideoParam Common
        m_encParam->AsyncDepth                    = 1;
        m_encParam->IOPattern                     = MFX_IOPATTERN_IN_VIDEO_MEMORY;

        // mfx
        m_encParam->mfx.LowPower                  = 0;
        m_encParam->mfx.BRCParamMultiplier        = 0;
        m_encParam->mfx.CodecLevel                = 0;
        m_encParam->mfx.NumThread                 = 0;

        // mfx Enc
        m_encParam->mfx.TargetUsage               = MFX_TARGETUSAGE_BALANCED; // MFX_TARGETUSAGE_4
        m_encParam->mfx.GopPicSize                = 3000;
        m_encParam->mfx.GopRefDist                = 1; // Workaround for Windows Chrome 4K H264 Decoding
        m_encParam->mfx.GopOptFlag                = 0;
        m_encParam->mfx.IdrInterval               = 0;

        m_encParam->mfx.RateControlMethod         = MFX_RATECONTROL_VBR;

        m_encParam->mfx.BufferSizeInKB            = 0;
        m_encParam->mfx.InitialDelayInKB          = 0;
        m_encParam->mfx.TargetKbps                = calcBitrate(m_encParam->mfx.FrameInfo.Width, m_encParam->mfx.FrameInfo.Height);
        m_encParam->mfx.MaxKbps                   = m_encParam->mfx.TargetKbps;

        m_encParam->mfx.NumSlice                  = 0;
        m_encParam->mfx.NumRefFrame               = 2;
        m_encParam->mfx.EncodedOrder              = 0;

        // MFX_EXTBUFF_CODING_OPTION
        m_encExtCodingOpt.reset(new mfxExtCodingOption);
        memset(m_encExtCodingOpt.get(), 0, sizeof(mfxExtCodingOption));

        m_encExtCodingOpt->Header.BufferId             = MFX_EXTBUFF_CODING_OPTION;
        m_encExtCodingOpt->Header.BufferSz             = sizeof(*m_encExtCodingOpt);
        m_encExtCodingOpt->AUDelimiter                 = MFX_CODINGOPTION_OFF;
        m_encExtCodingOpt->PicTimingSEI                = MFX_CODINGOPTION_OFF;
        m_encExtCodingOpt->VuiNalHrdParameters         = MFX_CODINGOPTION_OFF;

        m_encExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_encExtCodingOpt.get()));

        // MFX_EXTBUFF_CODING_OPTION2
        m_encExtCodingOpt2.reset(new mfxExtCodingOption2);
        memset(m_encExtCodingOpt2.get(), 0, sizeof(mfxExtCodingOption2));

        m_encExtCodingOpt2->Header.BufferId            = MFX_EXTBUFF_CODING_OPTION2;
        m_encExtCodingOpt2->Header.BufferSz            = sizeof(*m_encExtCodingOpt2);
        m_encExtCodingOpt2->RepeatPPS                  = MFX_CODINGOPTION_OFF;
        m_encExtCodingOpt2->ExtBRC                     = MFX_CODINGOPTION_ADAPTIVE;

        m_encExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_encExtCodingOpt2.get()));

#if (MFX_VERSION >= 1026)
        // MFX_EXTBUFF_CODING_OPTION3
        m_encExtCodingOpt3.reset(new mfxExtCodingOption3);
        memset(m_encExtCodingOpt3.get(), 0, sizeof(mfxExtCodingOption3));

        m_encExtCodingOpt3->Header.BufferId            = MFX_EXTBUFF_CODING_OPTION3;
        m_encExtCodingOpt3->Header.BufferSz            = sizeof(*m_encExtCodingOpt3);
        m_encExtCodingOpt3->ExtBrcAdaptiveLTR          = MFX_CODINGOPTION_ON;

        m_encExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_encExtCodingOpt3.get()));
#endif

#if (MFX_VERSION >= 1025)
        uint32_t MFE_timeout = 0;
        MsdkBase *msdkBase = MsdkBase::get();
        if (msdkBase)
            MFE_timeout = msdkBase->getConfigMFETimeout();
        if (MFE_timeout) {
            // MFX_EXTBUFF_MULTI_FRAME_PARAM
            m_encExtMultiFrameParam.reset(new mfxExtMultiFrameParam);
            memset(m_encExtMultiFrameParam.get(), 0, sizeof(mfxExtMultiFrameParam));

            m_encExtMultiFrameParam->Header.BufferId    = MFX_EXTBUFF_MULTI_FRAME_PARAM;
            m_encExtMultiFrameParam->Header.BufferSz    = sizeof(*m_encExtMultiFrameParam);
            m_encExtMultiFrameParam->MFMode             = MFX_MF_AUTO;
            m_encExtMultiFrameParam->MaxNumFrames       = 0;

            m_encExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_encExtMultiFrameParam.get()));

            // MFX_EXTBUFF_MULTI_FRAME_CONTROL
            m_encExtMultiFrameControl.reset(new mfxExtMultiFrameControl);
            memset(m_encExtMultiFrameControl.get(), 0, sizeof(mfxExtMultiFrameControl));

            m_encExtMultiFrameControl->Header.BufferId  = MFX_EXTBUFF_MULTI_FRAME_CONTROL;
            m_encExtMultiFrameControl->Header.BufferSz  = sizeof(*m_encExtMultiFrameControl);
            m_encExtMultiFrameControl->Timeout          = MFE_timeout;

            m_encExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_encExtMultiFrameControl.get()));
        }
#endif

        m_encParam->ExtParam                    = &m_encExtParams.front(); // vector is stored linearly in memory
        m_encParam->NumExtParam                 = m_encExtParams.size();
    }

    void updateParam()
    {
        m_encParam->mfx.FrameInfo.FrameRateExtN   = m_frameRate;
        m_encParam->mfx.FrameInfo.FrameRateExtD   = 1;

        m_encParam->mfx.FrameInfo.CropX   = 0;
        m_encParam->mfx.FrameInfo.CropY   = 0;
        m_encParam->mfx.FrameInfo.CropW   = m_width;
        m_encParam->mfx.FrameInfo.CropH   = m_height;

        m_encParam->mfx.FrameInfo.Width   = ALIGN16(m_width);
        m_encParam->mfx.FrameInfo.Height  = ALIGN16(m_height);

        if (m_format == FRAME_FORMAT_H265) {
            if ((!((m_encParam->mfx.FrameInfo.CropW & 15) ^ 8)) ||
                    (!((m_encParam->mfx.FrameInfo.CropH & 15) ^ 8))) {
                m_encExtHevcParam.reset(new mfxExtHEVCParam);
                memset(m_encExtHevcParam.get(), 0, sizeof(mfxExtHEVCParam));

                m_encExtHevcParam->Header.BufferId          = MFX_EXTBUFF_HEVC_PARAM;
                m_encExtHevcParam->Header.BufferSz          = sizeof(m_encExtHevcParam);
                m_encExtHevcParam->PicWidthInLumaSamples    = m_encParam->mfx.FrameInfo.CropW;
                m_encExtHevcParam->PicHeightInLumaSamples   = m_encParam->mfx.FrameInfo.CropH;

                m_encExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(m_encExtHevcParam.get()));
            }
        }

        m_encParam->mfx.TargetKbps        = m_bitRateKbps ? m_bitRateKbps : calcBitrate(m_width, m_height);
        m_encParam->mfx.MaxKbps           = m_encParam->mfx.TargetKbps;
        m_encParam->mfx.GopPicSize        = m_frameRate * m_keyFrameIntervalSeconds;

        m_encParam->ExtParam                    = &m_encExtParams.front(); // vector is stored linearly in memory
        m_encParam->NumExtParam                 = m_encExtParams.size();
    }

    bool isValidParam()
    {
        return m_encParam->mfx.FrameInfo.Width > 0
            && m_encParam->mfx.FrameInfo.Height > 0;
    }

    void dump(uint8_t *buf, int len)
    {
        if (m_bsDumpfp) {
            fwrite(buf, 1, len, m_bsDumpfp);
        }
    }

    bool initScaler()
    {
        MsdkBase *msdkBase = MsdkBase::get();
        if(msdkBase == NULL) {
            ELOG_ERROR("(%p)Get MSDK failed.", this);
            return false;
        }

        m_vppAllocator = msdkBase->createFrameAllocator();
        if (!m_vppAllocator) {
            ELOG_ERROR("(%p)Create frame allocator failed.", this);
            return false;
        }

        m_vppFramePool.reset(new MsdkFramePool(m_width, m_height, NumOfAsyncEnc, m_vppAllocator));
        m_scaler.reset(new MsdkScaler());
        return true;
    }

    void deinitScaler()
    {
        m_vppAllocator.reset();
        m_vppFramePool.reset();
        m_scaler.reset();
    }

    boost::shared_ptr<MsdkFrame> doScale(boost::shared_ptr<MsdkFrame> src)
    {
        boost::shared_ptr<MsdkFrame> dst = m_vppFramePool->getFreeFrame();
        if (!dst) {
            ELOG_WARN("(%p)No frame available", this);
            return NULL;
        }

        if (m_lastWidth != src->getVideoWidth() || m_lastHeight != src->getVideoHeight()) {
            m_cropW = std::min(m_width, src->getVideoWidth() * m_height / src->getVideoHeight());
            m_cropH = std::min(m_height, src->getVideoHeight()* m_width / src->getVideoWidth());

            m_cropX = (m_width - m_cropW) / 2;
            m_cropY = (m_height - m_cropH) / 2;

            m_lastWidth = src->getVideoWidth();
            m_lastHeight= src->getVideoHeight();
        }

        if (!m_scaler || !m_scaler->convert(src.get(), 0, 0, src->getCropW(), src->getCropH(), dst.get(), m_cropX, m_cropY, m_cropW, m_cropH))
            return NULL;

        return dst;
    }

    void initBitstreamBuffers(uint32_t bufferSize)
    {
        m_bitstreamBuffers.resize(NumOfAsyncEnc);
        for (auto& bsBuffer : m_bitstreamBuffers) {
            bsBuffer.reset(new mfxBitstream);
            memset((void *)bsBuffer.get(), 0, sizeof(mfxBitstream));

            bsBuffer->Data         = (mfxU8 *)malloc(bufferSize);
            bsBuffer->MaxLength    = bufferSize;
            bsBuffer->DataOffset   = 0;
            bsBuffer->DataLength   = 0;
        }
    }

    void deinitBitstreamBuffers()
    {
        for (auto& bsBuffer : m_bitstreamBuffers) {
            if (bsBuffer && bsBuffer->Data) {
                free(bsBuffer->Data);
                bsBuffer->Data = NULL;
            }
        }
        m_bitstreamBuffers.clear();
    }

    boost::shared_ptr<mfxBitstream> getBitstreamBuffer()
    {
        for (auto& bsBuffer : m_bitstreamBuffers) {
            if (bsBuffer && bsBuffer.use_count() == 1) {
                return bsBuffer;
            }
        }
        return NULL;
    }

    static void SyncFrame(StreamEncoder *This, boost::shared_ptr<bsBufferSync_t> bsBufferSync)
    {
        This->syncFrame(bsBufferSync);
    }

    void syncFrame(boost::shared_ptr<bsBufferSync_t> bsBufferSync)
    {
        mfxStatus sts = MFX_ERR_NONE;
        mfxSyncPoint syncp = bsBufferSync->syncp;
        boost::shared_ptr<mfxBitstream> bsBuffer = bsBufferSync->bsBuffer;

        sts = m_encSession->SyncOperation(syncp, MFX_INFINITE);
        if(sts != MFX_ERR_NONE) {
            ELOG_ERROR("(%p)SyncOperation failed, ret %d", this, sts);
            return;
        }

        if (bsBuffer->DataLength <= 0) {
            ELOG_ERROR("(%p)No output bitstream buffer", this);
            return;
        }

        ELOG_TRACE("(%p)Output FrameType 0x%x, DataOffset %d , DataLength %d", this,
                bsBuffer->FrameType,
                bsBuffer->DataOffset,
                bsBuffer->DataLength
                );

        Frame outFrame;
        memset(&outFrame, 0, sizeof(outFrame));
        outFrame.format = m_format;
        outFrame.payload = bsBuffer->Data + bsBuffer->DataOffset;
        outFrame.length = bsBuffer->DataLength;
        outFrame.additionalInfo.video.width = m_width;
        outFrame.additionalInfo.video.height = m_height;
        outFrame.additionalInfo.video.isKeyFrame = isKeyFrame(bsBuffer->FrameType);
        outFrame.timeStamp = (m_frameCount++) * 1000 / m_frameRate * 90;

        ELOG_TRACE_T("deliverFrame, %s, %dx%d(%s), length(%d)",
                getFormatStr(outFrame.format),
                outFrame.additionalInfo.video.width,
                outFrame.additionalInfo.video.height,
                outFrame.additionalInfo.video.isKeyFrame ? "key" : "delta",
                outFrame.length);

        dump(outFrame.payload, outFrame.length);

        deliverFrame(outFrame);

        bsBuffer->DataOffset   = 0;
        bsBuffer->DataLength   = 0;
    }

private:
    enum EncoderMode m_mode;
    bool m_ready;
    //format
    uint32_t m_frameCount;

    FrameFormat m_format;
    VideoCodecProfile m_profile;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_frameRate;
    uint32_t m_bitRateKbps;
    uint32_t m_keyFrameIntervalSeconds;
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
    boost::scoped_ptr<mfxExtCodingOption3> m_encExtCodingOpt3;
    boost::scoped_ptr<mfxExtHEVCParam> m_encExtHevcParam;

#if (MFX_VERSION >= 1025)
    boost::scoped_ptr<mfxExtMultiFrameParam> m_encExtMultiFrameParam;
    boost::scoped_ptr<mfxExtMultiFrameControl> m_encExtMultiFrameControl;
#endif

    std::vector<boost::shared_ptr<mfxBitstream>> m_bitstreamBuffers;
    mfxPluginUID m_pluginID;

    //scaler
    boost::shared_ptr<mfxFrameAllocator> m_vppAllocator;
    boost::scoped_ptr<MsdkFramePool> m_vppFramePool;
    boost::scoped_ptr<MsdkScaler> m_scaler;
    uint32_t m_lastWidth;
    uint32_t m_lastHeight;
    uint32_t m_cropX;
    uint32_t m_cropY;
    uint32_t m_cropW;
    uint32_t m_cropH;

    bool m_enableBsDump;
    FILE *m_bsDumpfp;

    boost::scoped_ptr<JobTimer> m_keyFrameTimer;

    boost::shared_ptr<boost::asio::io_service> m_srv;
    boost::shared_ptr<boost::asio::io_service::work> m_srvWork;
    boost::shared_ptr<boost::thread> m_thread;
};

DEFINE_LOGGER(StreamEncoder, "owt.StreamEncoder");

MsdkFrameEncoder::MsdkFrameEncoder(owt_base::FrameFormat format, owt_base::VideoCodecProfile profile, bool useSimulcast)
    : m_encodeFormat(format)
    , m_profile(profile)
    , m_useSimulcast(useSimulcast)
    , m_id(0)
{
}

MsdkFrameEncoder::~MsdkFrameEncoder()
{
    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

    m_streams.clear();
}

bool MsdkFrameEncoder::supportFormat(FrameFormat format)
{
    mfxU32 codecId = 0;
    switch (format) {
        case FRAME_FORMAT_H264:
            codecId = MFX_CODEC_AVC;
            break;
        case FRAME_FORMAT_H265:
            codecId = MFX_CODEC_HEVC;
            break;
        case FRAME_FORMAT_VP8:
            codecId = MFX_CODEC_VP8;
            break;
        default:
            return false;
    }

    MsdkBase *msdkBase = MsdkBase::get();
    if (!msdkBase)
        return false;

    return msdkBase->isSupportedEncoder(codecId);
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

int32_t MsdkFrameEncoder::generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, owt_base::FrameDestination* dest)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);

    boost::shared_ptr<StreamEncoder> stream(new StreamEncoder());
    if (!stream->init(m_encodeFormat, m_profile,  width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds, dest)) {
        return -1;
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);

    m_streams[m_id] = stream;

    ELOG_DEBUG("generateStream[%d]" , m_id);

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
            for (auto it = m_streams.begin(); it != m_streams.end(); ++it) {
                it->second->onFrame(frame);
            }
            break;
        default:
            ELOG_ERROR("frame format %d(%s) not supported", frame.format, getFormatStr(frame.format));
            assert(false);
            break;
    }
}

} // namespace owt_base
#endif /* ENABLE_MSDK */
