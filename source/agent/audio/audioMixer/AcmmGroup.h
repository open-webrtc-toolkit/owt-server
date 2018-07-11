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

#ifndef AcmmGroup_h
#define AcmmGroup_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/modules/audio_conference_mixer/include/audio_conference_mixer_defines.h>

#include <logger.h>

#include "MediaFramePipeline.h"

#include "AcmmInput.h"
#include "AcmmOutput.h"

namespace mcu {

using namespace woogeen_base;
using namespace webrtc;

class AcmmGroup {
    DECLARE_LOGGER();

    static const int32_t _MAX_INPUT_STREAMS_    = 128;
    static const int32_t _MAX_OUTPUT_STREAMS_   = 16;

public:
    AcmmGroup(uint16_t id);
    ~AcmmGroup();

    uint16_t id() {return m_groupId;}

    boost::shared_ptr<AcmmInput> getInput(const std::string& inStream);
    boost::shared_ptr<AcmmOutput> getOutput(const std::string& outStream);

    boost::shared_ptr<AcmmInput> getInput(uint16_t streamId);

    void getInputs(std::vector<boost::shared_ptr<AcmmInput>> &inputs);

    uint32_t numOfInputs() {return m_inputs.size();}
    uint32_t numOfOutputs() {return m_outputs.size();}

    boost::shared_ptr<AcmmInput> addInput(const std::string& inStream, const woogeen_base::FrameFormat format, woogeen_base::FrameSource* source);
    void removeInput(const std::string& inStream);

    boost::shared_ptr<AcmmOutput> addOutput(const std::string& outStream, const woogeen_base::FrameFormat format, woogeen_base::FrameDestination* destination);
    void removeOutput(const std::string& outStream);

    int32_t NeededFrequency();
    void NewMixedAudio(const AudioFrame* audioFrame);

protected:
    bool getFreeInputId(uint16_t *id);
    bool getFreeOutputId(uint16_t *id);

private:
    uint16_t m_groupId;

    std::vector<bool> m_inputIds;
    std::map<std::string, uint16_t> m_inputIdMap;
    std::map<uint16_t, boost::shared_ptr<AcmmInput>> m_inputs;

    std::vector<bool> m_outputIds;
    std::map<std::string, uint16_t> m_outputIdMap;
    std::map<uint16_t, boost::shared_ptr<AcmmOutput>> m_outputs;
};

} /* namespace mcu */

#endif /* AcmmGroup_h */
