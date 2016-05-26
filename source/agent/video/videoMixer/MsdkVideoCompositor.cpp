/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
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

#include <webrtc/system_wrappers/interface/clock.h>
#include <webrtc/system_wrappers/interface/tick_util.h>
#include <deque>

#include "MsdkBase.h"
#include "MsdkVideoCompositor.h"

using namespace webrtc;

namespace mcu {

class VppInput {
    DECLARE_LOGGER();

public:
    VppInput()
        : m_active(false)
    {
        memset(&m_rootSize, 0, sizeof(m_rootSize));
        memset(&m_vppRect, 0, sizeof(m_vppRect));
    }

    ~VppInput()
    {
        printfFuncEnter;

        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        m_queue.clear();

        printfFuncExit;
    }

    void updateRootSize(VideoSize& videoSize)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        m_rootSize = videoSize;
        updateRect();
    }

    void updateInputRegion(Region& region)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        m_region = region;
        updateRect();
    }

    void activate()
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        m_active = true;
    }

    void deActivate()
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        m_active = false;
        m_queue.clear();
    }

    void pushInput(const woogeen_base::Frame& frame)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        m_queue.push_back(convert(frame));
        if (m_queue.size() > MAX_DECODED_FRAME_IN_RENDERING) {
            ELOG_TRACE("(%p)Reach max frames in queue, drop oldest frame!", this);
            m_queue.pop_front();
        }
    }

    boost::shared_ptr<MsdkFrame> popInput()
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        boost::shared_ptr<MsdkFrame> input = NULL;
        if (!m_active)
            return NULL;

        if (m_queue.empty())
            return NULL;

        if (m_queue.size() == 1) {
            ELOG_TRACE("(%p)Repeated frame!", this);
        }
        else {
            // Keep at least one frame for renderer, postpone pop to next opt
            m_queue.pop_front();
        }

        input = m_queue.front();
        return input;
    }

    mfxVPPCompInputStream *getVppRect()
    {
        boost::shared_lock<boost::shared_mutex> lock(m_mutex);

        ELOG_TRACE("(%p)Rect: %d-%d-%d-%d", this, m_vppRect.DstX, m_vppRect.DstY, m_vppRect.DstW, m_vppRect.DstH);

        return &m_vppRect;
    }

protected:
    boost::shared_ptr<MsdkFrame> convert(const woogeen_base::Frame& frame)
    {
        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
        return holder->frame;
    }

    void updateRect()
    {
        const Region& region = m_region;
        const VideoSize& rootSize = m_rootSize;
        assert(!(region.relativeSize < 0.0 || region.relativeSize > 1.0)
            && !(region.left < 0.0 || region.left > 1.0)
            && !(region.top < 0.0 || region.top > 1.0));

        unsigned int sub_width = (unsigned int)(rootSize.width * region.relativeSize);
        unsigned int sub_height = (unsigned int)(rootSize.height * region.relativeSize);
        unsigned int offset_width = (unsigned int)(rootSize.width * region.left);
        unsigned int offset_height = (unsigned int)(rootSize.height * region.top);
        if (offset_width + sub_width > rootSize.width)
            sub_width = rootSize.width - offset_width;

        if (offset_height + sub_height > rootSize.height)
            sub_height = rootSize.height - offset_height;

        m_vppRect.DstX = offset_width;
        m_vppRect.DstY = offset_height;
        m_vppRect.DstW = sub_width;
        m_vppRect.DstH = sub_height;
    }

private:
    bool m_active;

    VideoSize m_rootSize;
    Region m_region;

    mfxVPPCompInputStream m_vppRect;

    std::deque<boost::shared_ptr<MsdkFrame>> m_queue;

    boost::shared_mutex m_mutex;
};

DEFINE_LOGGER(VppInput, "mcu.media.VppInput");

DEFINE_LOGGER(MsdkVideoCompositor, "mcu.media.MsdkVideoCompositor");

