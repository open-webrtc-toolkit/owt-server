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

#include "SwFrameProcesser.h"

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(SwFrameProcesser, "woogeen.SwFrameProcesser");

SwFrameProcesser::SwFrameProcesser()
    : m_lastWidth(0)
    , m_lastHeight(0)
{
}

SwFrameProcesser::~SwFrameProcesser()
{
}

bool SwFrameProcesser::init(FrameFormat format)
{
    return true;
}

void SwFrameProcesser::onFrame(const Frame& frame)
{
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

    deliverFrame(frame);
}

}//namespace woogeen_base
