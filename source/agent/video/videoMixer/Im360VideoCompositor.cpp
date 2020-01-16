// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "Im360VideoCompositor.h"

#include "libyuv/convert.h"
#include "libyuv/scale.h"

#include <iostream>
#include <fstream>

#include <boost/make_shared.hpp>

#include <xcam_utils.h>
#include <soft/soft_video_buf_allocator.h>

using namespace webrtc;
using namespace owt_base;

namespace mcu {
namespace xcam {

#define STITCH_INPUT_IMAGE_COUNT  3
#define STITCH_FISHEYE_MAX_NUM    6

DEFINE_LOGGER(AvatarManager, "mcu.media.Im360VideoCompositor.AvatarManager");

AvatarManager::AvatarManager(uint8_t size)
    : m_size(size)
{
}

AvatarManager::~AvatarManager()
{
}

bool AvatarManager::getImageSize(const std::string &url, uint32_t *pWidth, uint32_t *pHeight)
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

boost::shared_ptr<webrtc::VideoFrame> AvatarManager::loadImage(const std::string &url)
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

    boost::shared_ptr<webrtc::VideoFrame> frame(new webrtc::VideoFrame(i420Buffer, webrtc::kVideoRotation_0, 0));

    delete [] image;

    return frame;
}

bool AvatarManager::setAvatar(uint8_t index, const std::string &url)
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

bool AvatarManager::unsetAvatar(uint8_t index)
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

boost::shared_ptr<webrtc::VideoFrame> AvatarManager::getAvatarFrame(uint8_t index)
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

    boost::shared_ptr<webrtc::VideoFrame> frame = loadImage(it->second);
    m_frames[it->second] = frame;
    return frame;
}

DEFINE_LOGGER(SoftInput, "mcu.media.Im360VideoCompositor.SoftInput");

SoftInput::SoftInput()
    : m_active(false)
{
    m_bufferManager.reset(new I420BufferManager(30));
    m_converter.reset(new owt_base::FrameConverter());
}

SoftInput::~SoftInput()
{
}

void SoftInput::setActive(bool active)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_active = active;
    if (!m_active)
        m_busyFrame.reset();
}

bool SoftInput::isActive(void)
{
    return m_active;
}

void SoftInput::pushInput(webrtc::VideoFrame *videoFrame)
{
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        if (!m_active)
            return;
    }

    rtc::scoped_refptr<webrtc::I420Buffer> dstBuffer = m_bufferManager->getFreeBuffer(videoFrame->width(), videoFrame->height());
    if (!dstBuffer) {
        ELOG_ERROR("No free buffer");
        return;
    }

    rtc::scoped_refptr<webrtc::VideoFrameBuffer> srcI420Buffer = videoFrame->video_frame_buffer();
    if (!m_converter->convert(srcI420Buffer, dstBuffer.get())) {
        ELOG_ERROR("I420Copy failed");
        return;
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        if (m_active)
            m_busyFrame.reset(new webrtc::VideoFrame(dstBuffer, webrtc::kVideoRotation_0, 0));
    }
}

boost::shared_ptr<VideoFrame> SoftInput::popInput()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    if(!m_active)
        return NULL;

    return m_busyFrame;
}

DEFINE_LOGGER(Im360FrameStitcher, "mcu.media.Im360VideoCompositor.Im360FrameStitcher");