MsdkVideoCompositor::MsdkVideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, bool crop)
    : m_maxInput(maxInput)
    , m_crop(crop)
    , m_compositeSize(rootSize)
    , m_bgColor(bgColor)
    , m_solutionState(UN_INITIALIZED)
    , m_videoParam(NULL)
    , m_extVppComp(NULL)
    , m_session(NULL)
    , m_allocator(NULL)
    , m_vpp(NULL)
    , m_framePool(NULL)
    , m_defaultInputFramePool(NULL)
{
    ELOG_DEBUG("set size to %dx%d, maxInput = %d", rootSize.width, rootSize.height, maxInput);

    m_ntpDelta = Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() - TickTime::MillisecondTimestamp();

    initDefaultParam();
    updateParam();

    init();

    m_inputs.resize(maxInput);
    for (auto& input : m_inputs) {
        input.reset(new VppInput());
        input->updateRootSize(rootSize);
    }

    m_jobTimer.reset(new woogeen_base::JobTimer(30, this));
    m_jobTimer->start();
}

MsdkVideoCompositor::~MsdkVideoCompositor()
{
    printfFuncEnter;

    m_jobTimer->stop();

    if (m_vpp) {
        m_vpp->Close();
        delete m_vpp;
        m_vpp= NULL;
    }

    if (m_session) {
        //disjoint
        m_session->Close();
        delete m_session;
        m_session = NULL;
    }

    m_allocator.reset();

    m_videoParam.reset();
    m_extVppComp.reset();
    m_compInputStreams.clear();

    m_inputs.clear();

    m_defaultInputFrame.reset();
    m_defaultInputFramePool.reset();

    m_framePool.reset();

    printfFuncExit;
}

void MsdkVideoCompositor::initDefaultParam(void)
{
    m_videoParam.reset(new mfxVideoParam);
    memset(m_videoParam.get(), 0, sizeof(mfxVideoParam));

    m_extVppComp.reset(new mfxExtVPPComposite);
    memset(m_extVppComp.get(), 0, sizeof(mfxExtVPPComposite));

    // mfxVideoParam Common
    m_videoParam->AsyncDepth = 1;
    m_videoParam->IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    m_videoParam->NumExtParam = 1;
    m_videoParam->ExtParam = (mfxExtBuffer **)&m_extVppComp;

    // mfxVideoParam Vpp In
    m_videoParam->vpp.In.FourCC             = MFX_FOURCC_NV12;
    m_videoParam->vpp.In.ChromaFormat       = MFX_CHROMAFORMAT_YUV420;
    m_videoParam->vpp.In.PicStruct          = MFX_PICSTRUCT_PROGRESSIVE;

    m_videoParam->vpp.In.BitDepthLuma       = 0;
    m_videoParam->vpp.In.BitDepthChroma     = 0;
    m_videoParam->vpp.In.Shift              = 0;

    m_videoParam->vpp.In.AspectRatioW       = 0;
    m_videoParam->vpp.In.AspectRatioH       = 0;

    m_videoParam->vpp.In.FrameRateExtN      = 60;
    m_videoParam->vpp.In.FrameRateExtD      = 2;

    // mfxVideoParam Vpp Out
    m_videoParam->vpp.Out.FourCC            = MFX_FOURCC_NV12;
    m_videoParam->vpp.Out.ChromaFormat      = MFX_CHROMAFORMAT_YUV420;
    m_videoParam->vpp.Out.PicStruct         = MFX_PICSTRUCT_PROGRESSIVE;

    m_videoParam->vpp.Out.BitDepthLuma      = 0;
    m_videoParam->vpp.Out.BitDepthChroma    = 0;
    m_videoParam->vpp.Out.Shift             = 0;

    m_videoParam->vpp.Out.AspectRatioW      = 0;
    m_videoParam->vpp.Out.AspectRatioH      = 0;

    m_videoParam->vpp.Out.FrameRateExtN     = 60;
    m_videoParam->vpp.Out.FrameRateExtD     = 2;

    // mfxExtVPPComposite
    m_extVppComp->Header.BufferId           = MFX_EXTBUFF_VPP_COMPOSITE;
    m_extVppComp->Header.BufferSz           = sizeof(mfxExtVPPComposite);
    m_extVppComp->Y = 0;
    m_extVppComp->U = 0;
    m_extVppComp->V = 0;
}

