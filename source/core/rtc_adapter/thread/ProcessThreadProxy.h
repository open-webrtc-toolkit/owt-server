// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_THREAD_PROCESS_THREAD_PROXY_
#define RTC_ADAPTER_THREAD_PROCESS_THREAD_PROXY_

#include <modules/utility/include/process_thread.h>
#include <modules/include/module.h>
#include <rtc_base/checks.h>
#include <unordered_map>

namespace rtc_adapter {

// Proxy the ProcessThreadAttached method to actual thread
class ModuleProxy : public webrtc::Module {
public:
    ModuleProxy(webrtc::ProcessThread* thread, webrtc::Module* module)
    : m_thread(thread)
    , m_module(module)
    {}

    virtual int64_t TimeUntilNextProcess() override
    {
        return m_module->TimeUntilNextProcess();
    }

    virtual void Process() override
    {
        m_module->Process();
    }

    virtual void ProcessThreadAttached(webrtc::ProcessThread* processThread) override
    {
        m_module->ProcessThreadAttached(m_thread);
    }
private:
    webrtc::ProcessThread* m_thread;
    webrtc::Module* m_module;
};

// ProcessThreadProxy is a proxy for global ProcessThread
class ProcessThreadProxy : public webrtc::ProcessThread {
public:
    ProcessThreadProxy(const char* taskName);

    // Implements ProcessThread
    virtual void Start() override;

    // Implements ProcessThread
    virtual void Stop() override;

    // Implements ProcessThread
    // Call actual ProcessThread's WakeUp
    virtual void WakeUp(webrtc::Module* module) override;

    // Implements ProcessThread
    // Call actual ProcessThread's PostTask
    virtual void PostTask(std::unique_ptr<webrtc::QueuedTask> task) override;

    // Implements ProcessThread
    // Call actual ProcessThread's RegisterModule
    virtual void RegisterModule(webrtc::Module* module, const rtc::Location& from) override;

    // Implements ProcessThread
    // Call actual ProcessThread's DeRegisterModule
    virtual void DeRegisterModule(webrtc::Module* module) override;
private:
    std::string m_name;
    std::unordered_map<webrtc::Module*, std::unique_ptr<ModuleProxy>> m_moduleProxyMap;
};

} // namespace rtc_adapter

#endif
