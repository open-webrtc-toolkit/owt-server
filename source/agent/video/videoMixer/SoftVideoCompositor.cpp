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

#include "SoftVideoCompositor.h"

#include "libyuv/convert.h"
#include "libyuv/planar_functions.h"
#include "libyuv/scale.h"

#include <iostream>
#include <fstream>

using namespace webrtc;
using namespace woogeen_base;

namespace mcu {

DEFINE_LOGGER(AvatarManager, "mcu.media.SoftVideoCompositor.AvatarManager");

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

    delete image;

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

DEFINE_LOGGER(SwInput, "mcu.media.SwInput");

SwInput::SwInput()
    : m_active(false)
    , m_busyFrame(-1)
{
    m_frames.resize(2);
}

SwInput::~SwInput()
{

}

void SwInput::setActive(bool active)
{
    m_active = active;
}

void SwInput::pushInput(webrtc::VideoFrame *videoFrame)
{
    int ret;
    int index = (m_busyFrame + 1) % 2;
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> srcI420Buffer;
    boost::shared_ptr<webrtc::VideoFrame> dstFrame;
    webrtc::I420Buffer *dstI420Buffer;
    int width = videoFrame->width();
    int height = videoFrame->height();

    if (m_frames[index] == NULL
            || m_frames[index]->width() != width
            || m_frames[index]->height() != height) {
        m_frames[index].reset(new webrtc::VideoFrame(
                webrtc::I420Buffer::Create(width, height),
                webrtc::kVideoRotation_0,
                0));
    }

    dstFrame = m_frames[index];
    dstI420Buffer = static_cast<webrtc::I420Buffer *>(dstFrame->video_frame_buffer().get());
    srcI420Buffer = videoFrame->video_frame_buffer();

    ret = libyuv::I420Copy(
                srcI420Buffer->DataY(), srcI420Buffer->StrideY(),
                srcI420Buffer->DataU(), srcI420Buffer->StrideU(),
                srcI420Buffer->DataV(), srcI420Buffer->StrideV(),
                dstI420Buffer->MutableDataY(), dstI420Buffer->StrideY(),
                dstI420Buffer->MutableDataU(), dstI420Buffer->StrideU(),
                dstI420Buffer->MutableDataV(), dstI420Buffer->StrideV(),
                width, height);
    if (ret != 0) {
        ELOG_ERROR("I420Copy failed, ret %d", ret);
        return;
    }

    m_busyFrame = index;
}

boost::shared_ptr<VideoFrame> SwInput::popInput()
{
    if(m_busyFrame == -1)
        return NULL;

    return m_frames[m_busyFrame];
}

DEFINE_LOGGER(SoftVideoCompositor, "mcu.media.SoftVideoCompositor");

SoftVideoCompositor::SoftVideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, bool crop)
    : m_clock(Clock::GetRealTimeClock())
    , m_maxInput(maxInput)
    , m_compositeSize(rootSize)
    , m_bgColor(bgColor)
    , m_solutionState(UN_INITIALIZED)
    , m_crop(crop)
{
    m_compositeBuffer = webrtc::I420Buffer::Create(m_compositeSize.width, m_compositeSize.height);
    m_compositeFrame.reset(new webrtc::VideoFrame(
                m_compositeBuffer,
                webrtc::kVideoRotation_0,
                0));

    m_inputs.resize(m_maxInput);
    for (auto& input : m_inputs) {
        input.reset(new SwInput());
    }

    m_avatarManager.reset(new AvatarManager(maxInput));

    m_jobTimer.reset(new JobTimer(30, this));
    m_jobTimer->start();
}

SoftVideoCompositor::~SoftVideoCompositor()
{
    m_jobTimer->stop();
}

void SoftVideoCompositor::updateRootSize(VideoSize& videoSize)
{
    m_newCompositeSize = videoSize;
    m_solutionState = CHANGING;
}

void SoftVideoCompositor::updateBackgroundColor(YUVColor& bgColor)
{
    m_bgColor = bgColor;
}

void SoftVideoCompositor::updateLayoutSolution(LayoutSolution& solution)
{
    ELOG_DEBUG("Configuring layout");
    m_newLayout = solution;
    m_solutionState = CHANGING;
    ELOG_DEBUG("configChanged is true");
}

bool SoftVideoCompositor::activateInput(int input)
{
    m_inputs[input]->setActive(true);
    return true;
}

void SoftVideoCompositor::deActivateInput(int input)
{
    m_inputs[input]->setActive(false);
}

bool SoftVideoCompositor::setAvatar(int input, const std::string& avatar)
{
    return m_avatarManager->setAvatar(input, avatar);
}

bool SoftVideoCompositor::unsetAvatar(int input)
{
    return m_avatarManager->unsetAvatar(input);
}

void SoftVideoCompositor::pushInput(int input, const Frame& frame)
{
    assert(frame.format == woogeen_base::FRAME_FORMAT_I420);
    webrtc::VideoFrame* i420Frame = reinterpret_cast<webrtc::VideoFrame*>(frame.payload);

    m_inputs[input]->pushInput(i420Frame);
}

void SoftVideoCompositor::onTimeout()
{
    generateFrame();
}

void SoftVideoCompositor::generateFrame()
{
    VideoFrame* compositeFrame = layout();

    woogeen_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = woogeen_base::FRAME_FORMAT_I420;
    frame.payload = reinterpret_cast<uint8_t*>(compositeFrame);
    frame.length = 0; // unused.
    frame.timeStamp = compositeFrame->timestamp();
    frame.additionalInfo.video.width = compositeFrame->width();
    frame.additionalInfo.video.height = compositeFrame->height();

    deliverFrame(frame);
}

