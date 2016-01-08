/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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

#ifndef VideoMixer_h
#define VideoMixer_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <logger.h>
#include <map>
#include <vector>

#include "VideoFrameMixer.h"
#include "WebRTCTaskRunner.h"

namespace webrtc {
class VoEVideoSync;
}

namespace mcu {

class VideoLayoutProcessor;

class VideoMixer {
    DECLARE_LOGGER();

public:
    DLL_PUBLIC VideoMixer(const std::string& config);
    virtual ~VideoMixer();

    DLL_PUBLIC bool addInput(const std::string& inStreamID, const std::string& codec, woogeen_base::FrameSource* source);
    DLL_PUBLIC void removeInput(const std::string& inStreamID);
    DLL_PUBLIC void setRegion(const std::string& inStreamID, const std::string& regionID);
    DLL_PUBLIC std::string getRegion(const std::string& inStreamID);
    DLL_PUBLIC void setPrimary(const std::string& inStreamID);

    DLL_PUBLIC bool addOutput(const std::string& outStreamID, const std::string& codec, const std::string& resolution, woogeen_base::FrameDestination* dest);
    DLL_PUBLIC void removeOutput(const std::string& outStreamID);

    int32_t bindAudio(uint32_t sourceId, int voiceChannelId, webrtc::VoEVideoSync*)
    {
        //TODO: Establish another data path to guarantee the a/v sync of input streams.
        return -1;
    }

private:
    int useAFreeInputIndex();
    void closeAll();

    int m_nextOutputIndex;

    uint32_t m_inputCount;
    uint32_t m_maxInputCount;

    unsigned short m_outputKbps;
    boost::shared_ptr<VideoFrameMixer> m_frameMixer;
    boost::shared_ptr<woogeen_base::WebRTCTaskRunner> m_taskRunner;

    boost::shared_mutex m_inputsMutex;
    std::map<std::string, int> m_inputs;
    std::vector<bool> m_freeInputIndexes;

    boost::scoped_ptr<VideoLayoutProcessor> m_layoutProcessor;

    boost::shared_mutex m_outputsMutex;
    std::map<std::string, int32_t> m_outputs;
};


} /* namespace mcu */
#endif /* VideoMixer_h */