XCam::SmartPtr<XCam::Stitcher> Im360FrameStitcher::createStitcher(Im360StitchParams::CamModel cam_model, XCam::StitchScopicMode scopic_mode)
{
    uint32_t fisheye_num = STITCH_INPUT_IMAGE_COUNT;

    VideoSize output_size = Im360StitchParams::outputImageSize(cam_model);

    XCam::GeoMapScaleMode scale_mode = XCam::ScaleSingleConst;
    XCam::FeatureMatchMode fm_mode = XCam::FMDefault;
    XCam::FeatureMatchStatus fm_status = XCam::FMStatusFMFirst;
    XCam::FisheyeDewarpMode dewarp_mode = XCam::DewarpSphere;

    uint32_t blend_pyr_levels = 1;
    uint32_t fm_frames = 100;

    XCam::SmartPtr<XCam::Stitcher> stitcher = XCam::Stitcher::create_soft_stitcher();
    if (stitcher.ptr()) {
        stitcher->set_camera_num(fisheye_num);
        stitcher->set_output_size(output_size.width, output_size.height);
        if (! m_stitchOutBufferPool.ptr()) {
            m_stitchOutBufferPool = createStitchBufferPool(output_size.width, output_size.height);
        }

        float* viewpoint_range = new float[STITCH_FISHEYE_MAX_NUM];
        Im360StitchParams::viewpointsRange(cam_model, viewpoint_range);
        stitcher->set_viewpoints_range(viewpoint_range);

        stitcher->set_dewarp_mode(dewarp_mode);
        stitcher->set_scopic_mode(scopic_mode);
        stitcher->set_scale_mode(scale_mode);
        stitcher->set_blend_pyr_levels(blend_pyr_levels);

        XCam::StitchInfo info = Im360StitchParams::softStitchInfo(cam_model, scopic_mode);
        stitcher->set_stitch_info(info);

        stitcher->set_fm_mode(fm_mode);
        stitcher->set_fm_frames(fm_frames);
        stitcher->set_fm_status(fm_status);

        XCam::FMConfig cfg = Im360StitchParams::softFmConfig(cam_model);
        stitcher->set_fm_config(cfg);
        XCam::FMRegionRatio ratio = Im360StitchParams::fmRegionRatio(cam_model);
        stitcher->set_fm_region_ratio(ratio);
    }

    return stitcher;
}

Im360FrameStitcher::Im360FrameStitcher(
            Im360VideoCompositor *owner,
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

    m_bufferManager.reset(new I420BufferManager(30));

    m_textDrawer.reset(new owt_base::FFmpegDrawText());

    //m_jobTimer.reset(new JobTimer(m_maxSupportedFps, this));
    m_jobTimer.reset(new JobTimer(30, this));

    m_jobTimer->start();

    m_stitcher = createStitcher(Im360StitchParams::CamD3C8K, XCam::ScopicStereoLeft);
    m_stitchBufferList.clear();

    xcam_set_log("xcam_stitch.log");
}

Im360FrameStitcher::~Im360FrameStitcher()
{
    ELOG_DEBUG_T("Exit");

    m_jobTimer->stop();

    m_stitchBufferList.clear();

    for (uint32_t i = 0; i <  m_outputs.size(); i++) {
        if (m_outputs[i].size())
            ELOG_WARN_T("Outputs not empty!!!");
    }

    m_stitcher.release();
}

void Im360FrameStitcher::updateLayoutSolution(LayoutSolution& solution)
{
    boost::unique_lock<boost::shared_mutex> lock(m_configMutex);

    m_newLayout         = solution;
    m_configureChanged  = true;
}

bool Im360FrameStitcher::isSupported(uint32_t width, uint32_t height, uint32_t fps)
{
    //if (fps > m_maxSupportedFps || fps < m_minSupportedFps){
    //    return false;
    //}

    m_minSupportedFps = fps;
    uint32_t n = m_minSupportedFps;
    while (n <= m_maxSupportedFps) {
        if (n == fps)
            return true;

        n *= 2;
    }

    return true;
}

bool Im360FrameStitcher::addOutput(const uint32_t width, const uint32_t height, const uint32_t fps, owt_base::FrameDestination *dst) {
    assert(isSupported(width, height, fps));

    boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);

    int index = m_maxSupportedFps / fps - 1;

    Output_t output{.width = width, .height = height, .fps = fps, .dest = dst};
    m_outputs[index].push_back(output);
    return true;
}

