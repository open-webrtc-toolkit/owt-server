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

#include "Config.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>
#include <MediaSourceConsumer.h>
#include <vector>

namespace webrtc {
class VoEVideoSync;
}

namespace mcu {

class TaskRunner;
class VCMInputProcessor;
class VideoOutputProcessor;
class VideoFrameProcessor;
struct Layout;

static const int MIXED_VIDEO_STREAM_ID = 2;

/**
 * Receives media from several sources, mixed into one stream and retransmits it to the RTPDataReceiver.
 */
class VideoMixer : public woogeen_base::MediaSourceConsumer, public erizo::MediaSink, public erizo::FeedbackSink, public ConfigListener {
    DECLARE_LOGGER();

public:
    VideoMixer(erizo::RTPDataReceiver*, bool hardwareAccelerated);
    virtual ~VideoMixer();

    int32_t bindAudio(uint32_t sourceId, int voiceChannelId, webrtc::VoEVideoSync*);
    void onRequestIFrame();
    uint32_t sendSSRC();
    // TODO: Remove me once OOP Mixer is able to invoke addSource explicitly.
    void addSourceOnDemand(bool allow) { m_addSourceOnDemand = allow; }

    // Implements the MediaSourceConsumer interfaces
    /*
     * Each source will be served by a InputProcessor, which is responsible for
     * decoding the incoming streams into I420Frames
     */
    virtual int32_t addSource(uint32_t from, bool isAudio, erizo::FeedbackSink*);
    virtual int32_t removeSource(uint32_t from, bool isAudio);
    erizo::MediaSink* mediaSink() { return this; }

    /**
     * Implements the MediaSink interfaces
     */
    virtual int deliverAudioData(char* buf, int len);
    virtual int deliverVideoData(char* buf, int len);

    // Implements the FeedbackSink interfaces
    virtual int deliverFeedback(char* buf, int len);

    // Implements ConfigListener.
    void onConfigChanged();

private:
    bool init();
    void closeAll();

    int assignSlot(uint32_t source);
    // find the slot number for the corresponding source
    // return -1 if not found
    int getSlot(uint32_t source);

    int m_hardwareAccelerated;
    int m_participants;
    boost::shared_ptr<erizo::FeedbackSink> m_feedback;
    erizo::RTPDataReceiver* m_outputReceiver;
    boost::shared_mutex m_sourceMutex;
    std::map<uint32_t, boost::shared_ptr<VCMInputProcessor>> m_sinksForSources;
    std::vector<uint32_t> m_sourceSlotMap;    // each source will be allocated one index
    bool m_addSourceOnDemand;

    boost::shared_ptr<TaskRunner> m_taskRunner;
    boost::shared_ptr<VideoOutputProcessor> m_videoOutputProcessor;
    boost::shared_ptr<VideoFrameProcessor> m_mixer;
};

inline int VideoMixer::assignSlot(uint32_t source)
{
    for (uint32_t i = 0; i < m_sourceSlotMap.size(); i++) {
        if (!m_sourceSlotMap[i]) {
            m_sourceSlotMap[i] = source;
            return i;
        }
    }
    m_sourceSlotMap.push_back(source);
    return m_sourceSlotMap.size() - 1;
}

inline int VideoMixer::getSlot(uint32_t source)
{
    for (uint32_t i = 0; i < m_sourceSlotMap.size(); i++) {
        if (m_sourceSlotMap[i] == source)
            return i;
    }
    return -1;
}

} /* namespace mcu */
#endif /* VideoMixer_h */
