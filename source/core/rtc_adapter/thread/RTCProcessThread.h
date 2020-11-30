// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_THREAD_RTC_PROCESS_THREAD__
#define RTC_ADAPTER_THREAD_RTC_PROCESS_THREAD__

#include <modules/utility/include/process_thread.h>

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

} // namespace rtc_adapter

#endif
