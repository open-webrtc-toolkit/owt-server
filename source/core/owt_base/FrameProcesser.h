// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FrameProcesser_h
#define FrameProcesser_h

#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>

#include <webrtc/api/video/video_frame.h>
#include <webrtc/system_wrappers/include/clock.h>

#include <JobTimer.h>
#include "MediaFramePipeline.h"

#ifdef ENABLE_MSDK
#include "MsdkFrame.h"
#include "MsdkBase.h"
#endif

#include "I420BufferManager.h"

#include "FrameConverter.h"
#include "FFmpegDrawText.h"

namespace owt_base {

class FrameProcesser : public VideoFrameProcesser, public JobTimerListener {
    DECLARE_LOGGER();

    const uint32_t kMsToRtpTimestamp = 90;

public:
    FrameProcesser();
    ~FrameProcesser();

    void onFrame(const Frame&);
    bool init(FrameFormat format, const uint32_t width, const uint32_t height, const uint32_t frameRate);

    void onTimeout();

    void drawText(const std::string& textSpec);
    void clearText();

protected:
    bool filterFrame(const Frame& frame);
#ifdef ENABLE_MSDK
    boost::shared_ptr<owt_base::MsdkFrame> getMsdkFrame(const uint32_t width, const uint32_t height);

    void SendFrame(boost::shared_ptr<owt_base::MsdkFrame> msdkFrame, uint32_t timeStamp);
#endif
    void SendFrame(rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer, uint32_t timeStamp);

private:
    uint32_t m_lastWidth;
    uint32_t m_lastHeight;

    FrameFormat m_format;
    uint32_t m_outWidth;
    uint32_t m_outHeight;
    uint32_t m_outFrameRate;

#ifdef ENABLE_MSDK
    boost::shared_ptr<mfxFrameAllocator> m_allocator;
    std::vector<boost::shared_ptr<owt_base::MsdkFrame>> m_framePool;
    boost::shared_ptr<owt_base::MsdkFrame> m_activeMsdkFrame;
#endif

    boost::scoped_ptr<I420BufferManager> m_bufferManager;
    rtc::scoped_refptr<webrtc::I420Buffer> m_activeI420Buffer;

    boost::shared_mutex m_mutex;

    const webrtc::Clock *m_clock;
    boost::scoped_ptr<FrameConverter> m_converter;
    boost::scoped_ptr<JobTimer> m_jobTimer;

    boost::shared_ptr<owt_base::FFmpegDrawText> m_textDrawer;
};

} /* namespace owt_base */

#endif /* FrameProcesser_h */

