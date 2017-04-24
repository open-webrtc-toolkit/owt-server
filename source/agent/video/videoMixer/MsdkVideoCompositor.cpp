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

#include <iostream>
#include <fstream>

#include <deque>
#include <webrtc/system_wrappers/interface/clock.h>
#include <webrtc/system_wrappers/interface/tick_util.h>

#include "MsdkBase.h"
#include "MsdkVideoCompositor.h"

using namespace webrtc;
using namespace woogeen_base;

namespace mcu {

static void fillVppStream(mfxVPPCompInputStream *vppStream, const VideoSize& rootSize, const Region& region)
{
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

    memset(vppStream, 0, sizeof(mfxVPPCompInputStream));
    vppStream->DstX = offset_width;
    vppStream->DstY = offset_height;
    vppStream->DstW = sub_width;
    vppStream->DstH = sub_height;
}

DEFINE_LOGGER(MsdkAvatarManager, "mcu.media.MsdkVideoCompositor.MsdkAvatarManager");

MsdkAvatarManager::MsdkAvatarManager(uint8_t size, boost::shared_ptr<mfxFrameAllocator> allocator)
    : m_size(size)
    , m_allocator(allocator)
{
}

MsdkAvatarManager::~MsdkAvatarManager()
{
}

bool MsdkAvatarManager::getImageSize(const std::string &url, uint32_t *pWidth, uint32_t *pHeight)
{
    uint32_t width, height;
    size_t begin, end;
    char *str_end = NULL;

    begin = url.find('.');
    if (begin == std::string::npos) {
        ELOG_WARN("Invalid image size in url(%s)", url.c_str());
        return false;
    }

    end = url.find('x', begin);
    if (end == std::string::npos) {
        ELOG_WARN("Invalid image size in url(%s)", url.c_str());
        return false;
    }

    width = strtol(url.data() + begin + 1, &str_end, 10);
    if (url.data() + end != str_end) {
        ELOG_WARN("Invalid image size in url(%s)", url.c_str());
        return false;
    }

    begin = end;
    end = url.find('.', begin);
    if (end == std::string::npos) {
        ELOG_WARN("Invalid image size in url(%s)", url.c_str());
        return false;
    }

    height = strtol(url.data() + begin + 1, &str_end, 10);
    if (url.data() + end != str_end) {
        ELOG_WARN("Invalid image size in url(%s)", url.c_str());
        return false;
    }

    *pWidth = width;
    *pHeight = height;

    ELOG_TRACE("Image size in url(%s), %dx%d", url.c_str(), *pWidth, *pHeight);
    return true;
}

boost::shared_ptr<woogeen_base::MsdkFrame> MsdkAvatarManager::loadImage(const std::string &url)
{
    uint32_t width, height;

    if (!getImageSize(url, &width, &height))
        return NULL;

    std::ifstream in(url, std::ios::in | std::ios::binary);
    in.seekg (0, in.end);
    uint32_t size = in.tellg();
    in.seekg (0, in.beg);

    if (size <= 0 || ((width * height * 3 + 1) / 2) != size) {
        ELOG_WARN("Open avatar image(%s) error, invalid size %d, expected size %d"
                , url.c_str(), size, (width * height * 3 + 1) / 2);
        return NULL;
    }

    char *image = new char [size];;
    in.read (image, size);
    in.close();

    boost::shared_ptr<webrtc::I420VideoFrame> i420Frame(new webrtc::I420VideoFrame());
    i420Frame->CreateFrame(
            width * height,
            (uint8_t *)image,
            width * height / 4,
            (uint8_t *)image + width * height,
            width * height / 4,
            (uint8_t *)image + width * height * 5 / 4,
            width,
            height,
            width,
            width / 2,
            width / 2
            );
    delete image;

    boost::shared_ptr<woogeen_base::MsdkFrame> frame(new woogeen_base::MsdkFrame(width, height, m_allocator));
    if(!frame->init())
        return NULL;

    frame->convertFrom(*i420Frame.get());
    return frame;
}

bool MsdkAvatarManager::setAvatar(uint8_t index, const std::string &url)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("setAvatar(%d) = %s", index, url.c_str());

    auto it = m_inputs.find(index);
    if (it == m_inputs.end()) {
        m_inputs[index] = url;
        return true;
    }

    if (it->second == url) {
        return true;
    }
    std::string old_url = it->second;
    it->second = url;