bool Im360FrameStitcher::removeOutput(owt_base::FrameDestination *dst) {
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

void Im360FrameStitcher::onTimeout()
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
        rtc::scoped_refptr<webrtc::VideoFrameBuffer> compositeBuffer = generateFrame();
        if (compositeBuffer) {
            webrtc::VideoFrame compositeFrame(
                    compositeBuffer,
                    webrtc::kVideoRotation_0,
                    m_clock->TimeInMilliseconds()
                    );
            compositeFrame.set_timestamp(compositeFrame.timestamp_us() * kMsToRtpTimestamp);

            owt_base::Frame frame;
            memset(&frame, 0, sizeof(frame));
            frame.format = owt_base::FRAME_FORMAT_I420;
            frame.payload = reinterpret_cast<uint8_t*>(&compositeFrame);
            frame.length = 0; // unused.
            frame.timeStamp = compositeFrame.timestamp();
            frame.additionalInfo.video.width = compositeFrame.width();
            frame.additionalInfo.video.height = compositeFrame.height();

            m_textDrawer->drawFrame(frame);

            {
                boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
                for (uint32_t i = 0; i <  m_outputs.size(); i++) {
                    if (m_counter % (i + 1))
                        continue;

                    for (auto it = m_outputs[i].begin(); it != m_outputs[i].end(); ++it) {
                        ELOG_TRACE_T("deliverFrame(%d), dst(%p), fps(%d), timestamp(%d)"
                                , m_counter, it->dest, m_maxSupportedFps / (i + 1), frame.timeStamp / 90);

                        uint32_t start_ts = m_clock->TimeInMilliseconds();
                        it->dest->onFrame(frame);
                        uint32_t end_ts = m_clock->TimeInMilliseconds();
                        ELOG_DEBUG("send stitch frame   time cost (ms) %d", end_ts - start_ts);
                    }
                }
            }
        }
    }

    m_counter = (m_counter + 1) % m_counterMax;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> Im360FrameStitcher::generateFrame()
{
    ELOG_TRACE_T("generate Frame");

    reconfigureIfNeeded();
    return layout();
}


void Im360FrameStitcher::convertNV12toI420(XCam::SmartPtr<XCam::VideoBuffer> nv12Frame, rtc::scoped_refptr<webrtc::I420Buffer> i420Frame)
{
    const XCam::VideoBufferInfo info = nv12Frame->get_video_info();
    XCam::VideoBufferPlanarInfo planar;

    uint8_t* nv12 = nv12Frame->map();
    info.get_planar_info(planar, 0);
    int width = planar.width;
    int height = planar.height;

    const uint8_t* src_y = nv12 + info.offsets[0];
    int src_stride_y = info.strides[0];

    const uint8_t* src_uv = src_y + info.offsets[1];
    int src_stride_uv = info.strides[1];

    uint8_t* dst_y = (uint8_t*)i420Frame->MutableDataY();
    uint32_t dst_stride_y = i420Frame->StrideY();
    uint8_t* dst_u = (uint8_t*)i420Frame->MutableDataU();
    uint32_t dst_stride_u = i420Frame->StrideU();
    uint8_t* dst_v = i420Frame->MutableDataV();
    uint32_t dst_stride_v = i420Frame->StrideV();

    int ret = libyuv::NV12ToI420(src_y, src_stride_y, src_uv, src_stride_uv,
               dst_y, dst_stride_y, dst_u, dst_stride_u, dst_v, dst_stride_v, width, height);

    if (ret != 0) {
        ELOG_ERROR("libyuv::NV12ToI420 failed, ret %d", ret);
    }

    nv12Frame->unmap();
}

XCam::SmartPtr<XCam::VideoBuffer> Im360FrameStitcher::getStitchInputBuffer(rtc::scoped_refptr<webrtc::VideoFrameBuffer> i420Frame)
{
    int width = i420Frame->width();
    int height = i420Frame->height();

    const uint8_t* src_y = i420Frame->DataY();
    int src_stride_y = i420Frame->StrideY();
    const uint8* src_u = i420Frame->DataU();
    int src_stride_u = i420Frame->StrideU();
    const uint8* src_v = i420Frame->DataV();
    int src_stride_v = i420Frame->StrideV();

    if (!m_stitchInBufferPool.ptr()) {
        const uint32_t reserveCount = 12;
        m_stitchInBufferPool = createStitchBufferPool(width, height, reserveCount);
    }

    XCam::SmartPtr<XCam::VideoBuffer> nv12Frame = m_stitchInBufferPool->get_buffer();
    uint8_t* nv12Buffer = nv12Frame->map();

    uint8_t* dst_y = nv12Buffer;
    int dst_stride_y = i420Frame->StrideY();
    uint8_t* dst_uv = nv12Buffer + width * height;
    int dst_stride_uv = i420Frame->StrideY();

    int ret = libyuv::I420ToNV12(src_y, src_stride_y, src_u, src_stride_u, src_v, src_stride_v,
               dst_y, dst_stride_y, dst_uv, dst_stride_uv, width, height);

    if (ret != 0) {
        ELOG_ERROR("libyuv::I420ToNV12 failed, ret %d", ret);
    }

    nv12Frame->unmap();

    return nv12Frame;
}

