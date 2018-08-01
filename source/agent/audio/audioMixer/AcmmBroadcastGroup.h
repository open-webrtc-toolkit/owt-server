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

#ifndef AcmmBroadcastGroup_h
#define AcmmBroadcastGroup_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <logger.h>

#include "MediaFramePipeline.h"

#include "AcmmOutput.h"

namespace mcu {

using namespace woogeen_base;
using namespace webrtc;

class AcmmBroadcastGroup {
    DECLARE_LOGGER();

    static const int32_t _MAX_OUTPUT_STREAMS_ = 32;

public:
    AcmmBroadcastGroup();
    ~AcmmBroadcastGroup();

    bool addDest(const woogeen_base::FrameFormat format, woogeen_base::FrameDestination* destination);
    void removeDest(woogeen_base::FrameDestination* destination);

    int32_t NeededFrequency();
    void NewMixedAudio(const AudioFrame* audioFrame);

protected:
    bool getFreeOutputId(uint16_t *id);

private:
    const uint16_t m_groupId;

    std::vector<bool> m_outputIds;
    std::map<woogeen_base::FrameFormat, boost::shared_ptr<AcmmOutput>> m_outputMap;
    std::map<woogeen_base::FrameDestination*, woogeen_base::FrameFormat> m_formatMap;
};

} /* namespace mcu */

#endif /* AcmmBroadcastGroup_h */
