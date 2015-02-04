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

#ifndef MediaRecording_h
#define MediaRecording_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <Compiler.h>
#include <SharedQueue.h>

namespace mcu {

class EncodedFrame
{
public:
    EncodedFrame(const uint8_t* data, uint16_t length, uint32_t timeStamp, long long offsetMsec)
        : m_offsetMsec(offsetMsec), m_timeStamp(timeStamp)
        , m_payloadData(nullptr), m_payloadSize(length)
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

    long long m_offsetMsec;
    uint32_t m_timeStamp;
    uint8_t* m_payloadData;
    uint16_t m_payloadSize;
};

static const unsigned int DEFAULT_QUEUE_MAX = 10;

class MediaFrameQueue
{
public:
    MediaFrameQueue(unsigned int max = DEFAULT_QUEUE_MAX) : m_max(max) {}

    virtual ~MediaFrameQueue()
    {
        boost::shared_ptr<EncodedFrame> frame;
        while (m_queue.pop(frame)) {}
    }

    void pushFrame(const uint8_t* data, uint16_t length, uint32_t timeStamp, long long offsetMsec)
    {
        boost::shared_ptr<EncodedFrame> newFrame(new EncodedFrame(data, length, timeStamp, offsetMsec));
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
    // frames in this queue can be used to store decoded frame
    SharedQueue<boost::shared_ptr<EncodedFrame>> m_queue;

    // The max size we allow the queue to grow before discarding frames
    unsigned int m_max;
};

class MediaRecording {
public:
    virtual ~MediaRecording() {}

    virtual void startRecording(MediaFrameQueue& mediaQueue, long long recordStartTime) = 0;
    virtual void stopRecording() = 0;
};

} /* namespace mcu */

#endif /* MediaRecording_h */