XCam::SmartPtr<XCam::BufferPool> Im360FrameStitcher::createStitchBufferPool(
    const uint32_t imageWidth,
    const uint32_t imageHeight,
    const uint32_t reserveCount)
{
    XCam::VideoBufferInfo inInfo;
    inInfo.init (V4L2_PIX_FMT_NV12, imageWidth, imageHeight);
    XCam::SmartPtr<XCam::BufferPool> bufferPool = new XCam::SoftVideoBufAllocator(inInfo);
    bufferPool->reserve (reserveCount);

    return bufferPool;
}

void Im360FrameStitcher::stitch_images(Im360FrameStitcher *t, rtc::scoped_refptr<webrtc::I420Buffer> compositeBuffer)
{
    if (m_layout.size() < STITCH_INPUT_IMAGE_COUNT || !m_stitcher.ptr()) {
        return;
    }

    for (LayoutSolution::const_iterator it = m_layout.begin(); it != m_layout.end(); ++it) {
        boost::shared_ptr<webrtc::VideoFrame> inputFrame = this->m_owner->getInputFrame(it->input);
        if (inputFrame == NULL || (uint32_t)it->input >= STITCH_INPUT_IMAGE_COUNT) {
            continue;
        }
        rtc::scoped_refptr<webrtc::VideoFrameBuffer> inputBuffer = inputFrame->video_frame_buffer();

        XCam::SmartPtr<XCam::VideoBuffer> stitchInBuf = XCam::external_buf_to_xcam_video_buf (
            (uint8_t*)inputBuffer->DataY(), V4L2_PIX_FMT_YUV420,
            inputBuffer->width(), inputBuffer->height(),
            inputBuffer->width(), inputBuffer->height(),
            inputBuffer->width() * inputBuffer->height() * 3 / 2);

        if ((uint32_t)it->input >= m_stitchBufferList.size()) {
            m_stitchBufferList.push_back(stitchInBuf);
        }
    }

    if (m_stitchBufferList.size() == STITCH_INPUT_IMAGE_COUNT) {
        XCam::SmartPtr<XCam::VideoBuffer> stitchOutBuf = XCam::external_buf_to_xcam_video_buf (
            (uint8_t*)compositeBuffer->DataY(), V4L2_PIX_FMT_YUV420,
            compositeBuffer->width(), compositeBuffer->height(),
            compositeBuffer->width(), compositeBuffer->height(),
            compositeBuffer->width() * compositeBuffer->height() * 3 / 2);

        uint32_t start_ts = m_clock->TimeInMilliseconds();
        XCamReturn ret = m_stitcher->stitch_buffers(m_stitchBufferList, stitchOutBuf);
        uint32_t end_ts = m_clock->TimeInMilliseconds();
        ELOG_DEBUG("libxcam stitch      time cost (ms) %d", end_ts - start_ts);

        if (XCAM_RETURN_NO_ERROR != ret) {
            ELOG_ERROR("libxcam stitch failed!!");
        }
        m_stitchBufferList.clear();
    }
}

