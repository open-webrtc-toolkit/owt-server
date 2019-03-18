// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "FrameAnalyzer.h"
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <unordered_map>

using namespace webrtc;

namespace owt_base {

DEFINE_LOGGER(FrameAnalyzer, "owt.FrameAnalyzer");

FrameAnalyzer::FrameAnalyzer()
    : m_lastWidth(0)
    , m_lastHeight(0)
    , m_format(FRAME_FORMAT_UNKNOWN)
    , m_outWidth(-1)
    , m_outHeight(-1)
    , m_outFrameRate(-1)
    , m_clock(NULL)
{
}

FrameAnalyzer::~FrameAnalyzer()
{
    if (m_outFrameRate > 0) {
        m_jobTimer->stop();
    }
    if (plugin_ != nullptr && plugin_handle_ != nullptr) {
         destroy_plugin(plugin_);
         dlclose(plugin_handle_);
    }
}

bool FrameAnalyzer::init(FrameFormat format, const uint32_t width, const uint32_t height, const uint32_t frameRate, const std::string& pluginName)
{
    ELOG_DEBUG_T("format(%s), size(%dx%d), frameRate(%d)", getFormatStr(format), width, height, frameRate);

    if (format != FRAME_FORMAT_MSDK && format != FRAME_FORMAT_I420) {
        ELOG_ERROR_T("Invalid format(%d)", format);
        return false;
    }

    plugin_name_ = pluginName;

    m_format = format;
    m_outWidth = width;
    m_outHeight = height;
    m_outFrameRate = frameRate;

//    m_plugin.reset(new rvaPlugin());
    plugin_handle_ = dlopen(plugin_name_.c_str(), RTLD_LAZY);
    if (plugin_handle_ == nullptr) {
        ELOG_ERROR_T("Failed to open the plugin.(%s)", plugin_name_.c_str());
        return false;
    }

    create_plugin = (rva_create_t*)dlsym(plugin_handle_, "CreatePlugin");
    destroy_plugin = (rva_destroy_t*)dlsym(plugin_handle_, "DestroyPlugin");

    if (create_plugin == nullptr || destroy_plugin == nullptr) {
        ELOG_ERROR_T("Failed to get plugin interface.");
        dlclose(plugin_handle_);
        return false;
    }

    plugin_ = create_plugin();
    if (plugin_ == nullptr) {
        ELOG_ERROR_T("Failed to create the plugin.");
        dlclose(plugin_handle_);
        return false;
    }

    // Register frame callback
    plugin_->RegisterFrameCallback(this);

    std::unordered_map<std::string, std::string> plugin_config_map(
      {{"AnalyticsVersion", "1"}});
    plugin_->PluginInit(plugin_config_map);

    if (m_format == FRAME_FORMAT_I420)
        m_bufferManager.reset(new I420BufferManager(3));

    if (m_outFrameRate != 0) {
        m_clock = Clock::GetRealTimeClock();

        m_jobTimer.reset(new JobTimer(m_outFrameRate, this));
        m_jobTimer->start();
    }

    return true;
}

bool FrameAnalyzer::filterFrame(const Frame& frame)
{
    if (m_lastWidth != frame.additionalInfo.video.width
            || m_lastHeight != frame.additionalInfo.video.height) {
        ELOG_DEBUG_T("Stream(%s) resolution changed, %dx%d -> %dx%d"
                , getFormatStr(frame.format)
                , m_lastWidth, m_lastHeight
                , frame.additionalInfo.video.width, frame.additionalInfo.video.height
                );

        m_lastWidth = frame.additionalInfo.video.width;
        m_lastHeight = frame.additionalInfo.video.height;
    }

    return false;
}

void FrameAnalyzer::onFrame(const Frame& frame)
{
    if (filterFrame(frame))
        return;

    ELOG_TRACE_T("onFrame, format(%s), resolution(%dx%d), timestamp(%u)"
            , getFormatStr(frame.format)
            , frame.additionalInfo.video.width
            , frame.additionalInfo.video.height
            , frame.timeStamp / 90
            );

    if (!m_outFrameRate) {
        if (frame.format == m_format
                && (m_outWidth == frame.additionalInfo.video.width || m_outWidth == 0)
                && (m_outHeight == frame.additionalInfo.video.height || m_outHeight == 0)) {
            deliverFrame(frame);
            return;
        }
    }

    uint32_t width = (m_outWidth == 0 ? frame.additionalInfo.video.width : m_outWidth);
    uint32_t height = (m_outHeight == 0 ? frame.additionalInfo.video.height : m_outHeight);


    if (m_format == FRAME_FORMAT_I420) {
        if (frame.format == FRAME_FORMAT_I420) {
            VideoFrame *srcFrame = (reinterpret_cast<VideoFrame *>(frame.payload));
            std::unique_ptr<owt::analytics::AnalyticsBuffer> newFrame(new owt::analytics::AnalyticsBuffer());
            newFrame->buffer = new uint8_t[width * height * 3 / 2 + 1];
            memset(newFrame->buffer, 0, width * height * 3 / 2 + 1);
            newFrame->width = width;
            newFrame->height = height;
            rtc::scoped_refptr<webrtc::VideoFrameBuffer> i420Buffer = srcFrame->video_frame_buffer();
            memcpy(newFrame->buffer, i420Buffer->DataY(), height * width);
            memcpy(newFrame->buffer + height * width , i420Buffer->DataU(), height * width / 4);
            memcpy(newFrame->buffer + height * width * 5 / 4, i420Buffer->DataV(), height * width / 4);
            if (plugin_) {
                plugin_->ProcessFrameAsync(std::move(newFrame));
                return;
            }
        }
    } else {
        ELOG_ERROR_T("Invalid format, input %d(%s), output %d(%s)"
                , frame.format, getFormatStr(frame.format)
                , m_format, getFormatStr(m_format)
                );

        return;
    }
    return;
}

void FrameAnalyzer::OnPluginFrame(std::unique_ptr<owt::analytics::AnalyticsBuffer> pluginFrame) {
    int width = pluginFrame->width;
    int height = pluginFrame->height; 
    rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer = m_bufferManager->getFreeBuffer(width, height);
    if (!i420Buffer) {
        ELOG_ERROR_T("No valid i420Buffer");
        return;
    }
    // Copy over to i420Buffer
    memcpy(i420Buffer->MutableDataY(), pluginFrame->buffer, width * height);
    memcpy(i420Buffer->MutableDataU(), pluginFrame->buffer + width * height, width * height / 4);
    memcpy(i420Buffer->MutableDataV(), pluginFrame->buffer + width * height * 5 / 4, width * height / 4);
    SendFrame(i420Buffer, 0); 
    return;
}

void FrameAnalyzer::SendFrame(rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer, uint32_t timeStamp)
{
    owt_base::Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));

    webrtc::VideoFrame i420Frame(i420Buffer, timeStamp, 0, webrtc::kVideoRotation_0);

    outFrame.format = FRAME_FORMAT_I420;
    outFrame.payload = reinterpret_cast<uint8_t*>(&i420Frame);
    outFrame.length = 0;
    outFrame.additionalInfo.video.width = i420Frame.width();
    outFrame.additionalInfo.video.height = i420Frame.height();
    outFrame.timeStamp = timeStamp;

    ELOG_TRACE_T("sendI420Frame, %dx%d",
            outFrame.additionalInfo.video.width,
            outFrame.additionalInfo.video.height);

    deliverFrame(outFrame);
}

void FrameAnalyzer::onTimeout()
{
    uint32_t timeStamp = kMsToRtpTimestamp * m_clock->TimeInMilliseconds();;

    if (m_format == FRAME_FORMAT_I420) {
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer;
        {
            boost::shared_lock<boost::shared_mutex> lock(m_mutex);
            i420Buffer = m_activeI420Buffer;
        }
        if (i420Buffer)
            SendFrame(i420Buffer, timeStamp);
        return;
    }
}

}//namespace owt_base
