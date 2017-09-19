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

#include "FrameProcesser.h"

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(FrameProcesser, "woogeen.FrameProcesser");

FrameProcesser::FrameProcesser()
    : m_lastWidth(0)
    , m_lastHeight(0)
    , m_format(FRAME_FORMAT_UNKNOWN)
    , m_outWidth(-1)
    , m_outHeight(-1)
    , m_outFrameRate(-1)
    , m_clock(NULL)
{
}

FrameProcesser::~FrameProcesser()
{
    if (m_outFrameRate > 0) {
        m_jobTimer->stop();
    }
}

bool FrameProcesser::init(FrameFormat format, const uint32_t width, const uint32_t height, const uint32_t frameRate)
{
    ELOG_DEBUG_T("format(%s), size(%dx%d), frameRate(%d)", getFormatStr(format), width, height, frameRate);

    if (format != FRAME_FORMAT_MSDK && format != FRAME_FORMAT_I420) {
        ELOG_ERROR_T("Invalid format(%d)", format);
        return false;
    }

#ifdef ENABLE_MSDK
    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase == NULL) {
        ELOG_ERROR_T("Get MSDK failed.");
        return false;
    }

    m_allocator = msdkBase->createFrameAllocator();
    if (!m_allocator) {
        ELOG_ERROR_T("Create frame allocator failed.");
        return false;
    }

    m_framePool.resize(3);
#endif

    m_format = format;
    m_outWidth = width;
    m_outHeight = height;
    m_outFrameRate = frameRate;

    m_converter.reset(new FrameConverter());

    if (m_format == FRAME_FORMAT_I420)
        m_bufferManager.reset(new I420BufferManager(3));

    if (m_outFrameRate != 0) {
        m_clock = Clock::GetRealTimeClock();

        m_jobTimer.reset(new JobTimer(m_outFrameRate, this));
        m_jobTimer->start();
    }

    return true;
}

bool FrameProcesser::filterFrame(const Frame& frame)
{
#ifdef ENABLE_MSDK
    if (frame.format == FRAME_FORMAT_MSDK) {
        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
        if (holder->cmd != MsdkCmd_NONE)
            return true;
    }
#endif

    if (m_lastWidth != frame.additionalInfo.video.width
            || m_lastHeight != frame.additionalInfo.video.height) {
        ELOG_DEBUG_T("Stream(%s) resolution changed, %dx%d -> %dx%d"
                , getFormatStr(frame.format)
                , m_lastWidth, m_lastHeight
                , frame.additionalInfo.video.width, frame.additionalInfo.video.height
                );

        m_lastWidth = frame.additionalInfo.video.width;
        m_lastHeight = frame.additionalInfo.video.height;
    }

    return false;
}

#ifdef ENABLE_MSDK
boost::shared_ptr<woogeen_base::MsdkFrame> FrameProcesser::getMsdkFrame(const uint32_t width, const uint32_t height)
{
    int32_t nullIndex = -1;
    int32_t freeIndex = -1;

    for(uint32_t i = 0; i < m_framePool.size(); i++) {
        if (!m_framePool[i]) {
            nullIndex = i;
            continue;
        }

        if(m_framePool[i].use_count() == 1 && m_framePool[i]->isFree()) {
            freeIndex = i;
            if (m_framePool[i]->getWidth() >= width && m_framePool[i]->getHeight() >= height) {
                if (m_framePool[i]->getVideoWidth() != width || m_framePool[i]->getVideoHeight() != height)
                    m_framePool[i]->setCrop(0, 0, width, height);
                return m_framePool[i];
            }
        }
    }

    if (nullIndex == -1 && freeIndex == -1) {
        ELOG_ERROR_T("No free frame available");
        return NULL;
    }

    ELOG_TRACE_T("Alloc new frame");

    uint32_t newIndex = nullIndex != -1 ? nullIndex : freeIndex;
    m_framePool[newIndex].reset(new MsdkFrame(width, height, m_allocator));
    if (!m_framePool[newIndex]->init()) {
        m_framePool[newIndex].reset();
        return NULL;
    }

    return m_framePool[newIndex];
}
#endif

