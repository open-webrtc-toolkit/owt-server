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

#ifndef AVStreamOut_h
#define AVStreamOut_h

#include "MediaFramePipeline.h"

#include <EventRegistry.h>
#include <JobTimer.h>
#include <queue>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <rtputils.h>

namespace woogeen_base {

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

class EncodedFrame {
public:
    EncodedFrame(const uint8_t* data, size_t length, int64_t timeStamp, FrameFormat format)
        : m_timeStamp(timeStamp)
        , m_payloadData(nullptr)
        , m_payloadSize(length)
        , m_format(format)
    {
        // copy the encoded frame
        m_payloadData = new uint8_t[length];
        memcpy(m_payloadData, data, length);
    }

    ~EncodedFrame()
    {
        if (m_payloadData) {
            delete[] m_payloadData;
            m_payloadData = nullptr;
        }
    }

    int64_t m_timeStamp;
    uint8_t* m_payloadData;
    size_t m_payloadSize;
    FrameFormat m_format;
};

static const unsigned int DEFAULT_QUEUE_MAX = 10;

class MediaFrameQueue {
public:
    MediaFrameQueue(unsigned int max = DEFAULT_QUEUE_MAX)
        : m_max(max)
        , m_startTimeOffset(currentTimeMs())
    {
    }

    virtual ~MediaFrameQueue()
    {
    }

    void pushFrame(const uint8_t* data, size_t length, FrameFormat format = FRAME_FORMAT_UNKNOWN)
    {
        boost::mutex::scoped_lock lock(m_mutex);

        int64_t timestamp = currentTimeMs() - m_startTimeOffset;
        boost::shared_ptr<EncodedFrame> newFrame(new EncodedFrame(data, length, timestamp, format));
        m_queue.push(newFrame);

        if (m_queue.size() == 1)
            m_cond.notify_one();
    }

    boost::shared_ptr<EncodedFrame> popFrame(int timeout = 0)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        boost::shared_ptr<EncodedFrame> frame;

        if (m_queue.size() == 0 && timeout > 0) {
            m_cond.timed_wait(lock, boost::get_system_time() + boost::posix_time::milliseconds(timeout));
        }

        if (m_queue.size() > 0) {
            frame = m_queue.front();
            m_queue.pop();
        }

        return frame;
    }

    void cancel()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_cond.notify_all();
    }

private:
    // This queue can be used to store decoded frames
    std::queue<boost::shared_ptr<EncodedFrame>> m_queue;
    // The max size we allow the queue to grow before discarding frames
    unsigned int m_max;
    int64_t m_startTimeOffset;

    boost::mutex m_mutex;
    boost::condition_variable m_cond;
};

class AVStreamOut : public FrameDestination, public JobTimerListener, public EventRegistry {
public:
    enum Status {
        Context_CLOSED = -1,
        Context_EMPTY = 0,
        Context_INITIALIZING = 1,
        Context_READY = 2
    };

    AVStreamOut(EventRegistry* handle = nullptr)
        : m_status{ Context_EMPTY }
        , m_lastKeyFrameReqTime{ 0 }
        , m_asyncHandle{ handle }
    {
    }
    virtual ~AVStreamOut() {}

    void setEventRegistry(EventRegistry* handle) { m_asyncHandle = handle; }

    // FrameDestination
    virtual void onFrame(const Frame&) = 0;
    virtual void onVideoSourceChanged() {deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });}

    // JobTimerListener
    virtual void onTimeout() = 0;

protected:
    Status m_status;
    boost::scoped_ptr<JobTimer> m_jobTimer;
    int64_t m_lastKeyFrameReqTime;

    // EventRegistry
    virtual bool notifyAsyncEvent(const std::string& event, const std::string& data)
    {
        if (m_asyncHandle)
            return m_asyncHandle->notifyAsyncEvent(event, data);

        return false;
    }

    virtual bool notifyAsyncEventInEmergency(const std::string& event, const std::string& data)
    {
        if (m_asyncHandle)
            return m_asyncHandle->notifyAsyncEventInEmergency(event, data);

        return false;
    }

private:
    EventRegistry* m_asyncHandle;
};

} /* namespace woogeen_base */

#endif /* AVStreamOut_h */
