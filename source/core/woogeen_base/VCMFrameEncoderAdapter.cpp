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

#include "VCMFrameEncoderAdapter.h"

#include "MediaUtilities.h"

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(VCMFrameEncoderAdapter, "woogeen.VCMFrameEncoderAdapter");

VCMFrameEncoderAdapter::VCMFrameEncoderAdapter(FrameFormat format)
    : m_streamId(-1)
    , m_encoderStreamId(-1)
{
    m_encoder.reset(new VCMFrameEncoder(format, true));
}

VCMFrameEncoderAdapter::~VCMFrameEncoderAdapter()
{
}

bool VCMFrameEncoderAdapter::canSimulcast(FrameFormat format, uint32_t width, uint32_t height)
{
    return false;
}

bool VCMFrameEncoderAdapter::isIdle()
{
    return m_encoder->isIdle();
}

int32_t VCMFrameEncoderAdapter::generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, woogeen_base::FrameDestination* dest)
{
    if (m_streamId != -1) {
        ELOG_ERROR("Not support multiple streams");
        return -1;
    }

    m_width         = width;
    m_height        = height;
    m_frameRate     = frameRate;
    m_kbps          = bitrateKbps;
    m_keyInterval   = keyFrameIntervalSeconds;
    m_dest          = dest;

    if (m_width == 0 || m_height == 0) {
        ELOG_DEBUG("Generate adaptive encoder stream");
        m_streamId = 0;
        return m_streamId;
    }

    m_encoderStreamId = m_encoder->generateStream(m_width, m_height, m_frameRate, m_kbps, m_keyInterval, dest);
    m_streamId = 0;
    return m_streamId;
}

void VCMFrameEncoderAdapter::degenerateStream(int32_t streamId)
{
    if (streamId == m_streamId && m_encoderStreamId != -1) {
        m_encoder->degenerateStream(m_encoderStreamId);
        m_streamId = -1;
        m_encoderStreamId = -1;
    }
}

void VCMFrameEncoderAdapter::setBitrate(unsigned short kbps, int32_t streamId)
{
    if (streamId == m_streamId && m_encoderStreamId != -1)
        m_encoder->setBitrate(kbps, m_encoderStreamId);
}

void VCMFrameEncoderAdapter::requestKeyFrame(int32_t streamId)
{
    if (streamId == m_streamId && m_encoderStreamId != -1)
        m_encoder->requestKeyFrame(m_encoderStreamId);
}

void VCMFrameEncoderAdapter::onFrame(const Frame& frame)
{
    if (m_streamId != -1 && m_encoderStreamId == -1) {
        m_width     = frame.additionalInfo.video.width;
        m_height    = frame.additionalInfo.video.height;
        m_kbps      = calcBitrate(m_width, m_height);

        m_encoderStreamId = m_encoder->generateStream(m_width, m_height, m_frameRate, m_kbps, m_keyInterval, m_dest);
    }

    m_encoder->onFrame(frame);
}

} // namespace woogeen_base
