// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MediaFrameMulticaster_h
#define MediaFrameMulticaster_h

#include "MediaFramePipeline.h"
#include <JobTimer.h>

namespace woogeen_base {

class MediaFrameMulticaster : public FrameSource, public FrameDestination, public JobTimerListener {
public:
    MediaFrameMulticaster();
    virtual ~MediaFrameMulticaster();

    // Implements FrameSource.
    void onFeedback(const FeedbackMsg&);

    // Implements FrameDestination.
    void onFrame(const Frame&);

    // Implements JobTimerListener.
    void onTimeout();

private:
    boost::scoped_ptr<JobTimer> m_feedbackTimer;
    uint32_t m_pendingKeyFrameRequests;
};

} /* namespace woogeen_base */

#endif /* MediaFrameMulticaster_h */