    //delete
    for (auto& it2 : m_inputs) {
        if (old_url == it2.second)
            return true;
    }
    m_frames.erase(old_url);
    return true;
}

bool MsdkAvatarManager::unsetAvatar(uint8_t index)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("unsetAvatar(%d)", index);

    auto it = m_inputs.find(index);
    if (it == m_inputs.end()) {
        return true;
    }
    std::string url = it->second;
    m_inputs.erase(it);

    //delete
    for (auto& it2 : m_inputs) {
        if (url == it2.second)
            return true;
    }
    m_frames.erase(url);
    return true;
}

boost::shared_ptr<woogeen_base::MsdkFrame> MsdkAvatarManager::getAvatarFrame(uint8_t index)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    auto it = m_inputs.find(index);
    if (it == m_inputs.end()) {
        ELOG_WARN("Not valid index(%d)", index);
        return NULL;
    }
    auto it2 = m_frames.find(it->second);
    if (it2 != m_frames.end()) {
        return it2->second;
    }

    boost::shared_ptr<woogeen_base::MsdkFrame> frame = loadImage(it->second);
    m_frames[it->second] = frame;
    return frame;
}

class VppInput {
    DECLARE_LOGGER();

public:
    VppInput(MsdkVideoCompositor *owner, boost::shared_ptr<mfxFrameAllocator> allocator)
        : m_owner(owner)
        , m_allocator(allocator)
        , m_active(false)
        , m_swFramePoolWidth(0)
        , m_swFramePoolHeight(0)
    {
    }

    ~VppInput()
    {
        printfFuncEnter;

        {
            boost::unique_lock<boost::shared_mutex> lock(m_mutex);
            m_queue.clear();
        }

        m_owner->flush();
        m_swFramePool.reset(NULL);

        printfFuncExit;
    }

    void activate()
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        m_active = true;
    }

    void deActivate()
    {
        {
            boost::unique_lock<boost::shared_mutex> lock(m_mutex);

            m_active = false;
            m_queue.clear();
        }

        m_owner->flush();
        m_swFramePool.reset(NULL);
        m_swFramePoolWidth = 0;
        m_swFramePoolHeight = 0;
    }

    bool isActivate()
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        return m_active;
    }

    void pushInput(const woogeen_base::Frame& frame)
    {
        if (!processCmd(frame)) {
            boost::shared_ptr<MsdkFrame> msdkFrame = convert(frame);
            if (!msdkFrame)
                return;
            {
                boost::unique_lock<boost::shared_mutex> lock(m_mutex);

                m_queue.push_back(msdkFrame);
                if (m_queue.size() > MAX_DECODED_FRAME_IN_RENDERING) {
                    ELOG_TRACE("(%p)Reach max frames in queue, drop oldest frame!", this);
                    m_queue.pop_front();
                }
            }
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
        } else {
            // Keep at least one frame for renderer, postpone pop to next opt
            m_queue.pop_front();
        }

        input = m_queue.front();
        return input;
    }

protected:
    bool initSwFramePool(int width, int height)
    {
        m_swFramePool.reset(new MsdkFramePool(width, height, MAX_DECODED_FRAME_IN_RENDERING + 1, m_allocator));
        m_swFramePoolWidth = width;
        m_swFramePoolHeight = height;

        ELOG_TRACE("(%p)Frame pool initialzed for non MsdkFrame input", this);
        return true;
    }

    bool processCmd(const woogeen_base::Frame& frame)
    {
        if (frame.format == FRAME_FORMAT_MSDK) {
            MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
            if (holder && holder->cmd == MsdkCmd_DEC_FLUSH) {
                {
                    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
                    m_queue.clear();
                }

                m_owner->flush();
                return true;
            }
        }
        return false;
    }

    boost::shared_ptr<MsdkFrame> convert(const woogeen_base::Frame& frame)
    {
        if (frame.format == FRAME_FORMAT_MSDK) {
            MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;

            return holder->frame;
        } else if (frame.format == FRAME_FORMAT_I420) {
            const struct VideoFrameSpecificInfo &video = frame.additionalInfo.video;

            if (m_swFramePool == NULL || m_swFramePoolWidth < video.width || m_swFramePoolHeight < video.height) {
                if (m_swFramePool) {
                    {
                        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
                        m_queue.clear();
                    }
                    m_owner->flush();
                }

                if (!initSwFramePool(video.width, video.height))
                    return NULL;
            }

            boost::shared_ptr<MsdkFrame> dst = m_swFramePool->getFreeFrame();
            if (!dst)
            {
                ELOG_ERROR("(%p)No frame available in swFramePool", this);
                return NULL;
            }

            if (dst->getVideoWidth() != video.width || dst->getVideoHeight() != video.height)
                dst->setCrop(0, 0, video.width, video.height);

            I420VideoFrame *i420Frame = (reinterpret_cast<I420VideoFrame *>(frame.payload));
            if (!dst->convertFrom(*i420Frame))
            {
                ELOG_ERROR("(%p)Failed to convert I420 frame", this);
                return NULL;
            }

            return dst;
        } else{
            ELOG_ERROR("(%p)Unsupported frame format, %d", this, frame.format);
            return NULL;
        }
    }

