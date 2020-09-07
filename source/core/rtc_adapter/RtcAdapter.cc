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

#include <system_wrappers/include/clock.h>

namespace rtc_adapter {

namespace {

const uint32_t kModuleThreadsNum = 16;

std::once_flag g_startOnce;
std::vector<std::unique_ptr<webrtc::ProcessThread>> g_moduleThreads;

void initModuleThreads()
{
    for (uint32_t i = 0; i < kModuleThreadsNum; i++) {
        g_moduleThreads.push_back(webrtc::ProcessThread::Create("ModuleProcessThread"));
        g_moduleThreads[i]->Start();
    }
}

std::unique_ptr<webrtc::ProcessThread> getThreadProxy()
{
    // Initialize global threads once
    std::call_once(g_startOnce, initModuleThreads);
    uint32_t i = webrtc::Clock::GetRealTimeClock()->TimeInMilliseconds() %
        kModuleThreadsNum;

    std::unique_ptr<webrtc::ProcessThread> moduleThreadProxy =
        std::make_unique<ProcessThreadProxy>(g_moduleThreads[i].get());

    return moduleThreadProxy;
}

}

class RtcAdapterImpl : public RtcAdapter,
                       public CallOwner {
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

    // Implement CallOwner
    std::shared_ptr<webrtc::Call> call() override { return m_call; }
    std::shared_ptr<webrtc::TaskQueueFactory> taskQueueFactory() override
    {
        return m_taskQueueFactory;
    }
    std::shared_ptr<rtc::TaskQueue> taskQueue() override { return m_taskQueue; }
    std::shared_ptr<webrtc::RtcEventLog> eventLog() override { return m_eventLog; }

private:
    void initCall();

    std::shared_ptr<webrtc::TaskQueueFactory> m_taskQueueFactory;
    std::shared_ptr<rtc::TaskQueue> m_taskQueue;
    std::shared_ptr<webrtc::RtcEventLog> m_eventLog;
    std::shared_ptr<webrtc::Call> m_call;
};

RtcAdapterImpl::RtcAdapterImpl()
    : m_taskQueueFactory(createStaticTaskQueueFactory())
    , m_taskQueue(std::make_shared<rtc::TaskQueue>(m_taskQueueFactory->CreateTaskQueue(
          "CallTaskQueue",
          webrtc::TaskQueueFactory::Priority::NORMAL)))
    , m_eventLog(std::make_shared<webrtc::RtcEventLogNull>())
{
}

RtcAdapterImpl::~RtcAdapterImpl()
{
}

void RtcAdapterImpl::initCall()
{
    m_taskQueue->PostTask([this]() {
        // Initialize call
        if (!m_call) {
            std::unique_ptr<webrtc::ProcessThread> moduleThreadProxy = getThreadProxy();

            webrtc::Call::Config call_config(m_eventLog.get());
            call_config.task_queue_factory = m_taskQueueFactory.get();
            m_call.reset(webrtc::Call::Create(
                call_config, webrtc::Clock::GetRealTimeClock(),
                std::move(moduleThreadProxy),
                webrtc::ProcessThread::Create("PacerThread")));
        }
    });
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
    return new VideoSendAdapterImpl(this, config);
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
