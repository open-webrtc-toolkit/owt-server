/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#ifndef MediaMuxer_h
#define MediaMuxer_h

#include "MediaFramePipeline.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <Compiler.h>
#include <EventRegistry.h>
#include <JobTimer.h>
#include <rtputils.h>
#include <SharedQueue.h>

namespace woogeen_base {

static inline int64_t currentTimeMs() {
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

class EncodedFrame
{
public:
    EncodedFrame(const uint8_t* data, uint16_t length, int64_t timeStamp)
        : m_timeStamp(timeStamp)
        , m_payloadData(nullptr)
        , m_payloadSize(length)
    {
        // copy the encoded frame
        m_payloadData = new uint8_t[length];
        memcpy(m_payloadData, data, length);
    }

    ~EncodedFrame()
    {
        if (m_payloadData) {
            delete [] m_payloadData;
            m_payloadData = nullptr;
        }
    }

    int64_t m_timeStamp;
    uint8_t* m_payloadData;
    uint16_t m_payloadSize;
};

static const unsigned int DEFAULT_QUEUE_MAX = 10;

class MediaFrameQueue
{
public:
    MediaFrameQueue(unsigned int max = DEFAULT_QUEUE_MAX)
        : m_max(max)
        , m_startTimeOffset(currentTimeMs())
    {
    }

    virtual ~MediaFrameQueue()
    {
    }

    void pushFrame(const uint8_t* data, uint16_t length)
    {
        int64_t timestamp = currentTimeMs() - m_startTimeOffset;
        boost::shared_ptr<EncodedFrame> newFrame(new EncodedFrame(data, length, timestamp));
        m_queue.push(newFrame);

        // Enforce our max queue size
        boost::shared_ptr<EncodedFrame> frame;
        while (m_queue.size() > m_max)
            m_queue.pop(frame);
    }

    boost::shared_ptr<EncodedFrame> popFrame()
    {
        boost::shared_ptr<EncodedFrame> frame;
        m_queue.pop(frame);
        return frame;
    }

private:
    // This queue can be used to store decoded frames
    SharedQueue<boost::shared_ptr<EncodedFrame>> m_queue;
    // The max size we allow the queue to grow before discarding frames
    unsigned int m_max;
    int64_t m_startTimeOffset;
};

class MediaMuxer : public FrameConsumer, public JobTimerListener, public EventRegistry {
public:
    enum Status { Context_ERROR = -1, Context_EMPTY = 0, Context_READY = 1 };

    DLL_PUBLIC static MediaMuxer* createMediaMuxerInstance(const std::string& customParam);
    DLL_PUBLIC static bool recycleMediaMuxerInstance(const std::string& outputId);

    MediaMuxer() : m_status(Context_EMPTY) { }
    virtual ~MediaMuxer() { }

    DLL_PUBLIC virtual void setMediaSource(FrameProvider* videoProvider, FrameProvider* audioProvider) = 0;
    DLL_PUBLIC virtual void unsetMediaSource() = 0;
    DLL_PUBLIC void setEventRegistry(EventRegistry* handle) { m_asyncHandle = handle; }

    // FrameConsumer
    virtual void onFrame(const Frame&) = 0;

    // JobTimerListener
    virtual void onTimeout() = 0;

protected:
    Status m_status;
    boost::scoped_ptr<MediaFrameQueue> m_videoQueue;
    boost::scoped_ptr<MediaFrameQueue> m_audioQueue;
    boost::scoped_ptr<JobTimer> m_jobTimer;

    // EventRegistry
    virtual bool notifyAsyncEvent(const std::string& event, const std::string& data)
    {
        if (m_asyncHandle)
            return m_asyncHandle->notifyAsyncEvent(event, data);

        return false;
    }

private:
    EventRegistry* m_asyncHandle;
};

} /* namespace woogeen_base */

#endif /* MediaMuxer_h */