void Im360FrameStitcher::layout_regions(Im360FrameStitcher *t, rtc::scoped_refptr<webrtc::I420Buffer> compositeBuffer, const LayoutSolution &regions)
{
    uint32_t composite_width = compositeBuffer->width();
    uint32_t composite_height = compositeBuffer->height();

    for (LayoutSolution::const_iterator it = regions.begin(); it != regions.end(); ++it) {
        boost::shared_ptr<webrtc::VideoFrame> inputFrame = t->m_owner->getInputFrame(it->input);
        if (inputFrame == NULL) {
            continue;
        }

        rtc::scoped_refptr<webrtc::VideoFrameBuffer> inputBuffer = inputFrame->video_frame_buffer();

        Region region = it->region;
        uint32_t dst_x      = (uint64_t)composite_width * region.area.rect.left.numerator / region.area.rect.left.denominator;
        uint32_t dst_y      = (uint64_t)composite_height * region.area.rect.top.numerator / region.area.rect.top.denominator;
        uint32_t dst_width  = (uint64_t)composite_width * region.area.rect.width.numerator / region.area.rect.width.denominator;
        uint32_t dst_height = (uint64_t)composite_height * region.area.rect.height.numerator / region.area.rect.height.denominator;

        if (dst_x + dst_width > composite_width)
            dst_width = composite_width - dst_x;

        if (dst_y + dst_height > composite_height)
            dst_height = composite_height - dst_y;

        uint32_t cropped_dst_width;
        uint32_t cropped_dst_height;
        uint32_t src_x;
        uint32_t src_y;
        uint32_t src_width;
        uint32_t src_height;
        if (t->m_crop) {
            src_width   = std::min((uint32_t)inputBuffer->width(), dst_width * inputBuffer->height() / dst_height);
            src_height  = std::min((uint32_t)inputBuffer->height(), dst_height * inputBuffer->width() / dst_width);
            src_x       = (inputBuffer->width() - src_width) / 2;
            src_y       = (inputBuffer->height() - src_height) / 2;

            cropped_dst_width   = dst_width;
            cropped_dst_height  = dst_height;
        } else {
            src_width   = inputBuffer->width();
            src_height  = inputBuffer->height();
            src_x       = 0;
            src_y       = 0;

            cropped_dst_width   = std::min(dst_width, inputBuffer->width() * dst_height / inputBuffer->height());
            cropped_dst_height  = std::min(dst_height, inputBuffer->height() * dst_width / inputBuffer->width());
        }

        dst_x += (dst_width - cropped_dst_width) / 2;
        dst_y += (dst_height - cropped_dst_height) / 2;

        src_x               &= ~1;
        src_y               &= ~1;
        src_width           &= ~1;
        src_height          &= ~1;
        dst_x               &= ~1;
        dst_y               &= ~1;
        cropped_dst_width   &= ~1;
        cropped_dst_height  &= ~1;

        int ret = libyuv::I420Scale(
                inputBuffer->DataY() + src_y * inputBuffer->StrideY() + src_x, inputBuffer->StrideY(),
                inputBuffer->DataU() + (src_y * inputBuffer->StrideU() + src_x) / 2, inputBuffer->StrideU(),
                inputBuffer->DataV() + (src_y * inputBuffer->StrideV() + src_x) / 2, inputBuffer->StrideV(),
                src_width, src_height,
                compositeBuffer->MutableDataY() + dst_y * compositeBuffer->StrideY() + dst_x, compositeBuffer->StrideY(),
                compositeBuffer->MutableDataU() + (dst_y * compositeBuffer->StrideU() + dst_x) / 2, compositeBuffer->StrideU(),
                compositeBuffer->MutableDataV() + (dst_y * compositeBuffer->StrideV() + dst_x) / 2, compositeBuffer->StrideV(),
                cropped_dst_width, cropped_dst_height,
                libyuv::kFilterBox);
        if (ret != 0)
            ELOG_ERROR("I420Scale failed, ret %d", ret);
    }
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> Im360FrameStitcher::layout()
{
    rtc::scoped_refptr<webrtc::I420Buffer> compositeBuffer = m_bufferManager->getFreeBuffer(m_size.width, m_size.height);
    if (!compositeBuffer) {
        ELOG_ERROR("No valid composite buffer");
        return NULL;
    }

    // Set the background color
    libyuv::I420Rect(
            compositeBuffer->MutableDataY(), compositeBuffer->StrideY(),
            compositeBuffer->MutableDataU(), compositeBuffer->StrideU(),
            compositeBuffer->MutableDataV(), compositeBuffer->StrideV(),
            0, 0, compositeBuffer->width(), compositeBuffer->height(),
            m_bgColor.y, m_bgColor.cb, m_bgColor.cr);

    if (m_layout.size() < STITCH_INPUT_IMAGE_COUNT) {
        layout_regions(this, compositeBuffer, m_layout);
    } else {
        stitch_images(this, compositeBuffer);
    }

    return compositeBuffer;
}

void Im360FrameStitcher::reconfigureIfNeeded()
{
    {
        boost::unique_lock<boost::shared_mutex> lock(m_configMutex);
        if (!m_configureChanged)
            return;

        m_layout = m_newLayout;
        m_configureChanged = false;
    }

    ELOG_DEBUG_T("reconfigure");
}

void Im360FrameStitcher::drawText(const std::string& textSpec)
{
    m_textDrawer->setText(textSpec);
    m_textDrawer->enable(true);
}

void Im360FrameStitcher::clearText()
{
    m_textDrawer->enable(false);
}

DEFINE_LOGGER(Im360VideoCompositor, "mcu.media.Im360VideoCompositor");

Im360VideoCompositor::Im360VideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, bool crop)
    : m_maxInput(maxInput)
{
    m_inputs.resize(m_maxInput);
    for (auto& input : m_inputs) {
        input.reset(new SoftInput());
    }

    m_avatarManager.reset(new AvatarManager(maxInput));

    m_stitcher.reset(new Im360FrameStitcher(this, rootSize, bgColor, crop, 48, 6));
}

