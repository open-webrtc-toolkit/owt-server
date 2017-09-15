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

#include <webrtc/api/video/video_frame.h>

#include "MsdkBase.h"
#include "MsdkVideoCompositor.h"
#include "FrameConverter.h"

using namespace webrtc;
using namespace woogeen_base;

namespace mcu {

static void fillVppStream(mfxVPPCompInputStream *vppStream, const VideoSize& rootSize, const Region& region)
{
    uint32_t sub_width      = (uint64_t)rootSize.width * region.area.rect.width.numerator / region.area.rect.width.denominator;
    uint32_t sub_height     = (uint64_t)rootSize.height * region.area.rect.height.numerator / region.area.rect.height.denominator;
    uint32_t offset_width   = (uint64_t)rootSize.width * region.area.rect.left.numerator / region.area.rect.left.denominator;
    uint32_t offset_height  = (uint64_t)rootSize.height * region.area.rect.top.numerator / region.area.rect.top.denominator;
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

    rtc::scoped_refptr<I420Buffer> i420Buffer = I420Buffer::Copy(
            width, height,
            reinterpret_cast<const uint8_t *>(image), width,
            reinterpret_cast<const uint8_t *>(image + width * height), width / 2,
            reinterpret_cast<const uint8_t *>(image + width * height * 5 / 4), width / 2
            );

    delete image;

    boost::shared_ptr<woogeen_base::MsdkFrame> frame(new woogeen_base::MsdkFrame(width, height, m_allocator));
    if(!frame->init())
        return NULL;

    frame->convertFrom(i420Buffer.get());
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
        m_converter.reset(new FrameConverter(false));
    }

