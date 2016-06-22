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
#include <deque>
#include <set>
/*#include <va/va.h>*/
#include <va/va_compat.h>
#include <VideoDisplay.h>
#include <VideoPostProcessHost.h>
#include <YamiVideoFrame.h>


using namespace webrtc;

namespace mcu {

DEFINE_LOGGER(YamiVideoCompositor, "mcu.media.YamiVideoCompositor");

template <class T>
class VideoPool : public EnableSharedFromThis<VideoPool<T> >
{
public:

    SharedPtr<T> alloc()
    {
        SharedPtr<T> ret;
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);

        if (!m_freed.empty()) {
            T* p = m_freed.front();
            m_freed.pop_front();
            ret.reset(p, Recycler(this->shared_from_this()));
        }
        return ret;
    }

    VideoPool(std::deque<SharedPtr<T> >& buffers)
    {
            m_holder.swap(buffers);
            for (size_t i = 0; i < m_holder.size(); i++) {
                m_freed.push_back(m_holder[i].get());
            }
    }

private:
    void recycle(T* ptr)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        m_freed.push_back(ptr);
    }

    class Recycler
    {
    public:
        Recycler(const SharedPtr<VideoPool<T> >& pool)
            :m_pool(pool)
        {
        }
        void operator()(T* ptr) const
        {
            m_pool->recycle(ptr);
        }
    private:
        SharedPtr<VideoPool<T> > m_pool;
    };

    boost::shared_mutex m_mutex;

    std::deque<T*> m_freed;
    std::deque<SharedPtr<T> > m_holder;
};

struct SurfaceDestoryer
{
private:
    boost::shared_ptr<VADisplay> m_display;
    std::vector<VASurfaceID> m_surfaces;
public:
    SurfaceDestoryer(const boost::shared_ptr<VADisplay>& display, std::vector<VASurfaceID>& surfaces)
        :m_display(display)
    {
        m_surfaces.swap(surfaces);
    }
    void operator()(VideoPool<VideoFrame>* pool)
    {
        if (m_surfaces.size())
            vaDestroySurfaces(*m_display, &m_surfaces[0], m_surfaces.size());
        delete pool;
    }

};

class PooledFrameAllocator
{
    DECLARE_LOGGER();
public:
    PooledFrameAllocator(const boost::shared_ptr<VADisplay>& display, int poolsize)
        : m_display(display)
        , m_poolsize(poolsize)
    {
    }
    bool setFormat(uint32_t fourcc, int width, int height)
    {
        std::vector<VASurfaceID> surfaces;
        surfaces.resize(m_poolsize);

        VASurfaceAttrib attrib;
        attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
#if 0
        attrib.type = VASurfaceAttribPixelFormat;
        attrib.value.type = VAGenericValueTypeInteger;
        attrib.value.value.i = fourcc;
#else
        VASurfaceAttribExternalBuffers external;
        memset(&external, 0, sizeof(external));
        external.pixel_format = fourcc;
        external.width = width;
        external.height = height;
        // external.num_planes = ?;
        // external.flags &= (~VA_SURFACE_EXTBUF_DESC_ENABLE_TILING);
        external.flags |= VA_SURFACE_EXTBUF_DESC_ENABLE_TILING;

        attrib.type = VASurfaceAttribExternalBufferDescriptor;
        attrib.value.type = VAGenericValueTypePointer;
        attrib.value.value.p = &external;
#endif

        VAStatus status = vaCreateSurfaces(*m_display, VA_RT_FORMAT_YUV420, width, height,
                                           &surfaces[0], surfaces.size(),
                                           &attrib, 1);

        if (status != VA_STATUS_SUCCESS) {
            ELOG_ERROR("create surface failed, %s", vaErrorStr(status));
            return false;
        }
        std::deque<SharedPtr<VideoFrame> > buffers;
        for (size_t i = 0;  i < surfaces.size(); i++) {
            SharedPtr<VideoFrame> f(new VideoFrame);
            memset(f.get(), 0, sizeof(VideoFrame));
            f->surface = (intptr_t)surfaces[i];
            f->fourcc = fourcc;
            buffers.push_back(f);
        }
        m_pool.reset(new VideoPool<VideoFrame>(buffers), SurfaceDestoryer(m_display, surfaces));
        return true;
    }

    SharedPtr<VideoFrame> alloc()
    {
        return  m_pool->alloc();
    }
private:
    boost::shared_ptr<VADisplay> m_display;
    SharedPtr<VideoPool<VideoFrame> > m_pool;
    int m_poolsize;
};

DEFINE_LOGGER(PooledFrameAllocator, "mcu.media.PooledFrameAllocator");