Im360VideoCompositor::~Im360VideoCompositor()
{
    m_stitcher.reset();
    m_avatarManager.reset();
    m_inputs.clear();
}

void Im360VideoCompositor::updateRootSize(VideoSize& rootSize)
{
    ELOG_WARN("Not support updateRootSize: %dx%d", rootSize.width, rootSize.height);
}

void Im360VideoCompositor::updateBackgroundColor(YUVColor& bgColor)
{
    ELOG_WARN("Not support updateBackgroundColor: YCbCr(0x%x, 0x%x, 0x%x)", bgColor.y, bgColor.cb, bgColor.cr);
}

void Im360VideoCompositor::updateLayoutSolution(LayoutSolution& solution)
{
    assert(solution.size() <= m_maxInput);

    m_stitcher->updateLayoutSolution(solution);
}

bool Im360VideoCompositor::activateInput(int input)
{
    m_inputs[input]->setActive(true);
    return true;
}

void Im360VideoCompositor::deActivateInput(int input)
{
    m_inputs[input]->setActive(false);
}

bool Im360VideoCompositor::setAvatar(int input, const std::string& avatar)
{
    return m_avatarManager->setAvatar(input, avatar);
}

bool Im360VideoCompositor::unsetAvatar(int input)
{
    return m_avatarManager->unsetAvatar(input);
}

void Im360VideoCompositor::pushInput(int input, const Frame& frame)
{
    assert(frame.format == owt_base::FRAME_FORMAT_I420);
    webrtc::VideoFrame* i420Frame = reinterpret_cast<webrtc::VideoFrame*>(frame.payload);

    m_inputs[input]->pushInput(i420Frame);
}

bool Im360VideoCompositor::addOutput(const uint32_t width, const uint32_t height, const uint32_t framerateFPS, owt_base::FrameDestination *dst)
{
    if (m_stitcher->isSupported(width, height, framerateFPS)) {
        return m_stitcher->addOutput(width, height, framerateFPS, dst);
    }

    ELOG_ERROR("Can not addOutput, %dx%d, fps(%d), dst(%p)", width, height, framerateFPS, dst);
    return false;
}

bool Im360VideoCompositor::removeOutput(owt_base::FrameDestination *dst)
{
    if (m_stitcher->removeOutput(dst)) {
        return true;
    }

    ELOG_ERROR("Can not removeOutput, dst(%p)", dst);
    return false;
}

boost::shared_ptr<webrtc::VideoFrame> Im360VideoCompositor::getInputFrame(int index)
{
    boost::shared_ptr<webrtc::VideoFrame> src;

    auto& input = m_inputs[index];
    if (input->isActive()) {
        src = input->popInput();
    } else {
        src = m_avatarManager->getAvatarFrame(index);
    }

    return src;
}

void Im360VideoCompositor::drawText(const std::string& textSpec)
{
    m_stitcher->drawText(textSpec);
}

void Im360VideoCompositor::clearText()
{
    m_stitcher->clearText();
}

}
}
