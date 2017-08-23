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

#ifndef SoftVideoCompositor_h
#define SoftVideoCompositor_h

#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <webrtc/system_wrappers/include/clock.h>
#include <webrtc/api/video/video_frame.h>
#include <webrtc/api/video/i420_buffer.h>

#include "logger.h"
#include "JobTimer.h"
#include "MediaFramePipeline.h"
#include "FrameConverter.h"
#include "VideoFrameMixer.h"
#include "VideoLayout.h"
#include "I420BufferManager.h"

namespace mcu {

class AvatarManager {
    DECLARE_LOGGER();
public:
    AvatarManager(uint8_t size);
    ~AvatarManager();

    bool setAvatar(uint8_t index, const std::string &url);
    bool unsetAvatar(uint8_t index);

    boost::shared_ptr<webrtc::VideoFrame> getAvatarFrame(uint8_t index);

protected:
    bool getImageSize(const std::string &url, uint32_t *pWidth, uint32_t *pHeight);
    boost::shared_ptr<webrtc::VideoFrame> loadImage(const std::string &url);

private:
    uint8_t m_size;

    std::map<uint8_t, std::string> m_inputs;
    std::map<std::string, boost::shared_ptr<webrtc::VideoFrame>> m_frames;

    boost::shared_mutex m_mutex;
};

class SwInput {
    DECLARE_LOGGER();

public:
    SwInput();
    ~SwInput();

    void setActive(bool active);
    bool isActive(void) {return m_active;}

    void pushInput(webrtc::VideoFrame *videoFrame);
    boost::shared_ptr<webrtc::VideoFrame> popInput();

private:
    bool m_active;
    boost::shared_ptr<webrtc::VideoFrame> m_busyFrame;
    boost::shared_mutex m_mutex;

    boost::scoped_ptr<woogeen_base::I420BufferManager> m_bufferManager;

    boost::scoped_ptr<woogeen_base::FrameConverter> m_converter;
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
    const uint32_t kMsToRtpTimestamp = 90;

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

protected:
    webrtc::VideoFrame* layout();
    webrtc::VideoFrame* customLayout();
    void generateFrame();
    bool commitLayout();
    void setBackgroundColor();

private:
    const webrtc::Clock *m_clock;

    uint32_t m_maxInput;
    woogeen_base::VideoSize m_compositeSize;
    woogeen_base::VideoSize m_newCompositeSize;
    woogeen_base::YUVColor m_bgColor;
    LayoutSolution m_currentLayout;
    LayoutSolution m_newLayout;
    LayoutSolutionState m_solutionState;
    bool m_crop;

    rtc::scoped_refptr<webrtc::I420Buffer> m_compositeBuffer;
    boost::shared_ptr<webrtc::VideoFrame> m_compositeFrame;
    std::vector<boost::shared_ptr<SwInput>> m_inputs;

    boost::scoped_ptr<AvatarManager> m_avatarManager;

    boost::scoped_ptr<JobTimer> m_jobTimer;
};

}
#endif /* SoftVideoCompositor_h*/
