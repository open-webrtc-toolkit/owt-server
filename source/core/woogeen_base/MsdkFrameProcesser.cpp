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

#ifdef ENABLE_MSDK

#include "MsdkFrameProcesser.h"

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(MsdkFrameProcesser, "woogeen.MsdkFrameProcesser");

MsdkFrameProcesser::MsdkFrameProcesser()
    : m_lastWidth(0)
    , m_lastHeight(0)
    , m_format(FRAME_FORMAT_UNKNOWN)
{
}

MsdkFrameProcesser::~MsdkFrameProcesser()
{
}

bool MsdkFrameProcesser::init(FrameFormat format)
{
    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase == NULL) {
        ELOG_ERROR("Get MSDK failed.");
        return false;
    }

    m_allocator = msdkBase->createFrameAllocator();
    if (!m_allocator) {
        ELOG_ERROR("Create frame allocator failed.");
        return false;
    }

    m_format = format;
    return true;
}

bool MsdkFrameProcesser::filterFrame(const Frame& frame)
{
    if (frame.format == FRAME_FORMAT_MSDK) {
        MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
        if (holder->cmd != MsdkCmd_NONE)
            return true;
    }

    return false;
}

void MsdkFrameProcesser::onFrame(const Frame& frame)
{
    if (filterFrame(frame))
        return;

    ELOG_TRACE("onFrame, format(%s), resolution(%dx%d), timestamp(%u)"
            , getFormatStr(frame.format)
            , frame.additionalInfo.video.width
            , frame.additionalInfo.video.height
            , frame.timeStamp / 90
            );

    if (m_lastWidth != frame.additionalInfo.video.width
            || m_lastHeight != frame.additionalInfo.video.height) {
        ELOG_DEBUG("Stream(%s) resolution changed, %dx%d -> %dx%d"
                , getFormatStr(frame.format)
                , m_lastWidth, m_lastHeight
                , frame.additionalInfo.video.width, frame.additionalInfo.video.height
                );

        m_lastWidth = frame.additionalInfo.video.width;
        m_lastHeight = frame.additionalInfo.video.height;
    }

    if (frame.format == m_format) {
        deliverFrame(frame);
        return;
    } else if (frame.format == FRAME_FORMAT_I420 && m_format == FRAME_FORMAT_MSDK) {
        uint32_t width = frame.additionalInfo.video.width;
        uint32_t height = frame.additionalInfo.video.height;

        if (m_msdkFrame == NULL
                || m_msdkFrame->getWidth() < width
                || m_msdkFrame->getHeight() < height) {
            m_msdkFrame.reset(new MsdkFrame(width, height, m_allocator));
            if (!m_msdkFrame->init()) {
                m_msdkFrame.reset();
                return;
            }
        }

        if (m_msdkFrame->getVideoWidth() != width || m_msdkFrame->getVideoHeight() != height)
            m_msdkFrame->setCrop(0, 0, width, height);

        I420VideoFrame *i420Frame = (reinterpret_cast<I420VideoFrame *>(frame.payload));
        if (!m_msdkFrame->convertFrom(*i420Frame)) {
            ELOG_ERROR("(%p)Failed to convert I420 frame", this);
            return;
        }

        MsdkFrameHolder holder;
        holder.frame = m_msdkFrame;
        holder.cmd = MsdkCmd_NONE;

        woogeen_base::Frame outFrame;
        memset(&outFrame, 0, sizeof(outFrame));
        outFrame.format = woogeen_base::FRAME_FORMAT_MSDK;
        outFrame.payload = reinterpret_cast<uint8_t*>(&holder);
        outFrame.length = 0; // unused.
        outFrame.additionalInfo.video.width = m_msdkFrame->getVideoWidth();
        outFrame.additionalInfo.video.height = m_msdkFrame->getVideoHeight();
        outFrame.timeStamp = frame.timeStamp;

        deliverFrame(outFrame);
    } else if (frame.format == FRAME_FORMAT_MSDK && m_format == FRAME_FORMAT_I420) {
        deliverFrame(frame);
        return;
#if 0
        if (m_i420Frame == NULL) {
            m_i420Frame.reset(new I420VideoFrame());
        }

        if (!holder->frame->convertTo(*m_i420Frame.get()))
            return;

        woogeen_base::Frame outFrame;
        memset(&outFrame, 0, sizeof(outFrame));
        outFrame.format = woogeen_base::FRAME_FORMAT_I420;
        outFrame.payload = reinterpret_cast<uint8_t*>(m_i420Frame.get());
        outFrame.length = 0; // unused.
        outFrame.timeStamp = frame.timeStamp;
        outFrame.additionalInfo.video.width = m_i420Frame->width();
        outFrame.additionalInfo.video.height = m_i420Frame->height();

        ELOG_TRACE("+++deliverFrame");
        deliverFrame(outFrame);
        ELOG_TRACE("---deliverFrame");
        return;
#endif
    } else {
        ELOG_ERROR("Invalid format, input %d(%s), output %d(%s)"
                , frame.format, getFormatStr(frame.format)
                , m_format, getFormatStr(m_format)
                );
        return;
    }
}

}//namespace woogeen_base
#endif /* ENABLE_MSDK */
