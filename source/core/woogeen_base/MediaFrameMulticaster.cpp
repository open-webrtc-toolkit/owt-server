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

#include "MediaFrameMulticaster.h"

namespace woogeen_base {

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
        ++m_pendingKeyFrameRequests;
    }
}

void MediaFrameMulticaster::onFrame(const Frame& frame)
{
    deliverFrame(frame);
}

void MediaFrameMulticaster::onTimeout()
{
    if (m_pendingKeyFrameRequests > 0) {
        FeedbackMsg msg = {VIDEO_FEEDBACK, REQUEST_KEY_FRAME};
        deliverFeedbackMsg(msg);
        m_pendingKeyFrameRequests = 0;
    }
}

} /* namespace woogeen_base */

