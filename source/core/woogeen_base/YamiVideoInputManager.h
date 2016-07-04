/*
 * Copyright 2016 Intel Corporation All Rights Reserved.
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

#ifndef YamiVideoInputManager_h
#define YamiVideoInputManager_h

#include "MediaFramePipeline.h"
#include "VideoDisplay.h"
#include "VideoHelper.h"
#include "YamiVideoFrame.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <deque>
#include <logger.h>
#include <va/va_compat.h>
#include <VideoPostProcessHost.h>

namespace woogeen_base {

template <class T>
class YamiVideoFramePool : public EnableSharedFromThis<YamiVideoFramePool<T> > {
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

    YamiVideoFramePool(std::deque<SharedPtr<T> >& buffers)
    {
        m_holder.swap(buffers);
        for (size_t i = 0; i < m_holder.size(); i++)
            m_freed.push_back(m_holder[i].get());
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
        Recycler(const SharedPtr<YamiVideoFramePool<T> >& pool)
            :m_pool(pool)
        {
        }
        void operator()(T* ptr) const
        {
            m_pool->recycle(ptr);
        }
    private:
        SharedPtr<YamiVideoFramePool<T> > m_pool;
    };

    boost::shared_mutex m_mutex;

    std::deque<T*> m_freed;
    std::deque<SharedPtr<T> > m_holder;
};

struct SurfaceDestoryer {
private:
    boost::shared_ptr<VADisplay> m_display;
    std::vector<VASurfaceID> m_surfaces;
public:
    SurfaceDestoryer(const boost::shared_ptr<VADisplay>& display, std::vector<VASurfaceID>& surfaces)
        :m_display(display)
    {
        m_surfaces.swap(surfaces);
    }
    void operator()(YamiVideoFramePool<VideoFrame>* pool)
    {
        if (m_surfaces.size())
            vaDestroySurfaces(*m_display, &m_surfaces[0], m_surfaces.size());
        delete pool;
    }

};

class PooledFrameAllocator {
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
        m_pool.reset(new YamiVideoFramePool<VideoFrame>(buffers), SurfaceDestoryer(m_display, surfaces));
        return true;
    }

    SharedPtr<VideoFrame> alloc()
    {
        return  m_pool->alloc();
    }
private:
    boost::shared_ptr<VADisplay> m_display;
    SharedPtr<YamiVideoFramePool<VideoFrame> > m_pool;
    int m_poolsize;
};

class YamiVideoInputManager {
    DECLARE_LOGGER();
public:
    YamiVideoInputManager()
        : m_active(false)
    {
        memset(&m_inputSize, 0, sizeof(m_inputSize));
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
    SharedPtr<VideoFrame> popInput()
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
        }
        return input;
    }
private:
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
    boost::shared_mutex m_mutex;
    SharedPtr<PooledFrameAllocator> m_allocator;
    const static size_t kQueueSize = 5;
    //extra size for i420 convert
    const static size_t kSoftwareExtraSize = 3;
};

}

#endif // YamiVideoInputManager_h