    ~VppInput()
    {
        m_owner->flush();
        m_busyFrame.reset();
        m_swFramePool.reset(NULL);
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
        m_busyFrame.reset();
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

                if (!m_active)
                    return;

                m_busyFrame = msdkFrame;
            }
        }
    }

    boost::shared_ptr<MsdkFrame> popInput()
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        if(!m_active)
            return NULL;

        return m_busyFrame;
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

    boost::shared_ptr<woogeen_base::MsdkFrame> getMsdkFrame(const uint32_t width, const uint32_t height)
    {
        if (m_msdkFrame == NULL) {
            m_msdkFrame.reset(new MsdkFrame(width, height, m_allocator));
            if (!m_msdkFrame->init()) {
                m_msdkFrame.reset();
                return NULL;
            }
        }

        if(!(m_msdkFrame.use_count() == 1 && m_msdkFrame->isFree())) {
            ELOG_INFO("No free frame available");
            return NULL;
        }

        if (m_msdkFrame->getWidth() < width || m_msdkFrame->getHeight() < height) {
            m_msdkFrame.reset(new MsdkFrame(width, height, m_allocator));
            if (!m_msdkFrame->init()) {
                m_msdkFrame.reset();
                return NULL;
            }
        }

        if (m_msdkFrame->getVideoWidth() != width || m_msdkFrame->getVideoHeight() != height)
            m_msdkFrame->setCrop(0, 0, width, height);

        return m_msdkFrame;
    }

    bool processCmd(const woogeen_base::Frame& frame)
    {
        if (frame.format == FRAME_FORMAT_MSDK) {
            MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
            if (holder && holder->cmd == MsdkCmd_DEC_FLUSH) {
                {
                    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
                    boost::shared_ptr<MsdkFrame> copyFrame;
                    if(m_active && m_busyFrame) {
                        copyFrame = getMsdkFrame(m_busyFrame->getVideoWidth(), m_busyFrame->getVideoHeight());
                        if (copyFrame && !m_converter->convert(m_busyFrame.get(), copyFrame.get())) {
                            copyFrame.reset();
                        }
                    }
                    m_busyFrame = copyFrame;
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
                {
                    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
                    boost::shared_ptr<MsdkFrame> copyFrame;
                    if(m_active && m_busyFrame) {
                        copyFrame = getMsdkFrame(m_busyFrame->getVideoWidth(), m_busyFrame->getVideoHeight());
                        if (copyFrame && !m_converter->convert(m_busyFrame.get(), copyFrame.get())) {
                            copyFrame.reset();
                        }
                    }
                    m_busyFrame = copyFrame;
                }
                m_owner->flush();

                if (!initSwFramePool(video.width, video.height))
                    return NULL;
            }

            boost::shared_ptr<MsdkFrame> dst = m_swFramePool->getFreeFrame();
            if (!dst) {
                ELOG_ERROR("(%p)No frame available in swFramePool", this);
                return NULL;
            }

            if (dst->getVideoWidth() != video.width || dst->getVideoHeight() != video.height)
                dst->setCrop(0, 0, video.width, video.height);

            VideoFrame *i420Frame = (reinterpret_cast<VideoFrame *>(frame.payload));
            if (!dst->convertFrom(i420Frame->video_frame_buffer().get())) {
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

    boost::shared_ptr<woogeen_base::MsdkFrame> m_msdkFrame;
    boost::scoped_ptr<FrameConverter> m_converter;

    bool m_active;
    boost::shared_ptr<woogeen_base::MsdkFrame> m_busyFrame;

    // todo, dont flush
    boost::scoped_ptr<MsdkFramePool> m_swFramePool;
    int m_swFramePoolWidth;
    int m_swFramePoolHeight;

    boost::shared_mutex m_mutex;
};

DEFINE_LOGGER(VppInput, "mcu.media.VppInput");

DEFINE_LOGGER(MsdkVideoCompositor, "mcu.media.MsdkVideoCompositor");

MsdkVideoCompositor::MsdkVideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, bool crop)
    : m_clock(Clock::GetRealTimeClock())
    , m_maxInput(maxInput)
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

    // swap u/v as msdk's bug
    m_extVppComp->Y                 = m_bgColor.y;
    m_extVppComp->U                 = m_bgColor.cr;
    m_extVppComp->V                 = m_bgColor.cb;
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
        m_defaultRootFrame->fillFrame(m_bgColor.y, m_bgColor.cb, m_bgColor.cr);
    }

    if (!m_defaultInputFrame) {
        m_defaultInputFrame.reset(new MsdkFrame(16, 16, m_allocator));
        if (!m_defaultInputFrame->init()) {
            ELOG_ERROR("Default input frame init failed");
            m_defaultInputFrame.reset();

            m_vppReady = false;
            return false;
        }
        m_defaultInputFrame->fillFrame(m_bgColor.y, m_bgColor.cb, m_bgColor.cr);
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
        Region *pRegion = &l.region;

        ELOG_DEBUG("input(%d): shape(%s), left(%d/%d), top(%d/%d), width(%d/%d), height(%d/%d)"
                , l.input
                , pRegion->shape.c_str()
                , pRegion->area.rect.left.numerator, pRegion->area.rect.left.denominator
                , pRegion->area.rect.top.numerator, pRegion->area.rect.top.denominator
                , pRegion->area.rect.width.numerator, pRegion->area.rect.width.denominator
                , pRegion->area.rect.height.numerator, pRegion->area.rect.height.denominator);

        assert(pRegion->shape.compare("rectangle") == 0);
        assert(pRegion->area.rect.left.denominator != 0 && pRegion->area.rect.left.denominator >= pRegion->area.rect.left.numerator);
        assert(pRegion->area.rect.top.denominator != 0 && pRegion->area.rect.top.denominator >= pRegion->area.rect.top.numerator);
        assert(pRegion->area.rect.width.denominator != 0 && pRegion->area.rect.width.denominator >= pRegion->area.rect.width.numerator);
        assert(pRegion->area.rect.height.denominator != 0 && pRegion->area.rect.height.denominator >= pRegion->area.rect.height.numerator);

        ELOG_DEBUG("input(%d): left(%.2f), top(%.2f), width(%.2f), height(%.2f)"
                , l.input
                , (float)pRegion->area.rect.left.numerator / pRegion->area.rect.left.denominator
                , (float)pRegion->area.rect.top.numerator / pRegion->area.rect.top.denominator
                , (float)pRegion->area.rect.width.numerator / pRegion->area.rect.width.denominator
                , (float)pRegion->area.rect.height.numerator / pRegion->area.rect.height.denominator);
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

    woogeen_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = woogeen_base::FRAME_FORMAT_MSDK;
    frame.payload = reinterpret_cast<uint8_t*>(&holder);
    frame.length = 0; // unused.
    frame.additionalInfo.video.width = compositeFrame->getVideoWidth();
    frame.additionalInfo.video.height = compositeFrame->getVideoHeight();
    frame.timeStamp = kMsToRtpTimestamp * m_clock->TimeInMilliseconds();

    //ELOG_ERROR("+++deliverFrame %d", frame.timeStamp / 90);
    deliverFrame(frame);
    //ELOG_TRACE("---deliverFrame");
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

    if (m_vppLayout.size() > 0) {
        m_compInputStreams.resize(m_vppLayout.size());

        i = 0;
        for (auto& l : m_vppLayout) {
            m_compInputStreams[i++] = l.second;
        }
        m_extVppComp->NumInputStream = m_compInputStreams.size();
        m_extVppComp->InputStream = &m_compInputStreams.front();
        m_videoParam->NumExtParam = m_extVppComp->NumInputStream > 0 ? 1 : 0;
    } else {
        m_compInputStreams.clear();
        m_videoParam->NumExtParam = 0;
    }

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
    uint32_t size = m_frameQueue.size();

    if (size != m_vppLayout.size()) {
        ELOG_ERROR("Num of frames(%d) is not equal w/ input streams", size);
        return;
    }

    bool isChanged = false;
    for (uint32_t i = 0; i < size; i++) {
        boost::shared_ptr<MsdkFrame> frame = m_frameQueue[i];
        mfxVPPCompInputStream *layoutRect = &m_vppLayout[i];
        mfxVPPCompInputStream *vppRect = &m_extVppComp->InputStream[i];

        if (frame == m_defaultInputFrame)
            continue;

        uint32_t frame_w = frame->getCropW();
        uint32_t frame_h = frame->getCropH();
        uint32_t x, y, w, h;

        if (m_crop) {
            w = std::min(frame_w, layoutRect->DstW * frame_h / layoutRect->DstH);
            h = std::min(frame_h, layoutRect->DstH * frame_w / layoutRect->DstW);

            x = frame->getCropX() + (frame_w - w) / 2;
            y = frame->getCropY() + (frame_h - h) / 2;

            if (frame->getCropX() != x || frame->getCropY() != y || frame->getCropW() != w || frame->getCropH()!= h) {
                ELOG_TRACE("setCrop(%p) %d-%d-%d-%d -> %d-%d-%d-%d"
                        , frame.get()
                        , frame->getCropX(), frame->getCropY(), frame->getCropW(), frame->getCropH()
                        , x, y, w, h
                        );

                frame->setCrop(x, y, w, h);
            }
        } else {
            w = std::min(layoutRect->DstW, frame_w * layoutRect->DstH / frame_h);
            h = std::min(layoutRect->DstH, frame_h * layoutRect->DstW / frame_w);

            x = layoutRect->DstX + (layoutRect->DstW - w) / 2;
            y = layoutRect->DstY + (layoutRect->DstH - h) / 2;

            if (vppRect->DstX != x || vppRect->DstY != y || vppRect->DstW != w || vppRect->DstH != h) {
                ELOG_TRACE("update pos %d-%d-%d-%d -> %d-%d-%d-%d, aspect ratio %lf -> %lf"
                        , vppRect->DstX, vppRect->DstY, vppRect->DstW, vppRect->DstH
                        , x, y, w, h
                        , (double)vppRect->DstW / vppRect->DstH
                        , (double)w / h
                        );

                vppRect->DstX = x;
                vppRect->DstY = y;
                vppRect->DstW = w;
                vppRect->DstH = h;

                isChanged = true;
            }
        }
    }

    if (!isChanged)
        return;

    ELOG_DEBUG("apply new aspect ratio");
    resetVpp();
}

}
#endif /* ENABLE_MSDK */
