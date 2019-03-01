// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AcmmGroup_h
#define AcmmGroup_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <logger.h>

#include "MediaFramePipeline.h"

#include "AcmmInput.h"
#include "AcmmOutput.h"

namespace mcu {

using namespace woogeen_base;
using namespace webrtc;

class AcmmGroup {
    DECLARE_LOGGER();

    static const int32_t _MAX_INPUT_STREAMS_            = 128;
    static const int32_t _MAX_OUTPUT_STREAMS_           = 32;

public:
    AcmmGroup(uint16_t id);
    ~AcmmGroup();

    uint16_t id() {return m_groupId;}

    boost::shared_ptr<AcmmInput> getInput(const std::string& inStream);
    boost::shared_ptr<AcmmOutput> getOutput(const std::string& outStream);

    boost::shared_ptr<AcmmInput> getInput(uint16_t streamId);

    void getInputs(std::vector<boost::shared_ptr<AcmmInput>> &inputs);
    void getOutputs(std::vector<boost::shared_ptr<AcmmOutput>> &outputs);

    uint32_t numOfInputs() {return m_inputs.size();}
    uint32_t numOfOutputs() {return m_outputs.size();}

    boost::shared_ptr<AcmmInput> addInput(const std::string& inStream);
    void removeInput(const std::string& inStream);

    boost::shared_ptr<AcmmOutput> addOutput(const std::string& outStream);
    void removeOutput(const std::string& outStream);

    bool allInputsMuted();
    bool anyOutputsConnected();

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