void MsdkVideoCompositor::updateParam(void)
{
    m_videoParam->vpp.In.Width      = m_compositeSize.width;
    m_videoParam->vpp.In.Height     = m_compositeSize.height;
    m_videoParam->vpp.In.CropX      = 0;
    m_videoParam->vpp.In.CropY      = 0;
    m_videoParam->vpp.In.CropW      = m_compositeSize.width;
    m_videoParam->vpp.In.CropH      = m_compositeSize.height;

    m_videoParam->vpp.Out.Width     = m_compositeSize.width;
    m_videoParam->vpp.Out.Height    = m_compositeSize.height;
    m_videoParam->vpp.Out.CropX     = 0;
    m_videoParam->vpp.Out.CropY     = 0;
    m_videoParam->vpp.Out.CropW     = m_compositeSize.width;
    m_videoParam->vpp.Out.CropH     = m_compositeSize.height;

    m_extVppComp->Y                 = m_bgColor.y;
    m_extVppComp->U                 = m_bgColor.cb;
    m_extVppComp->V                 = m_bgColor.cr;
}

void MsdkVideoCompositor::fillFrame(mfxFrameSurface1 *pSurface, uint8_t y, uint8_t u, uint8_t v) {
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameInfo& pInfo = pSurface->Info;
    mfxFrameData& pData = pSurface->Data;

    // supports only NV12 mfx surfaces for code transparency,
    // other formats may be added if application requires such functionality
    if (MFX_FOURCC_NV12 != pInfo.FourCC) {
        ELOG_ERROR("Format (%c%c%c%c) is not upported!",
                pInfo.FourCC & 0xff,
                (pInfo.FourCC >> 8) & 0xff,
                (pInfo.FourCC >> 16) & 0xff,
                (pInfo.FourCC >> 24) & 0xff
                );

        return;
    }

    sts = m_allocator->Lock(m_allocator.get()->pthis, pData.MemId, &pData);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Failed to lock surface!");

        return;
    }

    mfxU16 w, h, i, pitch;
    mfxU8 *ptr;

    if (pInfo.CropH > 0 && pInfo.CropW > 0)
    {
        w = pInfo.CropW;
        h = pInfo.CropH;
    }
    else
    {
        w = pInfo.Width;
        h = pInfo.Height;
    }

    pitch = pData.Pitch;
    ptr = pData.Y + pInfo.CropX + pInfo.CropY * pData.Pitch;

    // read luminance plane
    for(i = 0; i < h; i++)
    {
        ptr[i] = y;
    }

    // read chroma planes
    switch (pSurface->Info.FourCC) // color format of data in the input file
    {
        case MFX_FOURCC_NV12:
            mfxU32 j;
            w /= 2;
            h /= 2;
            ptr = pData.UV + pInfo.CropX + (pInfo.CropY / 2) * pitch;

            // load U
            for (i = 0; i < h; i++)
            {
                for (j = 0; j < w; j++)
                {
                    ptr[i * pitch + j * 2] = u;
                }
            }
            // load V
            for (i = 0; i < h; i++)
            {
                for (j = 0; j < w; j++)
                {
                    ptr[i * pitch + j * 2 + 1] = v;
                }
            }

            break;

        default:

            ELOG_ERROR("Format (%c%c%c%c) is not upported!",
                    pInfo.FourCC & 0xff,
                    (pInfo.FourCC >> 8) & 0xff,
                    (pInfo.FourCC >> 16) & 0xff,
                    (pInfo.FourCC >> 24) & 0xff
                    );
            break;
    }

    sts = m_allocator->Unlock(m_allocator.get()->pthis, pData.MemId, &pData);
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Failed to unlock surface!");

        return;
    }
}

