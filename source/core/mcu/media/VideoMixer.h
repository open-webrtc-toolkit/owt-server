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

#include "VCMInputProcessor.h"
#include "VideoFrameMixer.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>
#include <MediaMuxer.h>
#include <MediaSourceConsumer.h>
#include <tuple>
#include <WebRTCFeedbackProcessor.h>
#include <WebRTCTaskRunner.h>
#include <vector>
#include <VideoFrameSender.h>

namespace webrtc {
class VoEVideoSync;
}

namespace mcu {

class VideoLayoutProcessor;
typedef std::tuple<int, unsigned int, unsigned int> ExtendedKey;

/**
 * Receives media from several sources, mixed into one stream and retransmits it to the RTPDataReceiver.
 */
class VideoMixer : public woogeen_base::MediaSourceConsumer, public woogeen_base::FrameProvider, public erizo::FeedbackSink, public InputProcessorCallback, public woogeen_base::Notification {
    DECLARE_LOGGER();
public:
    VideoMixer(erizo::RTPDataReceiver*, boost::property_tree::ptree& config);
    virtual ~VideoMixer();

    // Video output related methods.
    int32_t addOutput(int payloadType, bool nack, bool fec, const VideoSize& size);
    int32_t removeOutput(int payloadType, const VideoSize& size);
    woogeen_base::IntraFrameCallback* getIFrameCallback(int payloadType, const VideoSize& size);
    uint32_t getSendSSRC(int payloadType, bool nack, bool fec, const VideoSize& size);

    // Video input related methods.
    int32_t bindAudio(uint32_t sourceId, int voiceChannelId, webrtc::VoEVideoSync*);
    bool setSourceBitrate(uint32_t from, uint32_t kbps);
    boost::shared_ptr<erizo::MediaSink> getMediaSink(uint32_t from);

    // Implements MediaSourceConsumer.
    erizo::MediaSink* addSource(uint32_t from,
                                bool isAudio,
                                erizo::DataContentType,
                                int payloadType,
                                erizo::FeedbackSink*,
                                const std::string& participantId);
    void removeSource(uint32_t from, bool isAudio);

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements VCMInputProcessorInitCallback
    void onInputProcessorInitOK(int index);

    // Layout related operations
    bool specifySourceRegion(uint32_t from, const std::string& regionID);
    std::string getSourceRegion(uint32_t from);
    void promoteSources(std::vector<uint32_t>& sources);
    bool setResolution(const std::string& resolution);
    bool setBackgroundColor(const std::string& color);

    // Implements FrameProivder
    int32_t addFrameConsumer(const std::string&, woogeen_base::FrameFormat, woogeen_base::FrameConsumer*, const woogeen_base::MediaSpecInfo&);
    void removeFrameConsumer(int32_t id);
    // TODO: Implement it.
    virtual void setBitrate(unsigned short kbps, int id = 0) { }

    // Implements Notification
    void setupNotification(std::function<void (const std::string&, const std::string&)> f);
private:
    void closeAll();

    int assignInput(uint32_t source);
    // Find the slot number for the corresponding source
    // return -1 if not found
    int getInput(uint32_t source);

    uint32_t m_participants;

    boost::shared_ptr<woogeen_base::WebRTCTaskRunner> m_taskRunner;
    erizo::RTPDataReceiver* m_outputReceiver;
    boost::shared_mutex m_sourceMutex;
    std::map<uint32_t, boost::shared_ptr<erizo::MediaSink>> m_sinksForSources;
    std::vector<uint32_t> m_sourceInputMap;    // each source will be allocated one index
    boost::scoped_ptr<VideoLayoutProcessor> m_layoutProcessor;
    bool m_hardwareAccelerated;
    uint32_t m_maxInputCount;
    int m_outputKbps;
    boost::shared_ptr<VideoFrameMixer> m_frameMixer;
    boost::shared_mutex m_outputMutex;
    std::map<ExtendedKey, boost::shared_ptr<woogeen_base::VideoFrameSender>> m_outputs;
};

inline int VideoMixer::assignInput(uint32_t source)
{
    for (uint32_t i = 0; i < m_sourceInputMap.size(); i++) {
        if (!m_sourceInputMap[i]) {
            m_sourceInputMap[i] = source;
            return i;
        }
    }
    m_sourceInputMap.push_back(source);
    return m_sourceInputMap.size() - 1;
}

inline int VideoMixer::getInput(uint32_t source)
{
    for (uint32_t i = 0; i < m_sourceInputMap.size(); i++) {
        if (m_sourceInputMap[i] == source)
            return i;
    }
    return -1;
}

} /* namespace mcu */
#endif /* VideoMixer_h */
