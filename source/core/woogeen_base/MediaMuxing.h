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

#ifndef MediaMuxing_h
#define MediaMuxing_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <Compiler.h>
#include <SharedQueue.h>
#include <webrtc/video/encoded_frame_callback_adapter.h>
#include <webrtc/voice_engine/include/voe_base.h>

namespace woogeen_base {

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
    MediaFrameQueue(int64_t startTime, unsigned int max = DEFAULT_QUEUE_MAX)
        : m_max(max)
        , m_startTimeMsec(startTime)
        , m_timeOffsetMsec(-1)
    {
    }

    virtual ~MediaFrameQueue()
    {
    }

    void pushFrame(const uint8_t* data, uint16_t length, uint32_t timeStamp)
    {
        if (m_timeOffsetMsec == -1) {
            timeval time;
            gettimeofday(&time, nullptr);

            m_timeOffsetMsec = ((time.tv_sec * 1000) + (time.tv_usec / 1000)) - m_startTimeMsec;
        }

        boost::shared_ptr<EncodedFrame> newFrame(new EncodedFrame(data, length, timeStamp, m_timeOffsetMsec));
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
    int64_t m_startTimeMsec;
    int64_t m_timeOffsetMsec;
};

class VideoEncodedFrameCallbackAdapter : public webrtc::EncodedImageCallback {
public:
    VideoEncodedFrameCallbackAdapter(MediaFrameQueue* videoQueue)
        : m_videoQueue(videoQueue)
    {
    }

    virtual ~VideoEncodedFrameCallbackAdapter() { }

    virtual int32_t Encoded(const webrtc::EncodedImage& encodedImage,
                            const webrtc::CodecSpecificInfo* codecSpecificInfo,
                            const webrtc::RTPFragmentationHeader* fragmentation)
    {
        if (encodedImage._length > 0 && m_videoQueue)
            m_videoQueue->pushFrame(encodedImage._buffer, encodedImage._length, encodedImage._timeStamp);

        return 0;
    }

private:
    MediaFrameQueue* m_videoQueue;
};

class AudioEncodedFrameCallbackAdapter : public webrtc::AudioEncodedFrameCallback {
public:
    AudioEncodedFrameCallbackAdapter(MediaFrameQueue* audioQueue)
        : m_audioQueue(audioQueue)
    {
    }

    virtual ~AudioEncodedFrameCallbackAdapter() { }

    virtual int32_t Encoded(webrtc::FrameType frameType, uint8_t payloadType,
                            uint32_t timeStamp, const uint8_t* payloadData,
                            uint16_t payloadSize)
    {
        if (payloadSize > 0 && m_audioQueue)
            m_audioQueue->pushFrame(payloadData, payloadSize, timeStamp);

        return 0;
    }

private:
    MediaFrameQueue* m_audioQueue;
};


class MediaMuxing {
public:
    virtual ~MediaMuxing() {}
    virtual int32_t startMuxing(const std::string& uri, int codec, MediaFrameQueue& queue) = 0;
    virtual void stopMuxing(int32_t id) = 0;
    virtual bool getVideoSize(unsigned int& width, unsigned int& height) const = 0;
};

class MediaMuxer {
public:
    MediaMuxer() {}
    virtual ~MediaMuxer() {}
    virtual bool start() = 0;
    virtual void stop() = 0;
protected:
    bool m_muxing;
    boost::thread m_thread;
    int64_t m_firstVideoTimestamp;
    int64_t m_firstAudioTimestamp;
    boost::scoped_ptr<woogeen_base::MediaFrameQueue> m_videoQueue;
    boost::scoped_ptr<woogeen_base::MediaFrameQueue> m_audioQueue;
};

} /* namespace woogeen_base */

#endif /* MediaMuxing_h */
