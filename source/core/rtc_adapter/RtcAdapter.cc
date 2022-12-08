// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <AdapterInternalDefinitions.h>
#include <AudioSendAdapter.h>
#include <RtcAdapter.h>
#include <VideoReceiveAdapter.h>
#include <VideoSendAdapter.h>
#include <thread/ProcessThreadProxy.h>
#include <thread/StaticTaskQueueFactory.h>

#include <memory>
#include <mutex>

#include <call/rtp_transport_controller_send.h>
#include <system_wrappers/include/clock.h>
#include <system_wrappers/include/field_trial.h>

namespace rtc_adapter {

static rtc::scoped_refptr<webrtc::SharedModuleThread> g_moduleThread;

static std::unique_ptr<webrtc::FieldTrialBasedConfig> g_fieldTrial= []()
{
    auto config = std::make_unique<webrtc::FieldTrialBasedConfig>();
    /**/
    webrtc::field_trial::InitFieldTrialsFromString(
        "WebRTC-KeyframeInterval/"
        "max_wait_for_keyframe_ms:500,"
        "max_wait_for_frame_ms:1500/"
        "WebRTC-TaskQueuePacer/Enabled/");
    return config;
}();

static std::shared_ptr<webrtc::RtcEventLog> g_eventLog =
    std::make_shared<webrtc::RtcEventLogNull>();

static std::shared_ptr<webrtc::TaskQueueFactory> g_taskQueueFactory =
    createStaticTaskQueueFactory();
static std::shared_ptr<rtc::TaskQueue> g_taskQueue =
    std::make_shared<rtc::TaskQueue>(g_taskQueueFactory->CreateTaskQueue(
        "CallTaskQueue",
        webrtc::TaskQueueFactory::Priority::NORMAL));

static constexpr int kStartBitrateBps = 800000;

class RtcAdapterImpl : public RtcAdapter,
                       public CallOwner,
                       public webrtc::TargetTransferRateObserver,
                       public webrtc::BitrateStatisticsObserver {
public:
    RtcAdapterImpl();
    virtual ~RtcAdapterImpl();

    // Implement RtcAdapter
    VideoReceiveAdapter* createVideoReceiver(const Config&) override;
    void destoryVideoReceiver(VideoReceiveAdapter*) override;
    VideoSendAdapter* createVideoSender(const Config&) override;
    void destoryVideoSender(VideoSendAdapter*) override;
    AudioReceiveAdapter* createAudioReceiver(const Config&) override;
    void destoryAudioReceiver(AudioReceiveAdapter*) override;
    AudioSendAdapter* createAudioSender(const Config&) override;
    void destoryAudioSender(AudioSendAdapter*) override;


    typedef std::shared_ptr<webrtc::Call> CallPtr;
    typedef std::shared_ptr<webrtc::RtpTransportControllerSendInterface> ControllerSendPtr;

    // Implement CallOwner
    std::shared_ptr<webrtc::Call> call() override
    {
        return m_callPtr ? (*m_callPtr) : nullptr;
    }
    std::shared_ptr<webrtc::TaskQueueFactory> taskQueueFactory() override
    {
        return g_taskQueueFactory;
    }
    std::shared_ptr<rtc::TaskQueue> taskQueue() override { return g_taskQueue; }
    std::shared_ptr<webrtc::RtcEventLog> eventLog() override { return g_eventLog; }
    webrtc::WebRtcKeyValueConfig* trial() override { return g_fieldTrial.get(); }
    ControllerSendPtr rtpTransportController() override
    {
        return m_transportControllerSend;
    }
    uint32_t estimatedBandwidth(uint32_t ssrc) override;
    void registerVideoSender(uint32_t ssrc) override { m_videoSenderNumber++; }
    void deregisterVideoSender(uint32_t ssrc) override { m_videoSenderNumber--; }

    //Implements webrtc::TargetTransferRateObjserver
    void OnTargetTransferRate(webrtc::TargetTransferRate) override;
    //Implements webrtc::BitrateStatisticsObserver
    void Notify(uint32_t total_bitrate_bps,
                uint32_t retransmit_bitrate_bps,
                uint32_t ssrc) override;

private:
    void initCall();
    void initRtpTransportController();

    std::shared_ptr<CallPtr> m_callPtr;

    // For sender
    ControllerSendPtr m_transportControllerSend = nullptr;
    std::mutex m_bitrateMutex;
    std::map<uint32_t, uint32_t> m_sendBitrate;
    std::map<uint32_t, bool> m_adjustBitrate;
    uint32_t m_estimatedBandwidth = 0;

    std::atomic<int> m_videoSenderNumber = {0};
};

RtcAdapterImpl::RtcAdapterImpl()
{
}

RtcAdapterImpl::~RtcAdapterImpl()
{
    if (m_callPtr) {
        std::shared_ptr<CallPtr> pCallPtr = m_callPtr;
        m_callPtr.reset();
        g_taskQueue->PostTask([pCallPtr]() {
            if (*pCallPtr) {
                (*pCallPtr).reset();
            }
        });
    }
}

void RtcAdapterImpl::initCall()
{
    if (m_callPtr) {
        return;
    }
    m_callPtr.reset(new CallPtr());
    std::shared_ptr<CallPtr> pCallPtr = m_callPtr;
    g_taskQueue->PostTask([pCallPtr]() {
        // Initialize call
        if (!(*pCallPtr)) {
            webrtc::Call::Config call_config(g_eventLog.get());
            call_config.task_queue_factory = g_taskQueueFactory.get();
            call_config.trials = g_fieldTrial.get();

            if (!g_moduleThread) {
                g_moduleThread = webrtc::SharedModuleThread::Create(
                    webrtc::ProcessThread::Create("ModuleProcessThread"), nullptr);
            }

            // Empty thread for pacer
            std::unique_ptr<webrtc::ProcessThread> pacerThreadProxy =
                std::make_unique<ProcessThreadProxy>(nullptr);

            (*pCallPtr).reset(webrtc::Call::Create(
                call_config, webrtc::Clock::GetRealTimeClock(),
                g_moduleThread,
                std::move(pacerThreadProxy)));
        }
    });
}

void RtcAdapterImpl::initRtpTransportController()
{
    if (!m_transportControllerSend) {
        RTC_LOG(LS_INFO) << "Init RtptransportcontrollerSend";
        // Empty thread for pacer
        std::unique_ptr<webrtc::ProcessThread> pacerThreadProxy =
            std::make_unique<ProcessThreadProxy>(nullptr);

        webrtc::BitrateConstraints bitrateConstraints;
        bitrateConstraints.start_bitrate_bps = kStartBitrateBps;

        m_transportControllerSend = std::make_shared<webrtc::RtpTransportControllerSend>(
            webrtc::Clock::GetRealTimeClock(), g_eventLog.get(),
            nullptr/*network_state_predicator_factory*/,
            nullptr/*network_controller_factory*/, bitrateConstraints,
            std::move(pacerThreadProxy)/*pacer_thread*/, g_taskQueueFactory.get(), g_fieldTrial.get());
        m_transportControllerSend->RegisterTargetTransferRateObserver(this);
    }
}

void RtcAdapterImpl::OnTargetTransferRate(webrtc::TargetTransferRate msg)
{
    uint32_t target_bitrate_bps = msg.target_rate.bps();
    RTC_LOG(LS_INFO) << "OnTargetTransferRate(bps): " << target_bitrate_bps;
    m_estimatedBandwidth = target_bitrate_bps;
}

void RtcAdapterImpl::Notify(uint32_t total_bitrate_bps,
                            uint32_t retransmit_bitrate_bps,
                            uint32_t ssrc)
{
    std::lock_guard<std::mutex> guard(m_bitrateMutex);
    if (m_sendBitrate.count(ssrc) > 0) {
        if (total_bitrate_bps == 0) {
            if (retransmit_bitrate_bps == 0) {
                m_sendBitrate.erase(ssrc);
                m_adjustBitrate.erase(ssrc);
            } else if (retransmit_bitrate_bps == 1) {
                // Temporarily use this interface for adjust
                m_adjustBitrate[ssrc] = true;
            }
        } else {
            m_sendBitrate[ssrc] = total_bitrate_bps;
        }
    } else {
        m_sendBitrate[ssrc] = total_bitrate_bps;
        m_adjustBitrate[ssrc] = false;
    }
}

uint32_t RtcAdapterImpl::estimatedBandwidth(uint32_t ssrc)
{
    std::lock_guard<std::mutex> guard(m_bitrateMutex);
    uint32_t total = m_estimatedBandwidth;

}

VideoReceiveAdapter* RtcAdapterImpl::createVideoReceiver(const Config& config)
{
    initCall();
    return new VideoReceiveAdapterImpl(this, config);
}

void RtcAdapterImpl::destoryVideoReceiver(VideoReceiveAdapter* video_recv_adapter)
{
    VideoReceiveAdapterImpl* impl = static_cast<VideoReceiveAdapterImpl*>(video_recv_adapter);
    delete impl;
}

VideoSendAdapter* RtcAdapterImpl::createVideoSender(const Config& config)
{
    initRtpTransportController();
    return new VideoSendAdapterImpl(this, config, this);
}
void RtcAdapterImpl::destoryVideoSender(VideoSendAdapter* video_send_adapter)
{
    VideoSendAdapterImpl* impl = static_cast<VideoSendAdapterImpl*>(video_send_adapter);
    delete impl;
}

AudioReceiveAdapter* RtcAdapterImpl::createAudioReceiver(const Config& config)
{
    return nullptr;
}
void RtcAdapterImpl::destoryAudioReceiver(AudioReceiveAdapter* audio_recv_adapter) {}

AudioSendAdapter* RtcAdapterImpl::createAudioSender(const Config& config)
{
    return new AudioSendAdapterImpl(this, config);
}

void RtcAdapterImpl::destoryAudioSender(AudioSendAdapter* audio_send_adapter)
{
    AudioSendAdapterImpl* impl = static_cast<AudioSendAdapterImpl*>(audio_send_adapter);
    delete impl;
}

RtcAdapter* RtcAdapterFactory::CreateRtcAdapter()
{
    return new RtcAdapterImpl();
}

void RtcAdapterFactory::DestroyRtcAdapter(RtcAdapter* adapter) {}

} // namespace rtc_adapter