void MsdkVideoCompositor::init(void)
{
    mfxStatus sts = MFX_ERR_NONE;

    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase == NULL) {
        ELOG_ERROR("Get MSDK failed.");
        return;
    }

    m_session = msdkBase->createSession();
    if (!m_session ) {
        ELOG_ERROR("Create session failed.");
        return;
    }

    m_allocator = msdkBase->createFrameAllocator();
    if (!m_allocator) {
        ELOG_ERROR("Create frame allocator failed.");
        return;
    }

    sts = m_session->SetFrameAllocator(m_allocator.get());
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("Set frame allocator failed.");
        return;
    }

    m_vpp = new MFXVideoVPP(*m_session);
    if (!m_vpp) {
        ELOG_ERROR("Create vpp failed.");

        return;
    }

    sts = m_vpp->Init(m_videoParam.get());
    if (sts > 0) {
        ELOG_TRACE("Ignore mfx warning, ret %d", sts);
    }
    else if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("mfx int failed, ret %d", sts);
        return;
    }

    mfxFrameAllocRequest Request[2];
    memset(&Request, 0, sizeof(mfxFrameAllocRequest) * 2);

    sts = m_vpp->QueryIOSurf(m_videoParam.get(), Request);
    if (MFX_WRN_PARTIAL_ACCELERATION == sts || MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts)
    {
        ELOG_TRACE("Ignore warning!");
    }
    if (MFX_ERR_NONE != sts)
    {
        ELOG_ERROR("mfx QueryIOSurf() failed, ret %d", sts);
        return;
    }

    ELOG_TRACE("mfx QueryIOSurf: In(%d), Out(%d)", Request[0].NumFrameSuggested, Request[1].NumFrameSuggested);

    // after reset, dont need realloc frame pool
    if (!m_framePool) {
        m_framePool.reset(new MsdkFramePool(m_allocator, Request[1]));
        if (!m_framePool->init()) {
            ELOG_ERROR("Frame pool init failed, ret %d", sts);

            m_framePool.reset();
            return;
        }
    }

    // after reset, dont need realloc frame pool
    if (!m_defaultInputFramePool) {
        mfxFrameAllocRequest defaultInputRequest = Request[0];

        defaultInputRequest.NumFrameMin         = 1;
        defaultInputRequest.NumFrameSuggested   = 1;
        defaultInputRequest.Info.Width          = 10;
        defaultInputRequest.Info.Height         = 10;
        defaultInputRequest.Info.CropX          = 0;
        defaultInputRequest.Info.CropY          = 0;
        defaultInputRequest.Info.CropW          = 10;
        defaultInputRequest.Info.CropH          = 10;

        m_defaultInputFramePool.reset(new MsdkFramePool(m_allocator, defaultInputRequest));
        if (!m_defaultInputFramePool->init()) {
            ELOG_ERROR("Frame pool(default input) init failed, ret %d", sts);

            m_defaultInputFramePool.reset();
            return;
        }

        m_defaultInputFrame = m_defaultInputFramePool->getFreeFrame();
        fillFrame(m_defaultInputFrame->getSurface(), 0x0, 0x80, 0x80); //black
    }
}

void MsdkVideoCompositor::updateRootSize(VideoSize& videoSize)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG("updateRootSize to %dx%d", videoSize.width, videoSize.height);

    printfToDo;
    //m_newCompositeSize = videoSize;
    //m_solutionState = CHANGING;
}

void MsdkVideoCompositor::updateBackgroundColor(YUVColor& bgColor)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG("updateBackgroundColor");

    m_newBgColor = bgColor;
    m_solutionState = CHANGING;
}

void MsdkVideoCompositor::updateLayoutSolution(LayoutSolution& solution)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG("Configuring layout");

    m_newLayout = solution;
    m_solutionState = CHANGING;
    ELOG_DEBUG("configChanged is true");
}

bool MsdkVideoCompositor::activateInput(int input)
{
    ELOG_DEBUG("activateInput = %d", input);

    m_inputs[input]->activate();

    return true;
}

void MsdkVideoCompositor::deActivateInput(int input)
{
    ELOG_DEBUG("deActivateInput = %d", input);

    m_inputs[input]->deActivate();
}

void MsdkVideoCompositor::pushInput(int input, const woogeen_base::Frame& frame)
{
    ELOG_TRACE("pushInput %d", input);

    m_inputs[input]->pushInput(frame);
}

void MsdkVideoCompositor::onTimeout()
{
    generateFrame();
}

void MsdkVideoCompositor::generateFrame()
{
    boost::shared_ptr<MsdkFrame> compositeFrame = layout();
    if (!compositeFrame)
        return;

    MsdkFrameHolder holder;
    holder.frame = compositeFrame;

    const int kMsToRtpTimestamp = 90;

    woogeen_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = woogeen_base::FRAME_FORMAT_MSDK;
    frame.payload = reinterpret_cast<uint8_t*>(&holder);
    frame.length = 0; // unused.
    frame.additionalInfo.video.width = compositeFrame->getWidth();
    frame.additionalInfo.video.height = compositeFrame->getHeight();
    frame.timeStamp = kMsToRtpTimestamp *
        (TickTime::MillisecondTimestamp() + m_ntpDelta);

    //ELOG_TRACE("timeStamp %u", frame.timeStamp);
    ELOG_TRACE("deliverFrame");

    deliverFrame(frame);
}

