// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef WebRTCTaskRunner_h
#define WebRTCTaskRunner_h

#include <rtc_base/location.h>
#include <modules/utility/include/process_thread.h>

namespace owt_base {

/**
 * This thread is now responsible for running the non critical process for each modules.
 * It's now simply a wrapper of the webrtc ProcessThread, and we need it because it's shared
 * by different objects and we want to manage its lifetime automatically.
 */
class WebRTCTaskRunner {
public:
    WebRTCTaskRunner(const char* task_name);
    ~WebRTCTaskRunner();

    void Start();
    void Stop();
    void RegisterModule(webrtc::Module*);
    void DeRegisterModule(webrtc::Module*);

    webrtc::ProcessThread* unwrap();

private:
    std::unique_ptr<webrtc::ProcessThread> m_processThread;
};

inline WebRTCTaskRunner::WebRTCTaskRunner(const char* task_name)
    : m_processThread(webrtc::ProcessThread::Create(task_name))
{
}

inline WebRTCTaskRunner::~WebRTCTaskRunner()
{
}

inline void WebRTCTaskRunner::Start()
{
    m_processThread->Start();
}

inline void WebRTCTaskRunner::Stop()
{
    m_processThread->Stop();
}

inline void WebRTCTaskRunner::RegisterModule(webrtc::Module* module)
{
    m_processThread->RegisterModule(module, RTC_FROM_HERE);
}

inline void WebRTCTaskRunner::DeRegisterModule(webrtc::Module* module)
{
    m_processThread->DeRegisterModule(module);
}

inline webrtc::ProcessThread* WebRTCTaskRunner::unwrap()
{
    return m_processThread.get();
}

} /* namespace owt_base */

#endif /* WebRTCTaskRunner_h */
