// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AcmmBroadcastGroup_h
#define AcmmBroadcastGroup_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <logger.h>

#include "MediaFramePipeline.h"

#include "AcmmOutput.h"

namespace mcu {

using namespace owt_base;
using namespace webrtc;

class AcmmBroadcastGroup {
    DECLARE_LOGGER();

    static const int32_t _MAX_OUTPUT_STREAMS_ = 32;

public:
    AcmmBroadcastGroup();
    ~AcmmBroadcastGroup();

    bool addDest(const owt_base::FrameFormat format, owt_base::FrameDestination* destination);
    void removeDest(owt_base::FrameDestination* destination);

    int32_t NeededFrequency();
    void NewMixedAudio(const AudioFrame* audioFrame);

protected:
    bool getFreeOutputId(uint16_t *id);

private:
    const uint16_t m_groupId;

    std::vector<bool> m_outputIds;
    std::map<owt_base::FrameFormat, boost::shared_ptr<AcmmOutput>> m_outputMap;
    std::map<owt_base::FrameDestination*, owt_base::FrameFormat> m_formatMap;
};

} /* namespace mcu */

#endif /* AcmmBroadcastGroup_h */