bool MsdkVideoCompositor::commitLayout()
{
    // Update the current video layout
    // m_compositeSize = m_newCompositeSize;
    // it is not likely root size change

    ELOG_DEBUG("commit customlayout");

    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        m_bgColor = m_newBgColor;
        m_currentLayout = m_newLayout;
        m_solutionState = IN_WORK;
    }

    m_compInputStreams.resize(m_currentLayout.size());
    m_extVppComp->NumInputStream = m_compInputStreams.size();
    m_extVppComp->InputStream = &m_compInputStreams.front();
    memset(m_extVppComp->InputStream, 0 , sizeof(mfxVPPCompInputStream) * m_extVppComp->NumInputStream);

    int i = 0;
    for (auto& l : m_currentLayout) {
        m_inputs[l.input]->updateInputRegion(l.region);
        m_compInputStreams[i++] = *m_inputs[l.input]->getVppRect();
    }

    updateParam();

#if 1 // should be bug for mfx
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_vpp->Reset(m_videoParam.get());
    if (sts > 0) {
        ELOG_TRACE("Ignore mfx warning, ret %d", sts);
    }
    else if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("mfx reset failed, ret %d", sts);
        return false;
    }
#endif

    sts = m_vpp->Init(m_videoParam.get());
    if (sts > 0) {
        ELOG_TRACE("Ignore mfx warning, ret %d", sts);
    }
    else if (sts != MFX_ERR_NONE) {
        ELOG_ERROR("mfx init failed, ret %d", sts);
        return false;
    }

    m_vpp->GetVideoParam(m_videoParam.get());
    printfVideoParam(m_videoParam.get(), MFX_VPP);

    ELOG_DEBUG("configChanged sets to false after commitLayout!");
    return true;
}

bool MsdkVideoCompositor::isSolutionChanged()
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    return (m_solutionState == CHANGING);
}

boost::shared_ptr<MsdkFrame> MsdkVideoCompositor::layout()
{
    if (isSolutionChanged())
        commitLayout();

    return customLayout();
}

boost::shared_ptr<MsdkFrame> MsdkVideoCompositor::customLayout()
{
    if (!m_currentLayout.size())
        return NULL;

    mfxStatus sts = MFX_ERR_UNKNOWN;//MFX_ERR_NONE;

    mfxSyncPoint syncp;

    boost::shared_ptr<MsdkFrame> dst = m_framePool->getFreeFrame();
    if (!dst)
    {
        ELOG_ERROR("No frame available");
        return NULL;
    }

    for (auto& l : m_currentLayout) {
        auto& input = m_inputs[l.input];
        boost::shared_ptr<MsdkFrame> src = input->popInput();

        ELOG_TRACE("Render Input-%d(%lu)!", l.input, m_currentLayout.size());

        if (!src) {
            ELOG_TRACE("Input-%d(%lu): Null surface, using default!", l.input, m_currentLayout.size());

            src = m_defaultInputFrame;
        }

retry:
        sts = m_vpp->RunFrameVPPAsync(src->getSurface(), dst->getSurface(), NULL, &syncp);
        if (sts == MFX_WRN_DEVICE_BUSY) {
            ELOG_TRACE("Device busy, retry!");

            usleep(1000); //1ms
            goto retry;
        }
        else if (sts == MFX_ERR_MORE_DATA) {
            //ELOG_TRACE("Input-%d(%lu): Require more data!", l.input, m_currentLayout.size());
        }
        else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("Input -%d(%lu): mfx vpp error, ret %d", l.input, m_currentLayout.size(), sts);

            break;
        }
    }

    if(sts != MFX_ERR_NONE)
    {
        ELOG_ERROR("Composite failed, ret %d", sts);
        return NULL;
    }

#if 0
    sts = m_session->SyncOperation(syncp, MFX_INFINITE);
    if(sts != MFX_ERR_NONE)
    {
        ELOG_ERROR("SyncOperation failed, ret %d", sts);
        return NULL;
    }
#endif

    return dst;
}

}
#endif /* ENABLE_MSDK */
