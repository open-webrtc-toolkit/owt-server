// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MsdkVideoCompositor_h
#define MsdkVideoCompositor_h

#ifdef ENABLE_MSDK

#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <logger.h>
#include <JobTimer.h>

#include <webrtc/system_wrappers/include/clock.h>

#include "VideoFrameMixer.h"
#include "VideoLayout.h"
#include "MediaFramePipeline.h"
#include "MsdkFrame.h"

#include "FrameConverter.h"

namespace mcu {

using namespace owt_base;

class MsdkVideoCompositor;

class MsdkAvatarManager {
    DECLARE_LOGGER();

public:
    MsdkAvatarManager(uint8_t size, boost::shared_ptr<mfxFrameAllocator> allocator);
    ~MsdkAvatarManager();

    bool setAvatar(uint8_t index, const std::string &url);
    bool unsetAvatar(uint8_t index);

    boost::shared_ptr<owt_base::MsdkFrame> getAvatarFrame(uint8_t index);

protected:
    bool getImageSize(const std::string &url, uint32_t *pWidth, uint32_t *pHeight);
    boost::shared_ptr<owt_base::MsdkFrame> loadImage(const std::string &url);

private:
    uint8_t m_size;
    boost::shared_ptr<mfxFrameAllocator> m_allocator;

    std::map<uint8_t, std::string> m_inputs;
    std::map<std::string, boost::shared_ptr<owt_base::MsdkFrame>> m_frames;

    boost::shared_mutex m_mutex;
};

class MsdkInput {
    DECLARE_LOGGER();

public:
    MsdkInput(MsdkVideoCompositor *owner, boost::shared_ptr<mfxFrameAllocator> allocator);
    ~MsdkInput();

    void activate();
    void deActivate();
    bool isActivate();

    void pushInput(const owt_base::Frame& frame);
    boost::shared_ptr<MsdkFrame> popInput();

protected:
    bool initSwFramePool(int width, int height);
    boost::shared_ptr<owt_base::MsdkFrame> getMsdkFrame(const uint32_t width, const uint32_t height);
    bool processCmd(const owt_base::Frame& frame);
    boost::shared_ptr<MsdkFrame> convert(const owt_base::Frame& frame);

private:
    MsdkVideoCompositor *m_owner;
    boost::shared_ptr<mfxFrameAllocator> m_allocator;

    boost::shared_ptr<owt_base::MsdkFrame> m_msdkFrame;
    boost::scoped_ptr<FrameConverter> m_converter;

    bool m_active;
    boost::shared_ptr<owt_base::MsdkFrame> m_busyFrame;

    // todo, dont flush
    boost::scoped_ptr<MsdkFramePool> m_swFramePool;
    int m_swFramePoolWidth;
    int m_swFramePoolHeight;

    boost::shared_mutex m_mutex;
};

class MsdkVpp
{
    DECLARE_LOGGER();

    const uint8_t NumOfMixedFrames = 3;

public:
    MsdkVpp(owt_base::VideoSize &size, owt_base::YUVColor &bgColor, const bool crop);
    ~MsdkVpp();

    bool init(void);
    bool update(owt_base::VideoSize &size, owt_base::YUVColor &bgColor, LayoutSolution &layout);

    boost::shared_ptr<owt_base::MsdkFrame> mix(
            std::vector<boost::shared_ptr<owt_base::MsdkFrame>> &inputFrames);

protected:
    void defaultParam(void);
    void updateParam(void);
    bool isValidParam(void);

    bool allocateFrames();

    void createVpp(void);
    bool resetVpp(void);
    void applyAspectRatio(std::vector<boost::shared_ptr<owt_base::MsdkFrame>> &inputFrames);

    void convertToCompInputStream(mfxVPPCompInputStream *inputStream, const owt_base::VideoSize& rootSize, const Region& region);

private:
    MFXVideoSession *m_session;
    boost::shared_ptr<mfxFrameAllocator> m_allocator;
    MFXVideoVPP *m_vpp;
    bool m_vppReady;

    // param
    boost::scoped_ptr<mfxVideoParam> m_videoParam;
    boost::scoped_ptr<mfxExtVPPComposite> m_extVppComp;
    std::vector<mfxVPPCompInputStream> m_compInputStreams;

