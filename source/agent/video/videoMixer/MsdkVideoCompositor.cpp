// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifdef ENABLE_MSDK

#include <iostream>
#include <fstream>

#include "MsdkBase.h"
#include "MsdkVideoCompositor.h"

using namespace webrtc;
using namespace owt_base;

namespace mcu {

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

boost::shared_ptr<owt_base::MsdkFrame> MsdkAvatarManager::loadImage(const std::string &url)
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

    delete[] image;

    boost::shared_ptr<owt_base::MsdkFrame> frame(new owt_base::MsdkFrame(width, height, m_allocator));
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

boost::shared_ptr<owt_base::MsdkFrame> MsdkAvatarManager::getAvatarFrame(uint8_t index)
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

    boost::shared_ptr<owt_base::MsdkFrame> frame = loadImage(it->second);
    m_frames[it->second] = frame;
    return frame;
}

DEFINE_LOGGER(MsdkInput, "mcu.media.MsdkVideoCompositor.MsdkInput");

MsdkInput::MsdkInput(MsdkVideoCompositor *owner, boost::shared_ptr<mfxFrameAllocator> allocator)
    : m_owner(owner)
    , m_allocator(allocator)
    , m_active(false)
    , m_swFramePoolWidth(0)
    , m_swFramePoolHeight(0)
{
}

MsdkInput::~MsdkInput()
{
    m_busyFrame.reset();
    m_swFramePool.reset(NULL);
}

void MsdkInput::activate()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_active = true;
}

void MsdkInput::deActivate()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_active = false;
    m_busyFrame.reset();
}

bool MsdkInput::isActivate()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    return m_active;
}

void MsdkInput::pushInput(const owt_base::Frame& frame)
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

boost::shared_ptr<MsdkFrame> MsdkInput::popInput()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    if(!m_active)
        return NULL;

    return m_busyFrame;
}

bool MsdkInput::initSwFramePool(int width, int height)
{
    m_swFramePool.reset(new MsdkFramePool(width, height, MAX_DECODED_FRAME_IN_RENDERING + 1, m_allocator));
    m_swFramePoolWidth = width;
    m_swFramePoolHeight = height;

    ELOG_TRACE_T("Frame pool initialzed for non MsdkFrame input");
    return true;
}

