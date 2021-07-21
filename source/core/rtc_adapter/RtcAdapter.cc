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
#include <system_wrappers/include/field_trial.h>

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

static rtc::scoped_refptr<webrtc::SharedModuleThread> g_moduleThread;

static std::unique_ptr<RTCProcessThread> g_pacerThread;

static std::unique_ptr<webrtc::FieldTrialBasedConfig> g_fieldTrial= []()
{
    auto config = std::make_unique<webrtc::FieldTrialBasedConfig>();
    // webrtc::field_trial::InitFieldTrialsFromString("WebRTC-TaskQueuePacer/Enabled/");
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


    typedef std::shared_ptr<webrtc::Call> CallPtr;

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

private:
    void initCall();

    std::shared_ptr<CallPtr> m_callPtr;
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