private:
    MsdkVideoCompositor *m_owner;
    boost::shared_ptr<mfxFrameAllocator> m_allocator;
    bool m_active;
    std::deque<boost::shared_ptr<MsdkFrame>> m_queue;
    boost::scoped_ptr<MsdkFramePool> m_swFramePool;
    int m_swFramePoolWidth;
    int m_swFramePoolHeight;

    boost::shared_mutex m_mutex;
};

DEFINE_LOGGER(VppInput, "mcu.media.VppInput");

DEFINE_LOGGER(MsdkVideoCompositor, "mcu.media.MsdkVideoCompositor");

MsdkVideoCompositor::MsdkVideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, bool crop)
    : m_maxInput(maxInput)
    , m_crop(crop)
    , m_rootSize({0, 0})
    , m_newRootSize(rootSize)
    , m_bgColor({0, 0, 0})
    , m_newBgColor(bgColor)
    , m_solutionState(UN_INITIALIZED)
    , m_session(NULL)
    , m_vpp(NULL)
    , m_vppReady(false)
{
    ELOG_DEBUG("set rootSize(%dx%d), maxInput(%d), bgColor YCbCr(0x%x, 0x%x, 0x%x), crop(%d)"
            , rootSize.width, rootSize.height, maxInput
            , bgColor.y, bgColor.cb, bgColor.cr
            , crop
            );

    m_ntpDelta = Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() - TickTime::MillisecondTimestamp();

    initDefaultVppParam();
    initVpp();
    m_solutionState = CHANGING;

    m_inputs.resize(maxInput);
    for (auto& input : m_inputs) {
        input.reset(new VppInput(this, m_allocator));
    }

    m_avatarManager.reset(new MsdkAvatarManager(maxInput, m_allocator));

    m_jobTimer.reset(new JobTimer(30, this));
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
        MsdkBase *msdkBase = MsdkBase::get();
        if (msdkBase) {
            msdkBase->destroySession(m_session);
        }
    }

    m_allocator.reset();

    m_videoParam.reset();
    m_extVppComp.reset();
    m_compInputStreams.clear();

    m_defaultRootFrame.reset();

    m_inputs.clear();

    m_defaultInputFrame.reset();

    m_framePool.reset();

    m_frameQueue.clear();

    printfFuncExit;
}

void MsdkVideoCompositor::initDefaultVppParam(void)
{
    m_videoParam.reset(new mfxVideoParam);
    memset(m_videoParam.get(), 0, sizeof(mfxVideoParam));

    m_extVppComp.reset(new mfxExtVPPComposite);
    memset(m_extVppComp.get(), 0, sizeof(mfxExtVPPComposite));

    // mfxVideoParam Common
    m_videoParam->AsyncDepth = 1;
    m_videoParam->IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    m_videoParam->NumExtParam = 0;
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

    m_videoParam->vpp.In.FrameRateExtN      = 30;
    m_videoParam->vpp.In.FrameRateExtD      = 1;

    // mfxVideoParam Vpp Out
    m_videoParam->vpp.Out.FourCC            = MFX_FOURCC_NV12;
    m_videoParam->vpp.Out.ChromaFormat      = MFX_CHROMAFORMAT_YUV420;
    m_videoParam->vpp.Out.PicStruct         = MFX_PICSTRUCT_PROGRESSIVE;

    m_videoParam->vpp.Out.BitDepthLuma      = 0;
    m_videoParam->vpp.Out.BitDepthChroma    = 0;
    m_videoParam->vpp.Out.Shift             = 0;

    m_videoParam->vpp.Out.AspectRatioW      = 0;
    m_videoParam->vpp.Out.AspectRatioH      = 0;

    m_videoParam->vpp.Out.FrameRateExtN     = 30;
    m_videoParam->vpp.Out.FrameRateExtD     = 1;

    // mfxExtVPPComposite
    m_extVppComp->Header.BufferId           = MFX_EXTBUFF_VPP_COMPOSITE;
    m_extVppComp->Header.BufferSz           = sizeof(mfxExtVPPComposite);
    m_extVppComp->Y = 0;
    m_extVppComp->U = 0;
    m_extVppComp->V = 0;
}

