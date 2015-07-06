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
#include <rtputils.h>
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
    VideoEncodedFrameCallbackAdapter(FrameConsumer* consumer)
        : m_frameConsumer(consumer)
    {
    }

    virtual ~VideoEncodedFrameCallbackAdapter() { }

    virtual int32_t Encoded(const webrtc::EncodedImage& encodedImage,
                            const webrtc::CodecSpecificInfo* codecSpecificInfo,
                            const webrtc::RTPFragmentationHeader* fragmentation)
    {
        if (encodedImage._length > 0 && m_frameConsumer) {
            FrameFormat format = FRAME_FORMAT_UNKNOWN;
            switch (codecSpecificInfo->codecType) {
            case webrtc::kVideoCodecVP8:
                format = FRAME_FORMAT_VP8;
                break;
            case webrtc::kVideoCodecH264:
                format = FRAME_FORMAT_H264;
                break;
            default:
                break;
            }

            Frame frame;
            memset(&frame, 0, sizeof(frame));
            frame.format = format;
            frame.payload = encodedImage._buffer;
            frame.length = encodedImage._length;
            frame.timeStamp = encodedImage._timeStamp;
            frame.additionalInfo.video.width = encodedImage._encodedWidth;
            frame.additionalInfo.video.height = encodedImage._encodedHeight;

            m_frameConsumer->onFrame(frame);
        }

        return 0;
    }

private:
    FrameConsumer* m_frameConsumer;
};

class AudioEncodedFrameCallbackAdapter : public webrtc::AudioEncodedFrameCallback {
public:
    AudioEncodedFrameCallbackAdapter(FrameConsumer* consumer)
        : m_frameConsumer(consumer)
    {
    }

    virtual ~AudioEncodedFrameCallbackAdapter() { }

    virtual int32_t Encoded(webrtc::FrameType frameType, uint8_t payloadType,
                            uint32_t timeStamp, const uint8_t* payloadData,
                            uint16_t payloadSize)
    {
        if (payloadSize > 0 && m_frameConsumer) {
            FrameFormat format = FRAME_FORMAT_UNKNOWN;
            Frame frame;
            memset(&frame, 0, sizeof(frame));
            switch (payloadType) {
            case PCMU_8000_PT:
                format = FRAME_FORMAT_PCMU;
                frame.additionalInfo.audio.channels = 1; // FIXME: retrieve codec info from VOE?
                frame.additionalInfo.audio.sampleRate = 8000;
                break;
            case OPUS_48000_PT:
                format = FRAME_FORMAT_OPUS;
                frame.additionalInfo.audio.channels = 2;
                frame.additionalInfo.audio.sampleRate = 48000;
                break;
            default:
                break;
            }

            frame.format = format;
            frame.payload = const_cast<uint8_t*>(payloadData);
            frame.length = payloadSize;
            frame.timeStamp = timeStamp;

            m_frameConsumer->onFrame(frame);
        }

        return 0;
    }

private:
    FrameConsumer* m_frameConsumer;
};


class FrameDispatcher {
public:
    virtual ~FrameDispatcher() { }
    virtual int32_t addFrameConsumer(const std::string& name, int payloadType, FrameConsumer*) = 0;
    virtual void removeFrameConsumer(int32_t id) = 0;
};

class MediaMuxer : public FrameConsumer {
public:
    enum Status { Context_ERROR = -1, Context_EMPTY = 0, Context_READY = 1 };
    MediaMuxer(EventRegistry* registry = nullptr) : m_muxing(false), m_status(Context_EMPTY), m_firstVideoTimestamp(-1), m_firstAudioTimestamp(-1), m_callback(registry), m_callbackCalled(false) { }
    virtual ~MediaMuxer() { if (m_callback) delete m_callback; }
    virtual bool setMediaSource(FrameDispatcher* videoDispatcher, FrameDispatcher* audioDispatcher) = 0;
    virtual void unsetMediaSource() = 0;

protected:
    bool m_muxing;
    Status m_status;
    int64_t m_firstVideoTimestamp;
    int64_t m_firstAudioTimestamp;
    boost::thread m_thread;
    boost::scoped_ptr<woogeen_base::MediaFrameQueue> m_videoQueue;
    boost::scoped_ptr<woogeen_base::MediaFrameQueue> m_audioQueue;
    void callback(const std::string& data) { // executed *ONCE*, should be called by m_thread only;
        if (m_callback && (!m_callbackCalled)) {
            m_callbackCalled = true;
            m_callback->notify(data);
        }
    }
private:
    EventRegistry* m_callback;
    bool m_callbackCalled;
};

} /* namespace woogeen_base */

#endif /* MediaMuxer_h */