    // config msdk layout
    std::vector<mfxVPPCompInputStream> m_msdkLayout;

    // config
    owt_base::VideoSize     m_size;
    owt_base::YUVColor      m_bgColor;
    LayoutSolution              m_layout;
    bool                        m_crop;

    // frames
    boost::scoped_ptr<owt_base::MsdkFramePool> m_mixedFramePool;
    boost::shared_ptr<owt_base::MsdkFrame> m_defaultInputFrame;
    boost::shared_ptr<owt_base::MsdkFrame> m_defaultRootFrame;
};

class MsdkFrameGenerator : public JobTimerListener
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
    MsdkFrameGenerator(
            MsdkVideoCompositor *owner,
            owt_base::VideoSize &size,
            owt_base::YUVColor &bgColor,
            const bool crop,
            const uint32_t maxFps,
            const uint32_t minFps);

    ~MsdkFrameGenerator();

    void updateLayoutSolution(LayoutSolution& solution);

    bool isSupported(uint32_t width, uint32_t height, uint32_t fps);

    bool addOutput(const uint32_t width, const uint32_t height, const uint32_t fps, owt_base::FrameDestination *dst);
    bool removeOutput(owt_base::FrameDestination *dst);

    void onTimeout() override;

protected:
    boost::shared_ptr<owt_base::MsdkFrame> generateFrame();
    boost::shared_ptr<owt_base::MsdkFrame> layout();

    void reconfigureIfNeeded();

public:
    const webrtc::Clock *m_clock;

    MsdkVideoCompositor *m_owner;
    uint32_t m_maxSupportedFps;
    uint32_t m_minSupportedFps;

    uint32_t m_counter;
    uint32_t m_counterMax;

    std::vector<std::list<Output_t>>    m_outputs;
    boost::shared_mutex                 m_outputMutex;

    boost::scoped_ptr<MsdkVpp> m_msdkVpp;

    // configure
    owt_base::VideoSize     m_size;
    owt_base::YUVColor      m_bgColor;
    bool                        m_crop;

    // reconfifure
    LayoutSolution              m_layout;
    LayoutSolution              m_newLayout;
    bool                        m_configureChanged;
    boost::shared_mutex         m_configMutex;

    boost::scoped_ptr<JobTimer> m_jobTimer;
};

/**
 * composite a sequence of frames into one frame based on current layout config,
 * we may enable the video rotation based on VAD history.
 */
class MsdkVideoCompositor : public VideoFrameCompositor {
    DECLARE_LOGGER();

    friend class MsdkInput;
    friend class MsdkFrameGenerator;

public:
    MsdkVideoCompositor(uint32_t maxInput, owt_base::VideoSize rootSize, owt_base::YUVColor bgColor, bool crop);
    ~MsdkVideoCompositor();

    bool activateInput(int input);
    void deActivateInput(int input);
    bool setAvatar(int input, const std::string& avatar);
    bool unsetAvatar(int input);
    void pushInput(int input, const owt_base::Frame& frame);

    void updateRootSize(owt_base::VideoSize& rootSize);
    void updateBackgroundColor(owt_base::YUVColor& bgColor);
    void updateLayoutSolution(LayoutSolution& solution);

    bool addOutput(const uint32_t width, const uint32_t height, const uint32_t framerateFPS, owt_base::FrameDestination *dst) override;
    bool removeOutput(owt_base::FrameDestination *dst) override;

    void drawText(const std::string& textSpec) {}
    void clearText() {}

protected:
    void createAllocator();
    boost::shared_ptr<owt_base::MsdkFrame> getInputFrame(int index);
    void flush(void);

private:
    uint32_t m_maxInput;

    MFXVideoSession *m_session;
    boost::shared_ptr<mfxFrameAllocator> m_allocator;

    std::vector<boost::shared_ptr<MsdkFrameGenerator>> m_generators;

    std::vector<boost::shared_ptr<MsdkInput>> m_inputs;
    boost::scoped_ptr<MsdkAvatarManager> m_avatarManager;
};

}

#endif /* ENABLE_MSDK */
#endif /* MsdkVideoCompositor_h*/