void MsdkVideoCompositor::updateVppParam(void)
{
    m_videoParam->vpp.In.Width      = ALIGN16(m_rootSize.width);
    m_videoParam->vpp.In.Height     = ALIGN16(m_rootSize.height);
    m_videoParam->vpp.In.CropX      = 0;
    m_videoParam->vpp.In.CropY      = 0;
    m_videoParam->vpp.In.CropW      = m_rootSize.width;
    m_videoParam->vpp.In.CropH      = m_rootSize.height;

    m_videoParam->vpp.Out.Width     = ALIGN16(m_rootSize.width);
    m_videoParam->vpp.Out.Height    = ALIGN16(m_rootSize.height);
    m_videoParam->vpp.Out.CropX     = 0;
    m_videoParam->vpp.Out.CropY     = 0;
    m_videoParam->vpp.Out.CropW     = m_rootSize.width;
    m_videoParam->vpp.Out.CropH     = m_rootSize.height;

    m_extVppComp->Y                 = m_bgColor.y;
    m_extVppComp->U                 = m_bgColor.cb;
    m_extVppComp->V                 = m_bgColor.cr;

    {
        // workaroung 16.4.4 msdk's bug, swap r and b
        int C = m_bgColor.y - 16;
        int D = m_bgColor.cb - 128;
        int E = m_bgColor.cr - 128;

        int r = ( 298 * C           + 409 * E + 128) >> 8;
        int g = ( 298 * C - 100 * D - 208 * E + 128) >> 8;
        int b = ( 298 * C + 516 * D           + 128) >> 8;

        int t;

        t = r;
        r = b;
        b = t;

        m_extVppComp->Y = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
        m_extVppComp->U = ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
        m_extVppComp->V = ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

        ELOG_TRACE("swap r <-> b, yuv 0x%x, 0x%x, 0x%x -> 0x%x, 0x%x, 0x%x"
                , m_bgColor.y, m_bgColor.cb, m_bgColor.cr
                , m_extVppComp->Y, m_extVppComp->U, m_extVppComp->V
                );
    }
}

bool MsdkVideoCompositor::isValidVppParam(void)
{
    return (m_videoParam->vpp.In.CropW > 0
            && m_videoParam->vpp.In.CropH > 0
            && m_videoParam->vpp.Out.CropW > 0
            && m_videoParam->vpp.Out.CropH > 0);
}

void MsdkVideoCompositor::initVpp(void)
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

    m_defaultInputFrame.reset(new MsdkFrame(16, 16, m_allocator));
    if (!m_defaultInputFrame->init()) {
        ELOG_ERROR("Default input frame init failed");
        m_defaultInputFrame.reset();
        return;
    }
    m_defaultInputFrame->fillFrame(16, 128, 128);//black
    //m_defaultInputFrame->fillFrame(235, 128, 128);//white
    //m_defaultInputFrame->fillFrame(82, 90, 240);//red
    //m_defaultInputFrame->fillFrame(144, 54, 34);//green
    //m_defaultInputFrame->fillFrame(41, 240, 110);//blue
}