void SoftVideoCompositor::setBackgroundColor()
{
    if (m_compositeBuffer) {
        // Set the background color
        libyuv::I420Rect(
                m_compositeBuffer->MutableDataY(), m_compositeBuffer->StrideY(),
                m_compositeBuffer->MutableDataU(), m_compositeBuffer->StrideU(),
                m_compositeBuffer->MutableDataV(), m_compositeBuffer->StrideV(),
                0, 0, m_compositeBuffer->width(), m_compositeBuffer->height(),
                m_bgColor.y, m_bgColor.cb, m_bgColor.cr);
    }
}

bool SoftVideoCompositor::commitLayout()
{
    // Update the current video layout
    // m_compositeSize = m_newCompositeSize;
    m_currentLayout = m_newLayout;
    ELOG_DEBUG("commit customlayout");

    m_solutionState = IN_WORK;
    ELOG_DEBUG("configChanged sets to false after commitLayout!");
    return true;
}

webrtc::VideoFrame* SoftVideoCompositor::layout()
{
    if (m_solutionState == CHANGING)
        commitLayout();

    // Update the background color
    setBackgroundColor();
    return customLayout();
}

webrtc::VideoFrame* SoftVideoCompositor::customLayout()
{
    for (LayoutSolution::iterator it = m_currentLayout.begin(); it != m_currentLayout.end(); ++it) {
        int index = it->input;

        boost::shared_ptr<webrtc::VideoFrame> sub_image;
        if (m_inputs[index]->isActive()) {
            sub_image = m_inputs[index]->popInput();
        } else {
            sub_image = m_avatarManager->getAvatarFrame(index);
        }

        if (sub_image == NULL) {
            continue;
        }

        rtc::scoped_refptr<webrtc::VideoFrameBuffer> i420Buffer = sub_image->video_frame_buffer();

        Region region = it->region;

        assert(region.shape.compare("rectangle") == 0
                && (region.area.rect.left.denominator != 0 && region.area.rect.left.denominator >= region.area.rect.left.numerator)
                && (region.area.rect.top.denominator != 0 && region.area.rect.top.denominator >= region.area.rect.top.numerator)
                && (region.area.rect.width.denominator != 0 && region.area.rect.width.denominator >= region.area.rect.width.numerator)
                && (region.area.rect.height.denominator != 0 && region.area.rect.height.denominator >= region.area.rect.height.numerator));

        uint32_t sub_width      = (uint64_t)m_compositeSize.width * region.area.rect.width.numerator / region.area.rect.width.denominator;
        uint32_t sub_height     = (uint64_t)m_compositeSize.height * region.area.rect.height.numerator / region.area.rect.height.denominator;
        uint32_t offset_width   = (uint64_t)m_compositeSize.width * region.area.rect.left.numerator / region.area.rect.left.denominator;
        uint32_t offset_height  = (uint64_t)m_compositeSize.height * region.area.rect.top.numerator / region.area.rect.top.denominator;

        if (offset_width + sub_width > m_compositeSize.width)
            sub_width = m_compositeSize.width - offset_width;

        if (offset_height + sub_height > m_compositeSize.height)
            sub_height = m_compositeSize.height - offset_height;

        uint32_t cropped_sub_width;
        uint32_t cropped_sub_height;
        uint32_t src_x;
        uint32_t src_y;
        uint32_t src_width;
        uint32_t src_height;
        if (m_crop) {
            src_width   = std::min((uint32_t)i420Buffer->width(), sub_width * i420Buffer->height() / sub_height);
            src_height  = std::min((uint32_t)i420Buffer->height(), sub_height * i420Buffer->width() / sub_width);
            src_x       = (i420Buffer->width() - src_width) / 2;
            src_y       = (i420Buffer->height() - src_height) / 2;

            cropped_sub_width   = sub_width;
            cropped_sub_height  = sub_height;
        } else {
            src_width   = i420Buffer->width();
            src_height  = i420Buffer->height();
            src_x       = 0;
            src_y       = 0;

            cropped_sub_width   = std::min(sub_width, i420Buffer->width() * sub_height / i420Buffer->height());
            cropped_sub_height  = std::min(sub_height, i420Buffer->height() * sub_width / i420Buffer->width());
        }

        offset_width += (sub_width - cropped_sub_width) / 2;
        offset_height += (sub_height - cropped_sub_height) / 2;

        int ret = libyuv::I420Scale(
                i420Buffer->DataY() + src_y * i420Buffer->StrideY() + src_x, i420Buffer->StrideY(),
                i420Buffer->DataU() + (src_y * i420Buffer->StrideU() + src_x) / 2, i420Buffer->StrideU(),
                i420Buffer->DataV() + (src_y * i420Buffer->StrideV() + src_x) / 2, i420Buffer->StrideV(),
                src_width, src_height,
                m_compositeBuffer->MutableDataY() + offset_height * m_compositeBuffer->StrideY() + offset_width, m_compositeBuffer->StrideY(),
                m_compositeBuffer->MutableDataU() + (offset_height * m_compositeBuffer->StrideU() + offset_width) / 2, m_compositeBuffer->StrideU(),
                m_compositeBuffer->MutableDataV() + (offset_height * m_compositeBuffer->StrideV() + offset_width) / 2, m_compositeBuffer->StrideV(),
                cropped_sub_width, cropped_sub_height,
                libyuv::kFilterBox);
        if (ret != 0)
            ELOG_ERROR("I420Scale failed, ret %d", ret);
    }

    m_compositeFrame->set_timestamp_us(m_clock->TimeInMilliseconds());
    m_compositeFrame->set_timestamp(m_compositeFrame->timestamp_us() * kMsToRtpTimestamp);
    return m_compositeFrame.get();
}

}
