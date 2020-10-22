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

class RTCProcessThread {
public:
    RTCProcessThread(const char* task_name)
    : m_processThread(webrtc::ProcessThread::Create(task_name))
    {
        m_processThread->Start();
    }
    ~RTCProcessThread()
    {
        m_processThread->Stop();
    }

    webrtc::ProcessThread* unwrap()
    {
        return m_processThread.get();
    }
private:
    std::unique_ptr<webrtc::ProcessThread> m_processThread;
};

static std::unique_ptr<RTCProcessThread> g_moduleThread
    = std::make_unique<RTCProcessThread>("ModuleProcessThread");
static std::unique_ptr<RTCProcessThread> g_pacerThread
    = std::make_unique<RTCProcessThread>("PacerThread");

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
            webrtc::Call::Config call_config(m_eventLog.get());
            call_config.task_queue_factory = m_taskQueueFactory.get();

            std::unique_ptr<webrtc::ProcessThread> moduleThreadProxy =
                std::make_unique<ProcessThreadProxy>(g_moduleThread->unwrap());
            std::unique_ptr<webrtc::ProcessThread> pacerThreadProxy =
                std::make_unique<ProcessThreadProxy>(g_pacerThread->unwrap());
            m_call.reset(webrtc::Call::Create(
                call_config, webrtc::Clock::GetRealTimeClock(),
                std::move(moduleThreadProxy),
                std::move(pacerThreadProxy)));
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