bool MsdkVideoCompositor::resetVpp()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!isValidVppParam()) {
        m_vppReady = false;
        return false;
    }

    sts = m_vpp->Reset(m_videoParam.get());
    if (sts > 0) {
        ELOG_TRACE("Ignore mfx warning, ret %d", sts);
    } else if (sts != MFX_ERR_NONE) {
        ELOG_TRACE("mfx reset failed, ret %d. Try to reinitialize.", sts);

        m_vpp->Close();
        sts = m_vpp->Init(m_videoParam.get());
        if (sts > 0) {
            ELOG_TRACE("Ignore mfx warning, ret %d", sts);
        } else if (sts != MFX_ERR_NONE) {
            MsdkBase::printfVideoParam(m_videoParam.get(), MFX_VPP);

            ELOG_ERROR("mfx init failed, ret %d", sts);
            m_vppReady = false;
            return false;
        }
    }

    m_vpp->GetVideoParam(m_videoParam.get());
    MsdkBase::printfVideoParam(m_videoParam.get(), MFX_VPP);

    // check root size
    if (!m_defaultRootFrame
            || m_defaultRootFrame->getVideoWidth() != m_rootSize.width
            || m_defaultRootFrame->getVideoHeight() != m_rootSize.height) {
        mfxFrameAllocRequest Request[2];
        memset(&Request, 0, sizeof(mfxFrameAllocRequest) * 2);

        ELOG_TRACE("reset root size %dx%d -> %dx%d"
                , m_defaultRootFrame ? m_defaultRootFrame->getVideoWidth() : 0
                , m_defaultRootFrame ? m_defaultRootFrame->getVideoHeight() : 0
                , m_rootSize.width
                , m_rootSize.height
                );

        sts = m_vpp->QueryIOSurf(m_videoParam.get(), Request);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts || MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts) {
            ELOG_TRACE("Ignore warning!");
        } else if (MFX_ERR_NONE != sts) {
            ELOG_ERROR("mfx QueryIOSurf() failed, ret %d", sts);

            m_vppReady = false;
            return false;
        }
        ELOG_TRACE("mfx QueryIOSurf: In(%d), Out(%d)", Request[0].NumFrameSuggested, Request[1].NumFrameSuggested);

        m_framePool.reset(new MsdkFramePool(Request[1], m_allocator));

        m_defaultRootFrame.reset(new MsdkFrame(m_rootSize.width, m_rootSize.height, m_allocator));
        if (!m_defaultRootFrame->init()) {
            ELOG_ERROR("Default root frame init failed");
            m_defaultRootFrame.reset();

            m_vppReady = false;
            return false;
        }
        m_defaultRootFrame->fillFrame(16, 128, 128);//black
    }

    m_vppReady = true;
    return true;
}

void MsdkVideoCompositor::updateRootSize(VideoSize& rootSize)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("updateRootSize: %dx%d", rootSize.width, rootSize.height);

    m_newRootSize = rootSize;
    m_solutionState = CHANGING;
}

void MsdkVideoCompositor::updateBackgroundColor(YUVColor& bgColor)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG("updateBackgroundColor: YCbCr(0x%x, 0x%x, 0x%x)", bgColor.y, bgColor.cb, bgColor.cr);

    m_newBgColor = bgColor;
    m_solutionState = CHANGING;
}

void MsdkVideoCompositor::updateLayoutSolution(LayoutSolution& solution)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    ELOG_DEBUG("updateLayoutSolution: size(%ld)", solution.size());
    for (auto& l : solution) {
        ELOG_TRACE("input(%d): relative(%f), left(%f), top(%f)", l.input, l.region.relativeSize, l.region.left, l.region.top);
    }

    m_newLayout = solution;
    m_solutionState = CHANGING;
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

bool MsdkVideoCompositor::setAvatar(int input, const std::string& avatar)
{
    ELOG_DEBUG("setAvatar(%d) = %s", input, avatar.c_str());

    return m_avatarManager->setAvatar(input, avatar);
}

bool MsdkVideoCompositor::unsetAvatar(int input)
{
    ELOG_DEBUG("unsetAvatar(%d)", input);

    return m_avatarManager->unsetAvatar(input);
}

void MsdkVideoCompositor::pushInput(int input, const woogeen_base::Frame& frame)
{
    ELOG_TRACE("+++pushInput %d", input);

    m_inputs[input]->pushInput(frame);

    ELOG_TRACE("---pushInput %d", input);
}

void MsdkVideoCompositor::onTimeout()
{
    generateFrame();
}

void MsdkVideoCompositor::flush()
{
    boost::shared_ptr<MsdkFrame> frame;

    int i = 0;
    while(!(frame = m_framePool->getFreeFrame())) {
        i++;
        ELOG_DEBUG("flush - wait %d(ms)", i);
        usleep(1000); //1ms
    }

    ELOG_DEBUG("flush successfully after %d(ms)", i);
}