boost::shared_ptr<owt_base::MsdkFrame> MsdkInput::getMsdkFrame(const uint32_t width, const uint32_t height)
{
    if (m_msdkFrame == NULL) {
        m_msdkFrame.reset(new MsdkFrame(width, height, m_allocator));
        if (!m_msdkFrame->init()) {
            m_msdkFrame.reset();
            return NULL;
        }
    }

    if(!(m_msdkFrame.use_count() == 1 && m_msdkFrame->isFree())) {
        ELOG_INFO_T("No free frame available");
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

bool MsdkInput::processCmd(const owt_base::Frame& frame)
{
    if (frame.format == FRAME_FORMAT_MSDK) {
        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
        if (holder && holder->cmd == MsdkCmd_DEC_FLUSH) {
            {
                boost::unique_lock<boost::shared_mutex> lock(m_mutex);
                boost::shared_ptr<MsdkFrame> copyFrame;
                if(m_active && m_busyFrame) {
                    if (!m_converter)
                        m_converter.reset(new FrameConverter());

                    if (m_converter) {
                        copyFrame = getMsdkFrame(m_busyFrame->getVideoWidth(), m_busyFrame->getVideoHeight());
                        if (copyFrame && !m_converter->convert(m_busyFrame.get(), copyFrame.get())) {
                            copyFrame.reset();
                        }
                    }
                }
                m_busyFrame = copyFrame;
            }
            if (m_busyFrame)
                m_busyFrame->sync();

            m_owner->flush();
            return true;
        }
    }
    return false;
}

boost::shared_ptr<MsdkFrame> MsdkInput::convert(const owt_base::Frame& frame)
{
    if (frame.format == FRAME_FORMAT_MSDK) {
        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;

        return holder->frame;
    } else if (frame.format == FRAME_FORMAT_I420) {
        const struct VideoFrameSpecificInfo &video = frame.additionalInfo.video;

        if (m_swFramePool == NULL || m_swFramePoolWidth < video.width || m_swFramePoolHeight < video.height) {
            { // todo, new msdk frame pool impl
                boost::unique_lock<boost::shared_mutex> lock(m_mutex);
                boost::shared_ptr<MsdkFrame> copyFrame;
                if(m_active && m_busyFrame) {
                    if (!m_converter)
                        m_converter.reset(new FrameConverter());

                    if (m_converter) {
                        copyFrame = getMsdkFrame(m_busyFrame->getVideoWidth(), m_busyFrame->getVideoHeight());
                        if (copyFrame && !m_converter->convert(m_busyFrame.get(), copyFrame.get())) {
                            copyFrame.reset();
                        }
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
            ELOG_ERROR_T("No frame available in swFramePool");
            return NULL;
        }

        if (dst->getVideoWidth() != video.width || dst->getVideoHeight() != video.height)
            dst->setCrop(0, 0, video.width, video.height);

        VideoFrame *i420Frame = (reinterpret_cast<VideoFrame *>(frame.payload));
        if (!dst->convertFrom(i420Frame->video_frame_buffer().get())) {
            ELOG_ERROR_T("Failed to convert I420 frame");
            return NULL;
        }

        return dst;
    } else{
        ELOG_ERROR_T("Unsupported frame format, %d", frame.format);
        return NULL;
    }
}

DEFINE_LOGGER(MsdkVpp, "mcu.media.MsdkVideoCompositor.MsdkVpp");

MsdkVpp::MsdkVpp(owt_base::VideoSize &size, owt_base::YUVColor &bgColor, const bool crop)
    : m_size(size)
    , m_bgColor(bgColor)
    , m_crop(crop)
{
    defaultParam();
    updateParam();
    createVpp();

    allocateFrames();
}

MsdkVpp::~MsdkVpp()
{
    if (m_vpp) {
        m_vpp->Close();
        delete m_vpp;
        m_vpp= NULL;
    }

    m_mixedFramePool.reset();
    m_defaultInputFrame.reset();
    m_defaultRootFrame.reset();

    m_allocator.reset();

    if (m_session) {
        MsdkBase *msdkBase = MsdkBase::get();
        if (msdkBase) {
            msdkBase->destroySession(m_session);
        }
    }
}

bool MsdkVpp::allocateFrames()
{
    if (!m_defaultInputFrame) {
        m_defaultInputFrame.reset(new MsdkFrame(16, 16, m_allocator));
        if (!m_defaultInputFrame->init()) {
            ELOG_ERROR_T("Default input frame init failed");
            m_defaultInputFrame.reset();

            return false;
        }
        m_defaultInputFrame->fillFrame(m_bgColor.y, m_bgColor.cb, m_bgColor.cr);
    }

    if (!m_defaultRootFrame
            || m_defaultRootFrame->getVideoWidth() != m_size.width
            || m_defaultRootFrame->getVideoHeight() != m_size.height) {
        ELOG_TRACE_T("reset root size %dx%d -> %dx%d"
                , m_defaultRootFrame ? m_defaultRootFrame->getVideoWidth() : 0
                , m_defaultRootFrame ? m_defaultRootFrame->getVideoHeight() : 0
                , m_size.width
                , m_size.height
                );

        m_defaultRootFrame.reset(new MsdkFrame(m_size.width, m_size.height, m_allocator));
        if (!m_defaultRootFrame->init()) {
            ELOG_ERROR_T("Default root frame init failed");
            m_defaultRootFrame.reset();

            return false;
        }
        m_defaultRootFrame->fillFrame(m_bgColor.y, m_bgColor.cb, m_bgColor.cr);
    }

    return true;
}

void MsdkVpp::defaultParam(void)
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

void MsdkVpp::updateParam(void)
{
    m_videoParam->vpp.In.Width      = ALIGN16(m_size.width);
    m_videoParam->vpp.In.Height     = ALIGN16(m_size.height);
    m_videoParam->vpp.In.CropX      = 0;
    m_videoParam->vpp.In.CropY      = 0;
    m_videoParam->vpp.In.CropW      = m_size.width;
    m_videoParam->vpp.In.CropH      = m_size.height;

    m_videoParam->vpp.Out.Width     = ALIGN16(m_size.width);
    m_videoParam->vpp.Out.Height    = ALIGN16(m_size.height);
    m_videoParam->vpp.Out.CropX     = 0;
    m_videoParam->vpp.Out.CropY     = 0;
    m_videoParam->vpp.Out.CropW     = m_size.width;
    m_videoParam->vpp.Out.CropH     = m_size.height;

    // swap u/v as msdk's bug
    m_extVppComp->Y                 = m_bgColor.y;
    m_extVppComp->U                 = m_bgColor.cr;
    m_extVppComp->V                 = m_bgColor.cb;
}

void MsdkVpp::createVpp(void)
{
    mfxStatus sts = MFX_ERR_NONE;

    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase == NULL) {
        ELOG_ERROR_T("Get MSDK failed.");
        return;
    }

    m_session = msdkBase->createSession();
    if (!m_session ) {
        ELOG_ERROR_T("Create session failed.");
        return;
    }

    m_allocator = msdkBase->createFrameAllocator();
    if (!m_allocator) {
        ELOG_ERROR_T("Create frame allocator failed.");
        return;
    }

    sts = m_session->SetFrameAllocator(m_allocator.get());
    if (sts != MFX_ERR_NONE) {
        ELOG_ERROR_T("Set frame allocator failed.");
        return;
    }

    m_vpp = new MFXVideoVPP(*m_session);
    if (!m_vpp) {
        ELOG_ERROR_T("Create vpp failed.");
        return;
    }
}

bool MsdkVpp::resetVpp()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_vppReady = false;

    sts = m_vpp->Reset(m_videoParam.get());
    if (sts > 0) {
        ELOG_TRACE_T("Ignore mfx warning, ret %d", sts);
    } else if (sts != MFX_ERR_NONE) {
        ELOG_TRACE_T("mfx reset failed, ret %d. Try to reinitialize.", sts);

        m_vpp->Close();
        sts = m_vpp->Init(m_videoParam.get());
        if (sts > 0) {
            ELOG_TRACE_T("Ignore mfx warning, ret %d", sts);
        } else if (sts != MFX_ERR_NONE) {
            MsdkBase::printfVideoParam(m_videoParam.get(), MFX_VPP);

            ELOG_ERROR_T("mfx init failed, ret %d", sts);
            return false;
        }
    }

    m_vpp->GetVideoParam(m_videoParam.get());
    MsdkBase::printfVideoParam(m_videoParam.get(), MFX_VPP);

    m_vppReady = true;
    return true;
}

void MsdkVpp::convertToCompInputStream(mfxVPPCompInputStream *vppStream, const owt_base::VideoSize& rootSize, const Region& region)
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

void MsdkVpp::applyAspectRatio(std::vector<boost::shared_ptr<owt_base::MsdkFrame>> &inputFrames)
{
    uint32_t size = inputFrames.size();

    if (size != m_msdkLayout.size()) {
        ELOG_ERROR_T("Num of frames(%d) is not equal w/ input streams", size);
        return;
    }

    bool isChanged = false;
    for (uint32_t i = 0; i < size; i++) {
        boost::shared_ptr<MsdkFrame> frame = inputFrames[i];
        if (!frame)
            continue;

        mfxVPPCompInputStream *layoutRect = &m_msdkLayout[i];
        mfxVPPCompInputStream *roiRect = &m_extVppComp->InputStream[i];

        uint32_t frame_w = frame->getCropW();
        uint32_t frame_h = frame->getCropH();
        uint32_t x, y, w, h;

        if (m_crop) {
            w = std::min(frame_w, layoutRect->DstW * frame_h / layoutRect->DstH);
            h = std::min(frame_h, layoutRect->DstH * frame_w / layoutRect->DstW);

            x = frame->getCropX() + (frame_w - w) / 2;
            y = frame->getCropY() + (frame_h - h) / 2;

            if (frame->getCropX() != x || frame->getCropY() != y || frame->getCropW() != w || frame->getCropH()!= h) {
                ELOG_TRACE_T("setCrop(%p) %d-%d-%d-%d -> %d-%d-%d-%d"
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

            if (roiRect->DstX != x || roiRect->DstY != y || roiRect->DstW != w || roiRect->DstH != h) {
                ELOG_TRACE_T("update pos %d-%d-%d-%d -> %d-%d-%d-%d, aspect ratio %lf -> %lf"
                        , roiRect->DstX, roiRect->DstY, roiRect->DstW, roiRect->DstH
                        , x, y, w, h
                        , (double)roiRect->DstW / roiRect->DstH
                        , (double)w / h
                        );

                roiRect->DstX = x;
                roiRect->DstY = y;
                roiRect->DstW = w;
                roiRect->DstH = h;

                isChanged = true;
            }
        }
    }

    if (!isChanged)
        return;

    ELOG_DEBUG_T("apply new aspect ratio");
    resetVpp();
}

bool MsdkVpp::init()
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest Request[2];

    memset(&Request, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = m_vpp->QueryIOSurf(m_videoParam.get(), Request);
    if (MFX_WRN_PARTIAL_ACCELERATION == sts || MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == sts) {
        ELOG_TRACE_T("Ignore warning!");
    } else if (MFX_ERR_NONE != sts) {
        ELOG_ERROR_T("mfx QueryIOSurf() failed, ret %d", sts);

        return false;
    }

    Request[1].NumFrameSuggested = NumOfMixedFrames;
    m_mixedFramePool.reset(new MsdkFramePool(Request[1], m_allocator));

    if (!m_defaultInputFrame) {
        m_defaultInputFrame.reset(new MsdkFrame(16, 16, m_allocator));
        if (!m_defaultInputFrame->init()) {
            ELOG_ERROR_T("Default input frame init failed");
            m_defaultInputFrame.reset();

            return false;
        }
        m_defaultInputFrame->fillFrame(m_bgColor.y, m_bgColor.cb, m_bgColor.cr);
    }

    return true;
}

bool MsdkVpp::update(owt_base::VideoSize &size, owt_base::YUVColor &bgColor, LayoutSolution &layout)
{
    m_layout = layout;

    m_msdkLayout.clear();
    m_msdkLayout.resize(m_layout.size());

    m_compInputStreams.clear();
    m_compInputStreams.resize(m_layout.size());

    int i = 0;
    for (auto& l : m_layout) {
        convertToCompInputStream(&m_msdkLayout[i], m_size, l.region);
        m_compInputStreams[i] = m_msdkLayout[i];
        i++;
    }

    m_extVppComp->NumInputStream = m_compInputStreams.size();
    m_extVppComp->InputStream = &m_compInputStreams.front();
    m_videoParam->NumExtParam = (m_extVppComp->NumInputStream > 0) ? 1 : 0;

    updateParam();
    resetVpp();

    return true;
}

boost::shared_ptr<owt_base::MsdkFrame> MsdkVpp::mix(
            std::vector<boost::shared_ptr<owt_base::MsdkFrame>> &inputFrames)
{
    if (m_layout.size() == 0) {
        ELOG_TRACE_T("Feed default root frame");
        return m_defaultRootFrame;
    }

    boost::shared_ptr<MsdkFrame> dst = m_mixedFramePool->getFreeFrame();
    if (!dst) {
        ELOG_WARN_T("No frame available");
        return NULL;
    }

    applyAspectRatio(inputFrames);

    mfxStatus sts = MFX_ERR_UNKNOWN;
    mfxSyncPoint syncP;

    int i = 0;
    for (auto& l : m_layout) {
        boost::shared_ptr<MsdkFrame> src = inputFrames[i++];
        if (!src)
            src = m_defaultInputFrame;

retry:
        sts = m_vpp->RunFrameVPPAsync(src->getSurface(), dst->getSurface(), NULL, &syncP);
        if (sts == MFX_WRN_DEVICE_BUSY) {
            ELOG_TRACE_T("Device busy, retry!");

            usleep(1000); //1ms
            goto retry;
        } else if (sts == MFX_ERR_MORE_DATA) {
            //ELOG_TRACE("Input-%d(%lu): Require more data!", l.input, m_layout.size());
        } else if (sts != MFX_ERR_NONE) {
            ELOG_ERROR_T("Input -%d(%lu): mfx vpp error, ret %d", l.input, m_layout.size(), sts);

            break;
        }
    }

    if(sts != MFX_ERR_NONE) {
        ELOG_ERROR_T("Composite failed, ret %d", sts);
        return NULL;
    }

    dst->setSyncPoint(syncP);
    dst->setSyncFlag(true);

#if 0
    sts = m_session->SyncOperation(syncP, MFX_INFINITE);
    if(sts != MFX_ERR_NONE) {
        ELOG_ERROR_T("SyncOperation failed, ret %d", sts);
        return NULL;
    }
#endif

    return dst;
}

DEFINE_LOGGER(MsdkFrameGenerator, "mcu.media.MsdkVideoCompositor.MsdkFrameGenerator");

MsdkFrameGenerator::MsdkFrameGenerator(
            MsdkVideoCompositor *owner,
            owt_base::VideoSize &size,
            owt_base::YUVColor &bgColor,
            const bool crop,
            const uint32_t maxFps,
            const uint32_t minFps)
    : m_clock(Clock::GetRealTimeClock())
    , m_owner(owner)
    , m_maxSupportedFps(maxFps)
    , m_minSupportedFps(minFps)
    , m_counter(0)
    , m_counterMax(0)
    , m_size(size)
    , m_bgColor(bgColor)
    , m_crop(crop)
    , m_configureChanged(false)
{
    ELOG_DEBUG_T("Support fps max(%d), min(%d)", m_maxSupportedFps, m_minSupportedFps);

    uint32_t fps = m_minSupportedFps;
    while (fps <= m_maxSupportedFps) {
        if (fps == m_maxSupportedFps)
            break;

        fps *= 2;
    }
    if (fps > m_maxSupportedFps) {
        ELOG_WARN_T("Invalid fps min(%d), max(%d) --> mix(%d), max(%d)"
                , m_minSupportedFps, m_maxSupportedFps
                , m_minSupportedFps, m_minSupportedFps
                );
        m_maxSupportedFps = m_minSupportedFps;
    }

    m_counter = 0;
    m_counterMax = m_maxSupportedFps / m_minSupportedFps;

    m_outputs.resize(m_maxSupportedFps / m_minSupportedFps);

    m_msdkVpp.reset(new MsdkVpp(m_size, m_bgColor, m_crop));
    m_msdkVpp->init();

    ELOG_DEBUG_T("MsdkVpp(%p)", m_msdkVpp.get());

    m_jobTimer.reset(new JobTimer(m_maxSupportedFps, this));
    m_jobTimer->start();
}

MsdkFrameGenerator::~MsdkFrameGenerator()
{
    ELOG_DEBUG_T("Exit");

    m_jobTimer->stop();

    m_msdkVpp.reset();

    for (uint32_t i = 0; i <  m_outputs.size(); i++) {
        if (m_outputs[i].size())
            ELOG_WARN_T("Outputs not empty!!!");
    }
}

bool MsdkFrameGenerator::isSupported(uint32_t width, uint32_t height, uint32_t fps)
{
    if (fps > m_maxSupportedFps || fps < m_minSupportedFps)
        return false;

    uint32_t n = m_minSupportedFps;
    while (n <= m_maxSupportedFps) {
        if (n == fps)
            return true;

        n *= 2;
    }

    return false;
}

bool MsdkFrameGenerator::addOutput(const uint32_t width, const uint32_t height, const uint32_t fps, owt_base::FrameDestination *dst) {
    assert(isSupported(width, height, fps));

    boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);

    int index = m_maxSupportedFps / fps - 1;

    Output_t output{.width = width, .height = height, .fps = fps, .dest = dst};
    m_outputs[index].push_back(output);
    return true;
}

bool MsdkFrameGenerator::removeOutput(owt_base::FrameDestination *dst) {
    boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);

    for (uint32_t i = 0; i < m_outputs.size(); i++) {
        for (auto it = m_outputs[i].begin(); it != m_outputs[i].end(); ++it) {
            if (it->dest == dst) {
                m_outputs[i].erase(it);
                return true;
            }
        }
    }

    return false;
}

void MsdkFrameGenerator::updateLayoutSolution(LayoutSolution& solution)
{
    boost::unique_lock<boost::shared_mutex> lock(m_configMutex);

    m_newLayout         = solution;
    m_configureChanged  = true;
}

void MsdkFrameGenerator::onTimeout()
{
    bool hasValidOutput = false;
    {
        boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
        for (uint32_t i = 0; i < m_outputs.size(); i++) {
            if (m_counter % (i + 1))
                continue;

            if (m_outputs[i].size() > 0) {
                hasValidOutput = true;
                break;
            }
        }
    }

    if (hasValidOutput) {
        boost::shared_ptr<MsdkFrame> compositeFrame = generateFrame();
        if (compositeFrame) {
            MsdkFrameHolder holder;
            holder.frame = compositeFrame;
            holder.cmd = MsdkCmd_NONE;

            owt_base::Frame frame;
            memset(&frame, 0, sizeof(frame));
            frame.format = owt_base::FRAME_FORMAT_MSDK;
            frame.payload = reinterpret_cast<uint8_t*>(&holder);
            frame.length = 0; // unused.
            frame.additionalInfo.video.width = compositeFrame->getVideoWidth();
            frame.additionalInfo.video.height = compositeFrame->getVideoHeight();
            frame.timeStamp = kMsToRtpTimestamp * m_clock->TimeInMilliseconds();

            {
                boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
                for (uint32_t i = 0; i <  m_outputs.size(); i++) {
                    if (m_counter % (i + 1))
                        continue;

                    for (auto it = m_outputs[i].begin(); it != m_outputs[i].end(); ++it) {
                        ELOG_TRACE_T("+++deliverFrame(%d), dst(%p), fps(%d), timestamp(%d)"
                                , m_counter, it->dest, m_maxSupportedFps / (i + 1), frame.timeStamp / 90);

                        it->dest->onFrame(frame);
                    }
                }
            }
        }
    }

    m_counter = (m_counter + 1) % m_counterMax;
}

boost::shared_ptr<MsdkFrame> MsdkFrameGenerator::generateFrame()
{
    reconfigureIfNeeded();
    return layout();
}

void MsdkFrameGenerator::reconfigureIfNeeded()
{
    {
        boost::unique_lock<boost::shared_mutex> lock(m_configMutex);
        if (!m_configureChanged)
            return;

        m_layout = m_newLayout;
        m_configureChanged = false;
    }

    ELOG_DEBUG_T("reconfigure");
    m_msdkVpp->update(m_size, m_bgColor, m_layout);
}

boost::shared_ptr<MsdkFrame> MsdkFrameGenerator::layout()
{
    std::vector<boost::shared_ptr<owt_base::MsdkFrame>> inputFrames;
    for (auto& l : m_layout) {
        boost::shared_ptr<MsdkFrame> src = m_owner->getInputFrame(l.input);
        inputFrames.push_back(src);
    }

    boost::shared_ptr<MsdkFrame> dst = m_msdkVpp->mix(inputFrames);
    return dst;
}

DEFINE_LOGGER(MsdkVideoCompositor, "mcu.media.MsdkVideoCompositor");

MsdkVideoCompositor::MsdkVideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, bool crop)
    : m_maxInput(maxInput)
    , m_session(NULL)
{
    ELOG_DEBUG("set rootSize(%dx%d), maxInput(%d), bgColor YCbCr(0x%x, 0x%x, 0x%x), crop(%d)"
            , rootSize.width, rootSize.height, maxInput
            , bgColor.y, bgColor.cb, bgColor.cr
            , crop
            );

    createAllocator();

    ELOG_TRACE("+++MsdkInput");
    m_inputs.resize(maxInput);
    for (auto& input : m_inputs) {
        input.reset(new MsdkInput(this, m_allocator));
    }
    ELOG_TRACE("---MsdkInput");

    ELOG_TRACE("+++MsdkAvatarManager");
    m_avatarManager.reset(new MsdkAvatarManager(maxInput, m_allocator));
    ELOG_TRACE("---MsdkAvatarManager");

    ELOG_TRACE("+++MsdkFrameGenerator");
    m_generators.resize(2);
    m_generators[0].reset(new MsdkFrameGenerator(this, rootSize, bgColor, crop, 60, 15));
    m_generators[1].reset(new MsdkFrameGenerator(this, rootSize, bgColor, crop, 48, 6));
    ELOG_TRACE("---MsdkFrameGenerator");

    ELOG_DEBUG("Constructed!");
}

MsdkVideoCompositor::~MsdkVideoCompositor()
{
    if (m_session) {
        MsdkBase *msdkBase = MsdkBase::get();
        if (msdkBase) {
            msdkBase->destroySession(m_session);
        }
    }

    m_generators.clear();
    m_avatarManager.reset();
    m_inputs.clear();

    m_allocator.reset();
}

void MsdkVideoCompositor::createAllocator()
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

    return;
}

void MsdkVideoCompositor::updateRootSize(VideoSize& rootSize)
{
    ELOG_WARN("Not support updateRootSize: %dx%d", rootSize.width, rootSize.height);
}

void MsdkVideoCompositor::updateBackgroundColor(YUVColor& bgColor)
{
    ELOG_WARN("Not support updateBackgroundColor: YCbCr(0x%x, 0x%x, 0x%x)", bgColor.y, bgColor.cb, bgColor.cr);
}

void MsdkVideoCompositor::updateLayoutSolution(LayoutSolution& solution)
{
    assert(solution.size() <= m_maxInput);

    for (auto& generator : m_generators) {
        generator->updateLayoutSolution(solution);
    }
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

void MsdkVideoCompositor::pushInput(int input, const owt_base::Frame& frame)
{
    ELOG_TRACE("+++pushInput %d", input);

    m_inputs[input]->pushInput(frame);

    ELOG_TRACE("---pushInput %d", input);
}

bool MsdkVideoCompositor::addOutput(const uint32_t width, const uint32_t height, const uint32_t framerateFPS, owt_base::FrameDestination *dst)
{
    ELOG_DEBUG("addOutput, %dx%d, fps(%d), dst(%p)", width, height, framerateFPS, dst);

    for (auto& generator : m_generators) {
        if (generator->isSupported(width, height, framerateFPS)) {
            return generator->addOutput(width, height, framerateFPS, dst);
        }
    }

    ELOG_ERROR("Can not addOutput, %dx%d, fps(%d), dst(%p)", width, height, framerateFPS, dst);
    return false;
}

bool MsdkVideoCompositor::removeOutput(owt_base::FrameDestination *dst)
{
    ELOG_DEBUG("removeOutput, dst(%p)", dst);

    for (auto& generator : m_generators) {
        if (generator->removeOutput(dst)) {
            return true;
        }
    }

    ELOG_ERROR("Can not removeOutput, dst(%p)", dst);
    return false;
}

void MsdkVideoCompositor::flush()
{
    ELOG_DEBUG("flush");
    //todo, remove
#if 0
    boost::shared_ptr<MsdkFrame> frame;

    int i = 0;
    while(!(frame = m_framePool->getFreeFrame())) {
        i++;
        ELOG_DEBUG("flush - wait %d(ms)", i);
        usleep(1000); //1ms
    }

    ELOG_DEBUG("flush successfully after %d(ms)", i);
#endif
}

boost::shared_ptr<owt_base::MsdkFrame> MsdkVideoCompositor::getInputFrame(int index)
{
    boost::shared_ptr<owt_base::MsdkFrame> src;

    auto& input = m_inputs[index];
    if (input->isActivate()) {
        src = input->popInput();
    } else {
        src = m_avatarManager->getAvatarFrame(index);
    }

    return src;
}

}
#endif /* ENABLE_MSDK */
