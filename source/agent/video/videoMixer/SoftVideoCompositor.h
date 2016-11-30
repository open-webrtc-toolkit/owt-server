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

#ifndef SoftVideoCompositor_h
#define SoftVideoCompositor_h

#include "VideoFrameMixer.h"
#include "VideoLayout.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <BufferManager.h>
#include <JobTimer.h>
#include <logger.h>
#include <MediaFramePipeline.h>
#include <vector>

namespace webrtc {
class I420VideoFrame;
class VideoProcessingModule;
}

namespace mcu {

/**
 * manages a pool of VPM for preprocessing the incoming I420 frame
 */
class VPMPool {
public:
    VPMPool(unsigned int size);
    ~VPMPool();
    webrtc::VideoProcessingModule* get(unsigned int input);
    void update(unsigned int input, woogeen_base::VideoSize&);
    unsigned int size() { return m_size; }

private:
    webrtc::VideoProcessingModule** m_vpms;
    unsigned int m_size;    // total pool capacity
};

class AvatarManager {
    DECLARE_LOGGER();
public:
    AvatarManager(uint8_t size);
    ~AvatarManager();

    bool setAvatar(uint8_t index, const std::string &url);
    bool unsetAvatar(uint8_t index);

    boost::shared_ptr<webrtc::I420VideoFrame> getAvatarFrame(uint8_t index);

protected:
    bool getImageSize(const std::string &url, uint32_t *pWidth, uint32_t *pHeight);
    boost::shared_ptr<webrtc::I420VideoFrame> loadImage(const std::string &url);

private:
    uint8_t m_size;

    std::map<uint8_t, std::string> m_inputs;
    std::map<std::string, boost::shared_ptr<webrtc::I420VideoFrame>> m_frames;

    boost::shared_mutex m_mutex;
};

/**
 * composite a sequence of frames into one frame based on current layout config,
 * there is a question of how many streams to be composited if there are 16 participants
 * but only 6 regions are configed. current implementation will only show 6 participants but
 * still 16 audios will be mixed. In the future, we may enable the video rotation based on VAD history.
 */
class SoftVideoCompositor : public VideoFrameCompositor,
                            public JobTimerListener {
    DECLARE_LOGGER();
    enum LayoutSolutionState{UN_INITIALIZED = 0, CHANGING, IN_WORK};
public:
    SoftVideoCompositor(uint32_t maxInput, woogeen_base::VideoSize rootSize, woogeen_base::YUVColor bgColor, bool crop);
    ~SoftVideoCompositor();

    bool activateInput(int input);
    void deActivateInput(int input);
    bool setAvatar(int input, const std::string& avatar);
    bool unsetAvatar(int input);
    void pushInput(int input, const woogeen_base::Frame&);

    void updateRootSize(woogeen_base::VideoSize& rootSize);
    void updateBackgroundColor(woogeen_base::YUVColor& bgColor);
    void updateLayoutSolution(LayoutSolution& solution);

    void onTimeout();

private:
    webrtc::I420VideoFrame* layout();
    webrtc::I420VideoFrame* customLayout();
    void generateFrame();
    bool commitLayout(); // Commit the new layout config.
    void setBackgroundColor();
    // Delta used for translating between NTP and internal timestamps.
    int64_t m_ntpDelta;
    boost::scoped_ptr<VPMPool> m_vpmPool;
    boost::scoped_ptr<woogeen_base::BufferManager> m_bufferManager;
    boost::scoped_ptr<webrtc::I420VideoFrame> m_compositeFrame;
    woogeen_base::VideoSize m_compositeSize;
    woogeen_base::VideoSize m_newCompositeSize;
    woogeen_base::YUVColor m_bgColor;
    LayoutSolution m_currentLayout;
    LayoutSolution m_newLayout;
    LayoutSolutionState m_solutionState;
    bool m_crop;

    boost::scoped_ptr<AvatarManager> m_avatarManager;
    /*
     * Each incoming channel will store the decoded frame in this array, and the composition
     * thread will scan this array and composite the frames into one frame.
     */
    boost::scoped_ptr<JobTimer> m_jobTimer;
};

}
#endif /* SoftVideoCompositor_h*/