void MsdkVideoCompositor::generateFrame()
{
    boost::shared_ptr<MsdkFrame> compositeFrame = layout();
    if (!compositeFrame)
        return;

    MsdkFrameHolder holder;
    holder.frame = compositeFrame;
    holder.cmd = MsdkCmd_NONE;

    const int kMsToRtpTimestamp = 90;

    woogeen_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = woogeen_base::FRAME_FORMAT_MSDK;
    frame.payload = reinterpret_cast<uint8_t*>(&holder);
    frame.length = 0; // unused.
    frame.additionalInfo.video.width = compositeFrame->getVideoWidth();
    frame.additionalInfo.video.height = compositeFrame->getVideoHeight();
    frame.timeStamp = kMsToRtpTimestamp *
        (TickTime::MillisecondTimestamp() + m_ntpDelta);

    //ELOG_TRACE("timeStamp %u", frame.timeStamp);
    ELOG_TRACE("+++deliverFrame");
    deliverFrame(frame);
    ELOG_TRACE("---deliverFrame");
}

bool MsdkVideoCompositor::commitLayout()
{
    // Update the current video layout
    ELOG_DEBUG("commitLayout");

    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        m_rootSize = m_newRootSize;
        m_bgColor = m_newBgColor;
        m_currentLayout = m_newLayout;
        m_solutionState = IN_WORK;
    }

    m_vppLayout.clear();
    int i = 0;
    for (auto& l : m_currentLayout) {
        fillVppStream(&m_vppLayout[i++], m_rootSize, l.region);
    }

    i = 0;
    m_compInputStreams.resize(m_vppLayout.size());
    for (auto& l : m_vppLayout) {
        m_compInputStreams[i++] = l.second;
    }
    m_extVppComp->NumInputStream = m_compInputStreams.size();
    m_extVppComp->InputStream = &m_compInputStreams.front();
    m_videoParam->NumExtParam = m_extVppComp->NumInputStream > 0 ? 1 : 0;

    updateVppParam();
    resetVpp();

    ELOG_DEBUG("layout changed after commitLayout!");
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

    return isVppReady() ? customLayout() : NULL;
}

boost::shared_ptr<MsdkFrame> MsdkVideoCompositor::customLayout()
{
    // feed default root frame instead of NULL
    if (!m_currentLayout.size()) {
        ELOG_TRACE("Feed default root frame");
        return m_defaultRootFrame;
    }

    mfxStatus sts = MFX_ERR_UNKNOWN;

    mfxSyncPoint syncP;

    boost::shared_ptr<MsdkFrame> dst = m_framePool->getFreeFrame();
    if (!dst) {
        ELOG_WARN("No frame available");
        return NULL;
    }

    //dumpMsdkFrameInfo("+++dst", dst);

    for (auto& l : m_currentLayout) {
        ELOG_TRACE("Render Input-%d(%lu)!", l.input, m_currentLayout.size());

        auto& input = m_inputs[l.input];
        boost::shared_ptr<MsdkFrame> src;

        if (input->isActivate()) {
            src = input->popInput();
        } else {
            src = m_avatarManager->getAvatarFrame(l.input);
        }

        if (!src) {
            ELOG_TRACE("Input-%d(%lu): Null surface, using default!", l.input, m_currentLayout.size());

            src = m_defaultInputFrame;
        }

        m_frameQueue.push_back(src);
    }

    applyAspectRatio();

    int i = 0;
    for (auto& l : m_currentLayout) {
        boost::shared_ptr<MsdkFrame> src = m_frameQueue[i++];

        //dumpMsdkFrameInfo("+++src", src);
retry:
        sts = m_vpp->RunFrameVPPAsync(src->getSurface(), dst->getSurface(), NULL, &syncP);
        if (sts == MFX_WRN_DEVICE_BUSY) {
            ELOG_TRACE("Device busy, retry!");

            usleep(1000); //1ms
            goto retry;
        } else if (sts == MFX_ERR_MORE_DATA) {
            //ELOG_TRACE("Input-%d(%lu): Require more data!", l.input, m_currentLayout.size());
        } else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR("Input -%d(%lu): mfx vpp error, ret %d", l.input, m_currentLayout.size(), sts);

            break;
        }

        //dumpMsdkFrameInfo("---src", src);
    }

    m_frameQueue.clear();

    if(sts != MFX_ERR_NONE) {
        ELOG_ERROR("Composite failed, ret %d", sts);
        return NULL;
    }

    //dumpMsdkFrameInfo("---dst", dst);

    dst->setSyncPoint(syncP);
    dst->setSyncFlag(true);

    //dst->dump();

#if 0
    sts = m_session->SyncOperation(syncP, MFX_INFINITE);
    if(sts != MFX_ERR_NONE) {
        ELOG_ERROR("SyncOperation failed, ret %d", sts);
        return NULL;
    }
