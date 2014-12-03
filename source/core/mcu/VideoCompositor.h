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

#ifndef VideoCompositor_h
#define VideoCompositor_h

#include "JobTimer.h"
#include "VideoFrameProcessor.h"
#include "VideoLayout.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <logger.h>
#include <vector>

namespace webrtc {
class I420VideoFrame;
class VideoProcessingModule;
class CriticalSectionWrapper;
}

namespace mcu {

class VPMPool;
class BufferManager;

/**
 * manages a pool of VPM for preprocessing the incoming I420 frame
 */
class VPMPool {
public:
    VPMPool(unsigned int size);
    ~VPMPool();
    webrtc::VideoProcessingModule* get(unsigned int slot);
    void update(unsigned int slot, VideoSize&);
    unsigned int size() { return m_size; }

private:
    webrtc::VideoProcessingModule** m_vpms;
    unsigned int m_size;    // total pool capacity
    std::vector<VideoSize> m_subVideSize;
};

/**
 * composite a sequence of frames into one frame based on current layout config,
 * there is a question of how many streams to be composited if there are 16 participants
 * but only 6 regions are configed. current implementation will only show 6 participants but
 * still 16 audios will be mixed. In the future, we may enable the video rotation based on VAD history.
 */
class SoftVideoCompositor : public VideoFrameProcessor,
                            public JobTimerListener {
    DECLARE_LOGGER();
public:
    SoftVideoCompositor();
    ~SoftVideoCompositor();

    bool activateInput(int slot, FrameFormat, VideoFrameProvider*);
    void deActivateInput(int slot);
    void pushInput(int slot, unsigned char* payload, int len);

    bool activateOutput(FrameFormat, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer*);
    void deActivateOutput(FrameFormat);

    void setLayout(struct VideoLayout&);
    void setBitrate(FrameFormat, unsigned short bitrate);
    void requestKeyFrame(FrameFormat);

    void onTimeout();

private:
    webrtc::I420VideoFrame* layout();
    webrtc::I420VideoFrame* fluidLayout();
    webrtc::I420VideoFrame* customLayout();
    void generateFrame();
    void onSlotNumberChanged(uint32_t newSlotNum);
    bool commitLayout(); // Commit the new layout config.
    void setBackgroundColor();
    bool validateConfig(VideoLayout& layout)
    {
        return true;
    }

    // Delta used for translating between NTP and internal timestamps.
    int64_t m_ntpDelta;
    boost::scoped_ptr<webrtc::CriticalSectionWrapper> m_configLock;
    bool m_configChanged;
    boost::scoped_ptr<VPMPool> m_vpmPool;
    boost::scoped_ptr<BufferManager> m_bufferManager;
    boost::scoped_ptr<webrtc::I420VideoFrame> m_composedFrame;
    VideoLayout m_currentLayout;
    VideoLayout m_newLayout;
    std::vector<VideoFrameConsumer*> m_consumers;
    /*
     * Each incoming channel will store the decoded frame in this array, and the composition
     * thread will scan this array and compose the frames into one frame.
     */
    boost::scoped_ptr<JobTimer> m_jobTimer;
};

}
#endif /* VideoCompositor_h*/