class VideoInput {
    DECLARE_LOGGER();
public:
    VideoInput()
        : m_active(false)
    {
        memset(&m_inputSize, 0, sizeof(m_inputSize));
        memset(&m_rect, 0, sizeof(m_rect));
        memset(&m_rootSize, 0, sizeof(m_rootSize));
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
        SharedPtr<VideoFrame> input = convert(frame);
        if (!input) //convert failed
            return;

        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        m_queue.push_back(input);
        if (m_queue.size() > kQueueSize) {
            m_queue.pop_front();
        }
    }
    SharedPtr<VideoFrame> popInput(VideoRect& rect)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        SharedPtr<VideoFrame> input;
        if (!m_active)
            return input;
        if (!m_queue.empty()) {
            input = m_queue.front();
            // Keep at least one frame for renderer
            if (!m_queue.size() > 1)
                m_queue.pop_front();
            rect = m_rect;
        }
        return input;
    }
private:
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
        m_rect.x = offset_width;
        m_rect.y = offset_height;
        m_rect.width = sub_width;
        m_rect.height = sub_height;
    }
    SharedPtr<VideoFrame> convert(const woogeen_base::Frame& frame)
    {
        switch (frame.format) {
        case FRAME_FORMAT_YAMI: {
            YamiVideoFrame* holder = (YamiVideoFrame*)frame.payload;
            return holder->frame;
        }
        case FRAME_FORMAT_I420: {
            if (!m_allocator) {
                boost::shared_ptr<VADisplay> vaDisplay = GetVADisplay();
                m_allocator.reset(new PooledFrameAllocator(vaDisplay, kQueueSize + kSoftwareExtraSize));
            }

            // The resolution of the input can be changed on the fly,
            // for example the Chrome browser may scale down a VGA (640x480)
            // video stream to 320x240 if network condition is not good.
            // In such case we need to reset the format of the allocator.
            if (m_inputSize.width != frame.additionalInfo.video.width || m_inputSize.height != frame.additionalInfo.video.height) {
                m_inputSize.width = frame.additionalInfo.video.width;
                m_inputSize.height = frame.additionalInfo.video.height;
                if (!m_allocator->setFormat(YAMI_FOURCC('Y', 'V', '1', '2'),
                    m_inputSize.width, m_inputSize.height)) {
                    ELOG_DEBUG("set to %dx%d failed", m_inputSize.width, m_inputSize.height);
                }
            }

            SharedPtr<VideoFrame> target = m_allocator->alloc();
            if (!target)
                return target;

            YamiVideoFrame yamiFrame;
            yamiFrame.frame = target;
            webrtc::I420VideoFrame* i420Frame = reinterpret_cast<webrtc::I420VideoFrame*>(frame.payload);

            if (!yamiFrame.convertFromI420VideoFrame(*i420Frame))
                return SharedPtr<VideoFrame>();

            return yamiFrame.frame;
        }
        default:
            return SharedPtr<VideoFrame>();
        }

        return SharedPtr<VideoFrame>();
    }

    bool m_active;
    std::deque<SharedPtr<VideoFrame> > m_queue;
    VideoSize m_inputSize;
    VideoSize m_rootSize;
    Region m_region;
    VideoRect m_rect;
    boost::shared_mutex m_mutex;
    SharedPtr<PooledFrameAllocator> m_allocator;
    const static size_t kQueueSize = 5;
    //extra size for i420 convert
    const static size_t kSoftwareExtraSize = 3;
};

DEFINE_LOGGER(VideoInput, "mcu.media.VideoInput");


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
            m_allocator.reset(new PooledFrameAllocator(m_display, 1));
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
    SharedPtr<PooledFrameAllocator> m_allocator;
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
    for (auto& input : m_inputs) {
        input.reset(new VideoInput());
        input->updateRootSize(rootSize);
    }

    m_display = GetVADisplay();
    NativeDisplay nativeDisplay;
    nativeDisplay.type = NATIVE_DISPLAY_VA;
    nativeDisplay.handle = (intptr_t)*m_display;
    m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
    m_vpp->setNativeDisplay(nativeDisplay);

    m_allocator.reset(new PooledFrameAllocator(m_display, 5));

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

void YamiVideoCompositor::updateRootSize(VideoSize& videoSize)
{
    ELOG_DEBUG("updateRootSize to %dx%d", videoSize.width, videoSize.height);
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_compositeSize = videoSize;

    for (auto& input : m_inputs) {
        input->updateRootSize(videoSize);
    }
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
    for (auto& l : solution) {
        m_inputs[l.input]->updateInputRegion(l.region);
    }
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
        SharedPtr<VideoFrame> src = input->popInput(dest->crop);
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