#endif

    return dst;
}

void MsdkVideoCompositor::applyAspectRatio()
{
    bool isChanged = false;
    int size = m_frameQueue.size();

    if (size != m_extVppComp->NumInputStream) {
        ELOG_ERROR("Num of frames(%lu) is not equal w/ input streams(%d)", size, m_extVppComp->NumInputStream);
        return;
    }

    for (int i = 0; i < size; i++) {
        boost::shared_ptr<MsdkFrame> frame = m_frameQueue[i];
        mfxVPPCompInputStream *layoutRect = &m_vppLayout[i];
        mfxVPPCompInputStream *vppRect = &m_extVppComp->InputStream[i];

        if (frame == m_defaultInputFrame)
            continue;

        uint32_t frame_w    = frame->getCropW();
        uint32_t frame_h    = frame->getCropH();
        uint32_t vpp_w      = vppRect->DstW;
        uint32_t vpp_h      = vppRect->DstH;
        uint32_t x, y, w, h;

        if (m_crop) {
            if ((frame_w + 1) * vpp_h >= vpp_w * (frame_h - 1) &&
                    (frame_w - 1) * vpp_h <= vpp_w * (frame_h + 1)) {
                continue;
            } else if (frame_w * layoutRect->DstH > layoutRect->DstW * frame_h) {
                w = frame_h * layoutRect->DstW / layoutRect->DstH;
                h = frame_h;

                x = (frame_w - w) / 2;
                y = 0;
            } else {
                w = frame_w;
                h = frame_w * layoutRect->DstH / layoutRect->DstW;

                x = 0;
                y = (frame_h - h) / 2;
            }

            if (frame->getCropX() != x || frame->getCropY() != y || frame->getCropW() != w || frame->getCropH()!= h) {
                ELOG_TRACE("setCrop(%p) %d-%d-%d-%d -> %d-%d-%d-%d"
                        , frame.get()
                        , frame->getCropX(), frame->getCropY(), frame->getCropW(), frame->getCropH()
                        , x, y, w, h
                        );

                frame->setCrop(x, y, w, h);
            } else {
                ELOG_WARN("invalid setCrop(%p) %d-%d-%d-%d -> %d-%d-%d-%d"
                        , frame.get()
                        , frame->getCropX(), frame->getCropY(), frame->getCropW(), frame->getCropH()
                        , x, y, w, h
                        );
            }
        } else {
            if (frame_w * (vpp_h + 1) >= (vpp_w - 1) * frame_h &&
                    frame_w * (vpp_h - 1) <= (vpp_w + 1) * frame_h) {
                continue;
            } else if (frame_w * layoutRect->DstH > layoutRect->DstW * frame_h) {
                w = layoutRect->DstW;
                h = layoutRect->DstW * frame_h / frame_w;

                x = layoutRect->DstX;
                y = layoutRect->DstY + (layoutRect->DstH - h) / 2;
            } else {
                w = layoutRect->DstH * frame_w / frame_h;
                h = layoutRect->DstH;

                x = layoutRect->DstX + (layoutRect->DstW - w) / 2;
                y = layoutRect->DstY;
            }

            if (vppRect->DstX != x || vppRect->DstY != y || vppRect->DstW != w || vppRect->DstH != h) {
                ELOG_TRACE("update pos %d-%d-%d-%d -> %d-%d-%d-%d, aspect ratio %lf -> %lf"
                        , vppRect->DstX, vppRect->DstY, vppRect->DstW, vppRect->DstH
                        , x, y, w, h
                        , (double)vpp_w / vpp_h
                        , (double)frame_w / frame_h
                        );

                vppRect->DstX = x;
                vppRect->DstY = y;
                vppRect->DstW = w;
                vppRect->DstH = h;

                isChanged = true;
            } else {
                ELOG_WARN("invalid update pos %d-%d-%d-%d -> %d-%d-%d-%d, aspect ratio %lf -> %lf"
                        , vppRect->DstX, vppRect->DstY, vppRect->DstW, vppRect->DstH
                        , x, y, w, h
                        , (double)vpp_w / vpp_h
                        , (double)frame_w / frame_h
                        );
            }
        }
    }

    if (!isChanged)
        return;

    ELOG_INFO("apply new aspect ratio");
    resetVpp();
}

}
#endif /* ENABLE_MSDK */
