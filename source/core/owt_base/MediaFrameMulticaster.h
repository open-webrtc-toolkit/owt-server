// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MediaFrameMulticaster_h
#define MediaFrameMulticaster_h

#include "MediaFramePipeline.h"
#include <JobTimer.h>

namespace owt_base {

class MediaFrameMulticaster : public FrameSource, public FrameDestination, public JobTimerListener {
public:
    MediaFrameMulticaster();
    virtual ~MediaFrameMulticaster();

    // Implements FrameSource.
    void onFeedback(const FeedbackMsg&);

    // Implements FrameDestination.
    void onFrame(const Frame&);

    // Implements FrameDestination.
    void onMetaData(const MetaData&);

    // Implements JobTimerListener.
    void onTimeout();

private:
    std::shared_ptr<SharedJobTimer> m_feedbackTimer;
    uint32_t m_pendingKeyFrameRequests;
};

} /* namespace owt_base */

#endif /* MediaFrameMulticaster_h */
