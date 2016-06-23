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

#include "YamiVideoCompositor.h"

#include <webrtc/system_wrappers/interface/clock.h>
#include <webrtc/system_wrappers/interface/tick_util.h>
#include <set>
/*#include <va/va.h>*/
#include <va/va_compat.h>
#include <VideoDisplay.h>
#include <VideoPostProcessHost.h>
#include <YamiVideoFrame.h>
#include <YamiVideoInputManager.h>

using namespace webrtc;

namespace mcu {

DEFINE_LOGGER(YamiVideoCompositor, "mcu.media.YamiVideoCompositor");

class BackgroundCleaner {
    DECLARE_LOGGER();
public:
    BackgroundCleaner(const boost::shared_ptr<VADisplay>& display, const VideoSize& videoSize, const YUVColor& bgColor)
        : m_display(display)
        , m_videoSize(videoSize)
        , m_bgColor(bgColor)
    {
    }
    void setColor(YUVColor bgColor)
    {
        m_bgColor = bgColor;
        m_background.reset();
    }
    void updateRootSize(VideoSize& videoSize)
    {
        m_videoSize = videoSize;
        m_background.reset();
        m_allocator.reset();
    }
    void layoutSolutionUpdated()
    {
        m_cleared.clear();
    }
    bool clearBackground(const SharedPtr<VideoFrame>& frame)
    {
        if (!frame)
            return false;
        ELOG_DEBUG("clearBackground %p", (void*)frame->surface);
        //cleared before?
        if (m_cleared.find(frame->surface) != m_cleared.end())
            return true;

        if (!init())
            return false;
        m_vpp->process(m_background, frame);

        //book it
        m_cleared.insert(frame->surface);
        return true;
    }

private:
    static bool clearSurface(const SharedPtr<VideoFrame>& frame, YUVColor& bgColor)
    {
        ELOG_DEBUG("clearSurface %p", (void*)frame->surface);
        VAImage image;
        uint8_t* p = mapVASurfaceToVAImage(frame->surface, image);
        if (!p)
            return false;
        uint8_t* s =  p;
        //y
        for (int i = 0; i < image.height; i++) {
            memset(s, bgColor.y, image.width);
            s += image.pitches[0];
        }
        //uv
        s = p + image.offsets[1];
        for (int i = 0; i < image.height / 2; i++) {
            for (int j = 0; j < image.width; j += 2) {
                s[j] = bgColor.cb;
                s[j+1] = bgColor.cr;
            }
            s += image.pitches[1];;
        }
        unmapVAImage(image);
        return true;
    }

    bool init()
    {
        if (!m_allocator) {
            m_allocator.reset(new woogeen_base::PooledFrameAllocator(m_display, 1));
            if (!m_allocator->setFormat(YAMI_FOURCC('N', 'V', '1', '2'),
                m_videoSize.width, m_videoSize.height )) {
                return false;
            }
        }
        if (!m_background) {
            m_background = m_allocator->alloc();
            if (!m_background)
                return false;
            if (!clearSurface(m_background, m_bgColor))
                return false;

        }

        if (!m_vpp) {
            NativeDisplay nativeDisplay;
            nativeDisplay.type = NATIVE_DISPLAY_VA;
            nativeDisplay.handle = (intptr_t)*m_display;
            m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
            m_vpp->setNativeDisplay(nativeDisplay);
        }
        return true;
    }
    boost::shared_ptr<VADisplay> m_display;
    SharedPtr<YamiMediaCodec::IVideoPostProcess> m_vpp;
    SharedPtr<woogeen_base::PooledFrameAllocator> m_allocator;
    SharedPtr<VideoFrame> m_background;
    VideoSize m_videoSize;
    YUVColor m_bgColor;
    std::set<intptr_t> m_cleared;
};

DEFINE_LOGGER(BackgroundCleaner, "mcu.media.BackgroundCleaner");

YamiVideoCompositor::YamiVideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor)
    : m_compositeSize(rootSize)
    , m_bgColor(0xff000000)
{
    m_ntpDelta = Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() - TickTime::MillisecondTimestamp();
    ELOG_DEBUG("set size to %dx%d, input = %d", rootSize.width, rootSize.height, maxInput);
    m_inputs.resize(maxInput);
    m_inputRects.resize(maxInput);
    for (auto& input : m_inputs)
        input.reset(new woogeen_base::YamiVideoInputManager());

    m_display = GetVADisplay();
    NativeDisplay nativeDisplay;
    nativeDisplay.type = NATIVE_DISPLAY_VA;
    nativeDisplay.handle = (intptr_t)*m_display;
    m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
    m_vpp->setNativeDisplay(nativeDisplay);

    m_allocator.reset(new woogeen_base::PooledFrameAllocator(m_display, 5));

    if (!m_allocator->setFormat(YAMI_FOURCC('Y', 'V', '1', '2'),
        m_compositeSize.width, m_compositeSize.height)) {
        ELOG_DEBUG("set to %dx%d failed", m_compositeSize.width, m_compositeSize.height);
    }

    m_backgroundCleaner.reset(new BackgroundCleaner(m_display, rootSize, bgColor));
    m_jobTimer.reset(new woogeen_base::JobTimer(30, this));
    m_jobTimer->start();
    ELOG_DEBUG("set size to %dx%d, input = %d", rootSize.width, rootSize.height, maxInput);
}

