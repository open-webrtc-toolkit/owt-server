// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "MediaFrameMulticaster.h"

namespace owt_base {

MediaFrameMulticaster::MediaFrameMulticaster()
    : m_pendingKeyFrameRequests(0)
{
    m_feedbackTimer.reset(new JobTimer(1, this));
}

MediaFrameMulticaster::~MediaFrameMulticaster()
{
    m_feedbackTimer->stop();
}

void MediaFrameMulticaster::onFeedback(const FeedbackMsg& msg)
{
    if (msg.type == VIDEO_FEEDBACK && msg.cmd == REQUEST_KEY_FRAME) {
        if (!m_pendingKeyFrameRequests) {
            FeedbackMsg msg = {VIDEO_FEEDBACK, REQUEST_KEY_FRAME};
            deliverFeedbackMsg(msg);
        }
        ++m_pendingKeyFrameRequests;
    }
}

void MediaFrameMulticaster::onFrame(const Frame& frame)
{
    deliverFrame(frame);
}

void MediaFrameMulticaster::onTimeout()
{
    if (m_pendingKeyFrameRequests > 1) {
        FeedbackMsg msg = {VIDEO_FEEDBACK, REQUEST_KEY_FRAME};
        deliverFeedbackMsg(msg);
    }
    m_pendingKeyFrameRequests = 0;
}

} /* namespace owt_base */

