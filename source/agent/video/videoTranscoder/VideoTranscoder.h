// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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

    void drawText(const std::string& textSpec);
    void clearText();

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