YamiVideoCompositor::~YamiVideoCompositor()
{
    m_jobTimer->stop();
}

static void updateRect(const VideoSize& rootSize, const Region& region, VideoRect& rect)
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

    rect.x = offset_width;
    rect.y = offset_height;
    rect.width = sub_width;
    rect.height = sub_height;
}

void YamiVideoCompositor::updateRootSize(VideoSize& videoSize)
{
    ELOG_DEBUG("updateRootSize to %dx%d", videoSize.width, videoSize.height);
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_compositeSize = videoSize;

    for (auto& l : m_currentLayout)
        updateRect(videoSize, l.region, m_inputRects[l.input]);

    ELOG_DEBUG("set size to %dx%d failed", videoSize.width, videoSize.height);
    if (!m_allocator->setFormat(YAMI_FOURCC('Y', 'V', '1', '2'), videoSize.width, videoSize.height)) {
        ELOG_DEBUG("set size to %dx%d failed", videoSize.width, videoSize.height);
    }
    m_backgroundCleaner->updateRootSize(videoSize);
}

void YamiVideoCompositor::updateBackgroundColor(YUVColor& bgColor)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_bgColor = 0xff000000;
    m_backgroundCleaner->setColor(bgColor);
}

void YamiVideoCompositor::updateLayoutSolution(LayoutSolution& solution)
{
    ELOG_DEBUG("Configuring layout");

    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_currentLayout = solution;
    for (auto& l : solution)
        updateRect(m_compositeSize, l.region, m_inputRects[l.input]);

    m_backgroundCleaner->layoutSolutionUpdated();
}

bool YamiVideoCompositor::activateInput(int input)
{
    ELOG_DEBUG("activateInput = %d", input);
    m_inputs[input]->activate();
    return true;
}

void YamiVideoCompositor::deActivateInput(int input)
{
    m_inputs[input]->deActivate();
}

void YamiVideoCompositor::pushInput(int input, const woogeen_base::Frame& frame)
{
    ELOG_DEBUG("push input %d", input);
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_inputs[input]->pushInput(frame);
}

void YamiVideoCompositor::onTimeout()
{
    generateFrame();
}

void YamiVideoCompositor::generateFrame()
{
    SharedPtr<VideoFrame> compositeFrame = layout();
    if (!compositeFrame)
        return;

    YamiVideoFrame holder;
    holder.frame = compositeFrame;

    const int kMsToRtpTimestamp = 90;
    holder.frame->timeStamp = kMsToRtpTimestamp *
        (TickTime::MillisecondTimestamp() + m_ntpDelta);

    woogeen_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = woogeen_base::FRAME_FORMAT_YAMI;
    frame.payload = reinterpret_cast<uint8_t*>(&holder);
    frame.length = 0; // unused.
    frame.timeStamp = holder.frame->timeStamp;
    frame.additionalInfo.video.width = m_compositeSize.width;
    frame.additionalInfo.video.height = m_compositeSize.height;

    deliverFrame(frame);
}

void YamiVideoCompositor::clearBackgroud(const SharedPtr<VideoFrame>& dest)
{
    m_backgroundCleaner->clearBackground(dest);
}

SharedPtr<VideoFrame> YamiVideoCompositor::layout()
{
    return customLayout();
}

SharedPtr<VideoFrame> YamiVideoCompositor::customLayout()
{
    SharedPtr<VideoFrame> dest = m_allocator->alloc();
    if (!dest) {
        return dest;
    }
    clearBackgroud(dest);

    bool gotFrame = false;

    for (size_t i = 0; i < m_inputs.size(); i++) {
        auto& input = m_inputs[i];
        SharedPtr<VideoFrame> src = input->popInput();
        dest->crop = m_inputRects[i];
        if (src) {
            m_vpp->process(src, dest);
            ELOG_DEBUG("(%d, %d, %d, %d) to (%d, %d, %d, %d)", src->crop.x, src->crop.y, src->crop.width, src->crop.height,
                dest->crop.x, dest->crop.y, dest->crop.width, dest->crop.height);

            gotFrame = true;
        }
    }
    // Never got a frame, return null.
    if (!gotFrame)
        dest.reset();
    return dest;
}


}
