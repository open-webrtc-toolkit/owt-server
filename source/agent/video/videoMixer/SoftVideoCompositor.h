// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SoftVideoCompositor_h
#define SoftVideoCompositor_h

#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

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
#include "FFmpegDrawText.h"

namespace mcu {
class SoftVideoCompositor;

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

class SoftInput {
    DECLARE_LOGGER();

public:
    SoftInput();
    ~SoftInput();

    void setActive(bool active);
    bool isActive(void);

    void pushInput(webrtc::VideoFrame *videoFrame);
    boost::shared_ptr<webrtc::VideoFrame> popInput();

private:
    bool m_active;
    boost::shared_ptr<webrtc::VideoFrame> m_busyFrame;
    boost::shared_mutex m_mutex;

    boost::scoped_ptr<woogeen_base::I420BufferManager> m_bufferManager;

    boost::scoped_ptr<woogeen_base::FrameConverter> m_converter;
};

class SoftFrameGenerator : public JobTimerListener
{
    DECLARE_LOGGER();

    const uint32_t kMsToRtpTimestamp = 90;

    struct Output_t {
        uint32_t width;
        uint32_t height;
        uint32_t fps;
        woogeen_base::FrameDestination *dest;
    };

public:
    SoftFrameGenerator(
            SoftVideoCompositor *owner,
            woogeen_base::VideoSize &size,
            woogeen_base::YUVColor &bgColor,
            const bool crop,
            const uint32_t maxFps,
            const uint32_t minFps);

    ~SoftFrameGenerator();

    void updateLayoutSolution(LayoutSolution& solution);

    bool isSupported(uint32_t width, uint32_t height, uint32_t fps);

    bool addOutput(const uint32_t width, const uint32_t height, const uint32_t fps, woogeen_base::FrameDestination *dst);
    bool removeOutput(woogeen_base::FrameDestination *dst);

    void drawText(const std::string& textSpec);
    void clearText();

    void onTimeout() override;

protected:
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> generateFrame();
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> layout();
    static void layout_regions(SoftFrameGenerator *t, rtc::scoped_refptr<webrtc::I420Buffer> compositeBuffer, const LayoutSolution &regions);

    void reconfigureIfNeeded();

private:
    const webrtc::Clock *m_clock;

    SoftVideoCompositor *m_owner;
    uint32_t m_maxSupportedFps;
    uint32_t m_minSupportedFps;

    uint32_t m_counter;
    uint32_t m_counterMax;

    std::vector<std::list<Output_t>>    m_outputs;
    boost::shared_mutex                 m_outputMutex;

    // configure
    woogeen_base::VideoSize     m_size;
    woogeen_base::YUVColor      m_bgColor;
    bool                        m_crop;

    // reconfifure
    LayoutSolution              m_layout;
    LayoutSolution              m_newLayout;
    bool                        m_configureChanged;
    boost::shared_mutex         m_configMutex;

    boost::scoped_ptr<woogeen_base::I420BufferManager> m_bufferManager;

    boost::scoped_ptr<JobTimer> m_jobTimer;

    // parallel composition
    uint32_t m_parallelNum;
    boost::shared_ptr<boost::asio::io_service> m_srv;
    boost::shared_ptr<boost::asio::io_service::work> m_srvWork;
    boost::shared_ptr<boost::thread_group> m_thrGrp;

    boost::shared_ptr<woogeen_base::FFmpegDrawText> m_textDrawer;
};

/**
 * composite a sequence of frames into one frame based on current layout config,
 * In the future, we may enable the video rotation based on VAD history.
 */
class SoftVideoCompositor : public VideoFrameCompositor {
    DECLARE_LOGGER();

    friend class SoftFrameGenerator;

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

    bool addOutput(const uint32_t width, const uint32_t height, const uint32_t framerateFPS, woogeen_base::FrameDestination *dst) override;
    bool removeOutput(woogeen_base::FrameDestination *dst) override;

    void drawText(const std::string& textSpec);
    void clearText();

protected:
    boost::shared_ptr<webrtc::VideoFrame> getInputFrame(int index);

private:
    uint32_t m_maxInput;

    std::vector<boost::shared_ptr<SoftFrameGenerator>> m_generators;

    std::vector<boost::shared_ptr<SoftInput>> m_inputs;
    boost::scoped_ptr<AvatarManager> m_avatarManager;
};

}
#endif /* SoftVideoCompositor_h*/
