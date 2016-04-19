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
#include <va/va.h>
#include <va/va_drm.h>
#include <VideoPostProcessHost.h>
#include <YamiVideoFrame.h>

using namespace webrtc;

namespace mcu {

DEFINE_LOGGER(YamiVideoCompositor, "mcu.media.YamiVideoCompositor");

//efine ERROR ELOG_ERROR

struct VADisplayDeleter
{
    VADisplayDeleter(int fd):m_fd(fd) {}
    void operator()(VADisplay* display)
    {
        vaTerminate(*display);
        delete display;
        close(m_fd);
    }
private:
    int m_fd;
};

template <class T>
class VideoPool : public EnableSharedFromThis<VideoPool<T> >
{
public:
    static SharedPtr<VideoPool<T> >
    create(std::deque<SharedPtr<T> >& buffers)
    {
        SharedPtr<VideoPool<T> > ptr(new VideoPool<T>(buffers));
        return ptr;
    }

    SharedPtr<T> alloc()
    {
        SharedPtr<T> ret;
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);

        if (!m_freed.empty()) {
            T* p = m_freed.front();
            m_freed.pop_front();
            ret.reset(p, Recycler(this->shared_from_this()));
        }
        return ret;
    }

private:

    VideoPool(std::deque<SharedPtr<T> >& buffers)
    {
            m_holder.swap(buffers);
            for (size_t i = 0; i < m_holder.size(); i++) {
                m_freed.push_back(m_holder[i].get());
            }
    }

    void recycle(T* ptr)
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
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



class PooledFrameAllocator
{
    DECLARE_LOGGER();
public:
    PooledFrameAllocator(const SharedPtr<VADisplay>& display, int poolsize):
        m_display(display), m_poolsize(poolsize)
    {
    }
    bool setFormat(uint32_t fourcc, int width, int height)
    {
        if (m_surfaces.size()) {
            ELOG_ERROR("you can only set format for once");
            return false;
        }
        m_surfaces.resize(m_poolsize);
/*        VASurfaceAttrib attrib;
        attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
        attrib.type = VASurfaceAttribPixelFormat;
        attrib.value.type = VAGenericValueTypeInteger;
        attrib.value.value.i = fourcc;
*/
        VAStatus status = vaCreateSurfaces(*m_display, VA_RT_FORMAT_YUV420, width, height,
                                           &m_surfaces[0], m_surfaces.size(),
                                           NULL, 0);
        ELOG_ERROR("rt format = %d", VA_RT_FORMAT_YUV420);
        if (status != VA_STATUS_SUCCESS) {
            ELOG_ERROR("create surface failed fourcc = %p, %s", fourcc, vaErrorStr(status));
            m_surfaces.clear();
            return false;
        }
        std::deque<SharedPtr<VideoFrame> > buffers;
        for (size_t i = 0;  i < m_surfaces.size(); i++) {
            SharedPtr<VideoFrame> f(new VideoFrame);
            memset(f.get(), 0, sizeof(VideoFrame));
            f->surface = (intptr_t)m_surfaces[i];
            buffers.push_back(f);
        }
        m_pool = VideoPool<VideoFrame>::create(buffers);
        return true;
    }

    SharedPtr<VideoFrame> alloc()
    {
        return  m_pool->alloc();
    }
    ~PooledFrameAllocator()
    {
        if (m_surfaces.size())
            vaDestroySurfaces(*m_display, &m_surfaces[0], m_surfaces.size());
    }
private:
    SharedPtr<VADisplay> m_display;
    std::vector<VASurfaceID> m_surfaces;
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
        memset(&m_rect, 0, sizeof(m_rect));
        memset(&m_rootSize, 0, sizeof(m_rootSize));

    }
    void updateRootSize(VideoSize& videoSize)
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
        m_rootSize = videoSize;
        updateRect();
    }

    void updateInputRegion(Region& region)
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
        m_region = region;
        updateRect();
    }
    void activate()
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
        m_active = true;
    }
    void deActivate()
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
        m_active = false;
        m_queue.clear();
    }
    void pushInput(const woogeen_base::Frame& frame)
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
        SharedPtr<VideoFrame> input = convert(frame);
        m_queue.push_back(input);
        if (m_queue.size() > 5 ) {
            m_queue.pop_front();
        }
    }
    SharedPtr<VideoFrame> popInput(VideoRect& rect)
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
        SharedPtr<VideoFrame> input;
        if (!m_active)
            return input;
        if (!m_queue.empty()) {
            input = m_queue.front();
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
        YamiVideoFrame* holder = (YamiVideoFrame*)frame.payload;
        return holder->frame;
    }
    bool m_active;
    std::deque<SharedPtr<VideoFrame> > m_queue;
    VideoSize m_rootSize;
    Region m_region;
    //dest rect
    VideoRect m_rect;
    boost::shared_mutex m_mutex;
};

DEFINE_LOGGER(VideoInput, "mcu.media.VideoInput");

