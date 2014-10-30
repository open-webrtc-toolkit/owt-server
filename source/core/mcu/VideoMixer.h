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
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>
#include <MediaDefinitions.h>
#include <vector>

namespace webrtc {
class VoEVideoSync;
}

namespace mcu {

class BufferManager;
class VCMOutputProcessor;
class TaskRunner;

static const int MIXED_VIDEO_STREAM_ID = 2;

/**
 * Receives media from several sources, mixed into one stream and retransmits it to the RTPDataReceiver.
 */
class VideoMixer : public erizo::MediaSink {
    DECLARE_LOGGER();

public:
    VideoMixer(erizo::RTPDataReceiver*);
    virtual ~VideoMixer();

    /*
     * Each source will be served by a InputProcessor, which is responsible for
     * decoding the incoming streams into I420Frames
     */
    void addSource(erizo::MediaSource*, int voiceChannelId, webrtc::VoEVideoSync*);
    void removeSource(erizo::MediaSource*);

    void onRequestIFrame();

    // Implements the MediaSink interfaces
    virtual int deliverAudioData(char* buf, int len, erizo::MediaSource*);
    virtual int deliverVideoData(char* buf, int len, erizo::MediaSource*);

private:
    bool init();
    void closeAll();

    int assignSlot(erizo::MediaSource*);
    int maxSlot();
    // find the slot number for the corresponding source
    // return -1 if not found
    int getSlot(erizo::MediaSource*);

    boost::shared_ptr<erizo::FeedbackSink> m_feedback;
    erizo::RTPDataReceiver* m_outputReceiver;
    boost::shared_mutex m_sourceMutex;
    std::map<erizo::MediaSource*, boost::shared_ptr<erizo::MediaSink>> m_sinksForSources;
    std::vector<erizo::MediaSource*> m_sourceSlotMap;    // each source will be allocated one index

    boost::shared_ptr<BufferManager> m_bufferManager;
    boost::shared_ptr<TaskRunner> m_taskRunner;
    boost::shared_ptr<VCMOutputProcessor> m_vcmOutputProcessor;
};

inline int VideoMixer::assignSlot(erizo::MediaSource* source)
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

inline int VideoMixer::maxSlot()
{
    return m_sourceSlotMap.size();
}

inline int VideoMixer::getSlot(erizo::MediaSource* source)
{
    for (uint32_t i = 0; i < m_sourceSlotMap.size(); i++) {
        if (m_sourceSlotMap[i] == source)
            return i;
    }
    return -1;
}

} /* namespace mcu */
#endif /* VideoMixer_h */
