// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef Im360VideoCompositor_h
#define Im360VideoCompositor_h

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

#include "Im360StitchParams.h"

#include <interface/stitcher.h>
#include <buffer_pool.h>

namespace mcu {
namespace xcam {

class Im360VideoCompositor;

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

    boost::scoped_ptr<owt_base::I420BufferManager> m_bufferManager;

    boost::scoped_ptr<owt_base::FrameConverter> m_converter;
};

class Im360FrameStitcher : public JobTimerListener
{
    DECLARE_LOGGER();

    const uint32_t kMsToRtpTimestamp = 90;

    struct Output_t {
        uint32_t width;
        uint32_t height;
        uint32_t fps;
        owt_base::FrameDestination *dest;
    };

public:
    Im360FrameStitcher(
            Im360VideoCompositor *owner,
            owt_base::VideoSize &size,
            owt_base::YUVColor &bgColor,
            const bool crop,
            const uint32_t maxFps,
            const uint32_t minFps);

    ~Im360FrameStitcher();

    void updateLayoutSolution(LayoutSolution& solution);

    bool isSupported(uint32_t width, uint32_t height, uint32_t fps);

    bool addOutput(const uint32_t width, const uint32_t height, const uint32_t fps, owt_base::FrameDestination *dst);
    bool removeOutput(owt_base::FrameDestination *dst);

    void drawText(const std::string& textSpec);
    void clearText();

    void onTimeout() override;

    XCam::SmartPtr<XCam::Stitcher> createStitcher(Im360StitchParams::CamModel cam_model, XCam::StitchScopicMode scopic_mode);
    void stitch_images(Im360FrameStitcher *t, rtc::scoped_refptr<webrtc::I420Buffer> compositeBuffer);

protected:
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> generateFrame();
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> layout();
    static void layout_regions(Im360FrameStitcher *t, rtc::scoped_refptr<webrtc::I420Buffer> compositeBuffer, const LayoutSolution &regions);

    void reconfigureIfNeeded();

private:
    XCam::SmartPtr<XCam::BufferPool> createStitchBufferPool(const uint32_t imageWidth,
                                const uint32_t imageHeight,
                                const uint32_t reserveCount = 4);
    XCam::SmartPtr<XCam::VideoBuffer> getStitchInputBuffer(rtc::scoped_refptr<webrtc::VideoFrameBuffer> i420Frame);

    void convertNV12toI420(XCam::SmartPtr<XCam::VideoBuffer> nv12Frame, rtc::scoped_refptr<webrtc::I420Buffer> i420Frame);

private:
    const webrtc::Clock *m_clock;

    Im360VideoCompositor *m_owner;
    uint32_t m_maxSupportedFps;
    uint32_t m_minSupportedFps;

    uint32_t m_counter;
    uint32_t m_counterMax;

    std::vector<std::list<Output_t>>    m_outputs;
    boost::shared_mutex                 m_outputMutex;

    // configure
    owt_base::VideoSize     m_size;
    owt_base::YUVColor      m_bgColor;
    bool                    m_crop;

    // reconfifure
    LayoutSolution              m_layout;
    LayoutSolution              m_newLayout;
    bool                        m_configureChanged;
    boost::shared_mutex         m_configMutex;

    boost::scoped_ptr<owt_base::I420BufferManager> m_bufferManager;

    boost::scoped_ptr<JobTimer> m_jobTimer;

    boost::shared_ptr<owt_base::FFmpegDrawText> m_textDrawer;

    XCam::SmartPtr<XCam::Stitcher> m_stitcher;
    XCam::VideoBufferList m_stitchBufferList;
    XCam::SmartPtr<XCam::BufferPool> m_stitchInBufferPool;
    XCam::SmartPtr<XCam::BufferPool> m_stitchOutBufferPool;
};

/**
 * stitch a sequence of frames into one 360 degree panorama frame,
 */
class Im360VideoCompositor : public VideoFrameCompositor {
    DECLARE_LOGGER();

    friend class Im360FrameStitcher;

public:
    Im360VideoCompositor(uint32_t maxInput, owt_base::VideoSize rootSize, owt_base::YUVColor bgColor, bool crop);
    ~Im360VideoCompositor();

    bool activateInput(int input);
    void deActivateInput(int input);
    bool setAvatar(int input, const std::string& avatar);
    bool unsetAvatar(int input);
    void pushInput(int input, const owt_base::Frame&);

    void updateRootSize(owt_base::VideoSize& rootSize);
    void updateBackgroundColor(owt_base::YUVColor& bgColor);
    void updateLayoutSolution(LayoutSolution& solution);

    bool addOutput(const uint32_t width, const uint32_t height, const uint32_t framerateFPS, owt_base::FrameDestination *dst) override;
    bool removeOutput(owt_base::FrameDestination *dst) override;

    void drawText(const std::string& textSpec);
    void clearText();

protected:
    boost::shared_ptr<webrtc::VideoFrame> getInputFrame(int index);

private:
    uint32_t m_maxInput;

    boost::shared_ptr<Im360FrameStitcher> m_stitcher;

    std::vector<boost::shared_ptr<SoftInput>> m_inputs;
    boost::scoped_ptr<AvatarManager> m_avatarManager;
};

}
}
#endif /* Im360VideoCompositor_h*/