YamiVideoCompositor::YamiVideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor)
    : m_compositeSize(rootSize)
    , m_bgColor(bgColor)
{
    ELOG_ERROR("set size to %dx%d, input = %d", rootSize.width, rootSize.height, maxInput);
    m_inputs.resize(maxInput);
    for (auto& input : m_inputs) {
        input.reset(new VideoInput());
        input->updateRootSize(rootSize);
    }

    m_display = createVADisplay();
    NativeDisplay nativeDisplay;
    nativeDisplay.type = NATIVE_DISPLAY_VA;
    nativeDisplay.handle = (intptr_t)*m_display;
    m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
    m_vpp->setNativeDisplay(nativeDisplay);

    m_allocator.reset(new PooledFrameAllocator(m_display, 5));

    if (!m_allocator->setFormat(YAMI_FOURCC('N', 'V', '1', '2'),
        m_compositeSize.width, m_compositeSize.width)) {
        ELOG_ERROR("set to %dx%d ", m_compositeSize.width, m_compositeSize.width);
    }
    m_jobTimer.reset(new woogeen_base::JobTimer(30, this));
    m_jobTimer->start();
    ELOG_ERROR("-set size to %dx%d, input = %d", rootSize.width, rootSize.height, maxInput);
}

YamiVideoCompositor::~YamiVideoCompositor()
{
    m_jobTimer->stop();
}

SharedPtr<VADisplay> YamiVideoCompositor::createVADisplay()
{
    SharedPtr<VADisplay> display;
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0)
        fd = open("/dev/dri/card0", O_RDWR);
    if (fd < 0) {
        ELOG_ERROR("can't open drm device");
        return display;
    }
    VADisplay vadisplay = vaGetDisplayDRM(fd);
    int majorVersion, minorVersion;
    VAStatus vaStatus = vaInitialize(vadisplay, &majorVersion, &minorVersion);
    if (vaStatus != VA_STATUS_SUCCESS) {
        ELOG_ERROR("va init failed, status =  %d", vaStatus);
        close(fd);
        return display;
    }
    display.reset(new VADisplay(vadisplay), VADisplayDeleter(fd));
    return display;
}


void YamiVideoCompositor::updateRootSize(VideoSize& videoSize)
{
    ELOG_ERROR("updateRootSize to %dx%d", videoSize.width, videoSize.height);
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    m_compositeSize = videoSize;

    for (auto& input : m_inputs) {
        input->updateRootSize(videoSize);
    }
    ELOG_ERROR("set size to %dx%d failed", videoSize.width, videoSize.height);
    if (!m_allocator->setFormat(YAMI_FOURCC('N', 'V', '1', '2'), videoSize.width, videoSize.height)) {
        ELOG_ERROR("set size to %dx%d failed", videoSize.width, videoSize.height);
    }
}

void YamiVideoCompositor::updateBackgroundColor(YUVColor& bgColor)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    m_bgColor = bgColor;
}

void YamiVideoCompositor::updateLayoutSolution(LayoutSolution& solution)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("Configuring layout");
    m_currentLayout = solution;
    for (auto& l : solution) {
        m_inputs[l.input]->updateInputRegion(l.region);
    }
}

bool YamiVideoCompositor::activateInput(int input)
{
    ELOG_ERROR("activateInput = %d", input);
    m_inputs[input]->activate();
    return true;
}

void YamiVideoCompositor::deActivateInput(int input)
{
    m_inputs[input]->deActivate();
}

void YamiVideoCompositor::pushInput(int input, const woogeen_base::Frame& frame)
{
    ELOG_ERROR("push input %d", input);
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    m_inputs[input]->pushInput(frame);
}

void YamiVideoCompositor::onTimeout()
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    generateFrame();
}

void YamiVideoCompositor::generateFrame()
{
    SharedPtr<VideoFrame> compositeFrame = layout();

    YamiVideoFrame holder;
    holder.frame = compositeFrame;

    woogeen_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = woogeen_base::FRAME_FORMAT_YAMI;
    frame.payload = reinterpret_cast<uint8_t*>(&holder);
    frame.length = 0; // unused.
    frame.additionalInfo.video.width = m_compositeSize.width;
    frame.additionalInfo.video.height = m_compositeSize.height;
    if (compositeFrame) {
      ELOG_ERROR("composite frame != null");
    } else {
      ELOG_ERROR("compiste frame = null");
    }

    deliverFrame(frame);
}

void YamiVideoCompositor::setBackgroundColor()
{
    ELOG_TRACE("setBackgroundColor");
}

SharedPtr<VideoFrame> YamiVideoCompositor::layout()
{
    // Update the background color
//    setBackgroundColor();
    return customLayout();
}

SharedPtr<VideoFrame> YamiVideoCompositor::customLayout()
{
    SharedPtr<VideoFrame> dest = m_allocator->alloc();
    if (!dest) {
        ELOG_TRACE("no frame");
    }
    for (auto& input : m_inputs) {
        SharedPtr<VideoFrame> src = input->popInput(dest->crop);
        if (src) {
            m_vpp->process(src, dest);
        }
    }
    return dest;
}


}