void FrameProcesser::onFrame(const Frame& frame)
{
    if (filterFrame(frame))
        return;

    ELOG_TRACE_T("onFrame, format(%s), resolution(%dx%d), timestamp(%u)"
            , getFormatStr(frame.format)
            , frame.additionalInfo.video.width
            , frame.additionalInfo.video.height
            , frame.timeStamp / 90
            );

    if (!m_outFrameRate) {
        if (frame.format == m_format
                && (m_outWidth == frame.additionalInfo.video.width || m_outWidth == 0)
                && (m_outHeight == frame.additionalInfo.video.height || m_outHeight == 0)) {
            deliverFrame(frame);
            return;
        }
    }

    uint32_t width = (m_outWidth == 0 ? frame.additionalInfo.video.width : m_outWidth);
    uint32_t height = (m_outHeight == 0 ? frame.additionalInfo.video.height : m_outHeight);

#ifdef ENABLE_MSDK
    if (m_format == FRAME_FORMAT_MSDK) {
        boost::shared_ptr<woogeen_base::MsdkFrame> msdkFrame = getMsdkFrame(width, height);
        if (!msdkFrame) {
            ELOG_ERROR_T("No valid msdkFrame");
            return;
        }

        if (frame.format == FRAME_FORMAT_I420) {
            VideoFrame *srcFrame = (reinterpret_cast<VideoFrame *>(frame.payload));

            if (!m_converter->convert(srcFrame->video_frame_buffer().get(), msdkFrame.get())) {
                ELOG_ERROR_T("Failed to convert frame");
                return;
            }
        } else {
            MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
            boost::shared_ptr<MsdkFrame> srcMsdkFrame = holder->frame;

            if (!m_converter->convert(srcMsdkFrame.get(), msdkFrame.get())) {
                ELOG_ERROR_T("Failed to convert frame");
                return;
            }
        }

        if (!m_outFrameRate) {
            SendFrame(msdkFrame, frame.timeStamp);
        } else {
            boost::shared_lock<boost::shared_mutex> lock(m_mutex);
            m_activeMsdkFrame = msdkFrame;
        }

        return;
    }
#endif

    if (m_format == FRAME_FORMAT_I420) {
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer = m_bufferManager->getFreeBuffer(width, height);
        if (!i420Buffer) {
            ELOG_ERROR_T("No valid i420Buffer");
            return;
        }

        if (frame.format == FRAME_FORMAT_I420) {
            VideoFrame *srcFrame = (reinterpret_cast<VideoFrame *>(frame.payload));
            if (!m_converter->convert(srcFrame->video_frame_buffer().get(), i420Buffer.get())) {
                ELOG_ERROR_T("Failed to convert frame");
                return;
            }
        }
#ifdef ENABLE_MSDK
        else {
            MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
            boost::shared_ptr<MsdkFrame> srcMsdkFrame = holder->frame;
            if (!m_converter->convert(srcMsdkFrame.get(), i420Buffer.get())) {
                ELOG_ERROR_T("Failed to convert frame");
                return;
            }
        }
#endif

        if (!m_outFrameRate) {
            SendFrame(i420Buffer, frame.timeStamp);
        } else {
            boost::shared_lock<boost::shared_mutex> lock(m_mutex);
            m_activeI420Buffer = i420Buffer;
        }

        return;
    } else {
        ELOG_ERROR_T("Invalid format, input %d(%s), output %d(%s)"
                , frame.format, getFormatStr(frame.format)
                , m_format, getFormatStr(m_format)
                );

        return;
    }
    return;
}

#ifdef ENABLE_MSDK
void FrameProcesser::SendFrame(boost::shared_ptr<woogeen_base::MsdkFrame> msdkFrame, uint32_t timeStamp)
{
    woogeen_base::Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));

    MsdkFrameHolder holder;
    holder.frame = msdkFrame;
    holder.cmd = MsdkCmd_NONE;

    outFrame.format = woogeen_base::FRAME_FORMAT_MSDK;
    outFrame.payload = reinterpret_cast<uint8_t*>(&holder);
    outFrame.length = 0;
    outFrame.additionalInfo.video.width = msdkFrame->getVideoWidth();
    outFrame.additionalInfo.video.height = msdkFrame->getVideoHeight();
    outFrame.timeStamp = timeStamp;

    ELOG_TRACE_T("sendMsdkFrame, %dx%d",
            outFrame.additionalInfo.video.width,
            outFrame.additionalInfo.video.height);

    deliverFrame(outFrame);
}
#endif

void FrameProcesser::SendFrame(rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer, uint32_t timeStamp)
{
    woogeen_base::Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));

    webrtc::VideoFrame i420Frame(i420Buffer, timeStamp, 0, webrtc::kVideoRotation_0);

    outFrame.format = FRAME_FORMAT_I420;
    outFrame.payload = reinterpret_cast<uint8_t*>(&i420Frame);
    outFrame.length = 0;
    outFrame.additionalInfo.video.width = i420Frame.width();
    outFrame.additionalInfo.video.height = i420Frame.height();
    outFrame.timeStamp = timeStamp;

    ELOG_TRACE_T("sendI420Frame, %dx%d",
            outFrame.additionalInfo.video.width,
            outFrame.additionalInfo.video.height);

    deliverFrame(outFrame);
}

void FrameProcesser::onTimeout()
{
    uint32_t timeStamp = kMsToRtpTimestamp * m_clock->TimeInMilliseconds();;

#ifdef ENABLE_MSDK
    if (m_format == FRAME_FORMAT_MSDK) {
        boost::shared_ptr<woogeen_base::MsdkFrame> msdkFrame;
        {
            boost::shared_lock<boost::shared_mutex> lock(m_mutex);
            msdkFrame = m_activeMsdkFrame;
        }
        if (msdkFrame)
            SendFrame(msdkFrame, timeStamp);
        return;
    }
#endif

    if (m_format == FRAME_FORMAT_I420) {
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer;
        {
            boost::shared_lock<boost::shared_mutex> lock(m_mutex);
            i420Buffer = m_activeI420Buffer;
        }
        if (i420Buffer)
            SendFrame(i420Buffer, timeStamp);
        return;
    }
}

}//namespace woogeen_base
