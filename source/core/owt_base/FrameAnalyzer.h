// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FrameAnalyzer_h
#define FrameAnalyzer_h

#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>

#include <webrtc/api/video/video_frame.h>
#include <webrtc/system_wrappers/include/clock.h>

#include <JobTimer.h>
#include "MediaFramePipeline.h"

// TODO: enable MSDK for analyzer

#include "I420BufferManager.h"

#include "AnalyticsPlugin.h"

namespace owt_base {

class FrameAnalyzer : public VideoFrameAnalyzer, public JobTimerListener, public rvaFrameCallback {
    DECLARE_LOGGER();

    const uint32_t kMsToRtpTimestamp = 90;

public:
    FrameAnalyzer();
    ~FrameAnalyzer();

    void onFrame(const Frame&);
    bool init(FrameFormat format, const uint32_t width, const uint32_t height, const uint32_t frameRate, const std::string& pluginName);

    void onTimeout();

    void OnPluginFrame(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer);

protected:
    bool filterFrame(const Frame& frame);
    void SendFrame(rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer, uint32_t timeStamp);

private:
    uint32_t m_lastWidth;
    uint32_t m_lastHeight;

    FrameFormat m_format;
    uint32_t m_outWidth;
    uint32_t m_outHeight;
    uint32_t m_outFrameRate;

    boost::scoped_ptr<I420BufferManager> m_bufferManager;
    rtc::scoped_refptr<webrtc::I420Buffer> m_activeI420Buffer;

    boost::shared_mutex m_mutex;

    const webrtc::Clock *m_clock;
    //boost::scoped_ptr<rvaPlugin> m_plugin;
    std::string plugin_guid_;
    std::string plugin_name_;
    void* plugin_handle_;
    rvaPlugin* plugin_;
    rva_create_t* create_plugin;
    rva_destroy_t* destroy_plugin;
    boost::scoped_ptr<JobTimer> m_jobTimer;
};

} /* namespace owt_base */

#endif /* FrameAnalyzer_h */

