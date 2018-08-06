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

#ifndef VideoTranscoder_h
#define VideoTranscoder_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <logger.h>

#include "MediaFramePipeline.h"
#include "VideoFrameTranscoder.h"

namespace mcu {

struct VideoTranscoderConfig {
    bool useGacc;
    uint32_t MFE_timeout;
};

class VideoTranscoder {
    DECLARE_LOGGER();

public:
    VideoTranscoder(const VideoTranscoderConfig& config);
    virtual ~VideoTranscoder();

    bool setInput(const std::string& inStreamID, const std::string& codec, woogeen_base::FrameSource* source);
    void unsetInput(const std::string& inStreamID);

    bool addOutput(const std::string& outStreamID
            , const std::string& codec
            , const woogeen_base::VideoCodecProfile profile
            , const std::string& resolution
            , const unsigned int framerateFPS
            , const unsigned int bitrateKbps
            , const unsigned int keyFrameIntervalSeconds
            , woogeen_base::FrameDestination* dest);
    void removeOutput(const std::string& outStreamID);
    void forceKeyFrame(const std::string& outStreamID);

protected:
    int useAFreeInputIndex();
    void closeAll();

private:
    uint32_t m_inputCount;
    uint32_t m_maxInputCount;
    uint32_t m_nextOutputIndex;

    boost::shared_mutex m_inputsMutex;
    std::map<std::string, int> m_inputs;
    std::vector<bool> m_freeInputIndexes;

    boost::shared_mutex m_outputsMutex;
    std::map<std::string, int32_t> m_outputs;

    boost::shared_ptr<VideoFrameTranscoder> m_frameTranscoder;
};

} /* namespace mcu */
#endif /* VideoTranscoder_h */
