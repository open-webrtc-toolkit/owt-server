// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "ProcessThreadProxy.h"
#include "RTCProcessThread.h"
#include <mutex>

namespace rtc_adapter {

static std::mutex g_threadsMutex;
static std::unordered_map<std::string,
    std::unique_ptr<RTCProcessThread>> g_processThreads;
static std::unordered_map<std::string, int> g_threadCount;

ProcessThreadProxy::ProcessThreadProxy(const char* taskName)
    : m_name(taskName)
{
}

void ProcessThreadProxy::Start()
{
    // The lock is unnecessary if called from same thread
    if (g_processThreads.count(m_name) == 0) {
        g_processThreads[m_name] = std::make_unique<RTCProcessThread>(m_name.c_str());
        g_threadCount[m_name] = 0;
    }
    g_threadCount[m_name]++;
}

void ProcessThreadProxy::Stop()
{
    // The lock is unnecessary if called from same thread
    if (g_threadCount.count(m_name) > 0) {
        g_threadCount[m_name]--;
        if (g_threadCount[m_name] == 0) {
            g_processThreads.erase(m_name);
        }
    }
    
}

void ProcessThreadProxy::WakeUp(webrtc::Module* module)
{
    // The lock is unnecessary if called from same thread
    if (m_moduleProxyMap.count(module) > 0) {
        webrtc::Module* moduleProxy = m_moduleProxyMap[module].get();
        if (g_processThreads.count(m_name) > 0) {
            g_processThreads[m_name]->unwrap()->WakeUp(moduleProxy);
        }
    }
}

void ProcessThreadProxy::PostTask(std::unique_ptr<webrtc::QueuedTask> task)
{
    // The lock is unnecessary if called from same thread
    if (g_processThreads.count(m_name) > 0) {
        g_processThreads[m_name]->unwrap()->PostTask(std::move(task));
    }
}

void ProcessThreadProxy::RegisterModule(webrtc::Module* module, const rtc::Location& from)
{
    // The lock is unnecessary if called from same thread
    if (m_moduleProxyMap.count(module) == 0) {
        m_moduleProxyMap.emplace(module,
            std::make_unique<ModuleProxy>(this, module));
    }
    webrtc::Module* moduleProxy = m_moduleProxyMap[module].get();
    if (g_processThreads.count(m_name) > 0) {
        g_processThreads[m_name]->unwrap()->RegisterModule(moduleProxy, from);
    }
}

void ProcessThreadProxy::DeRegisterModule(webrtc::Module* module)
{
    // The lock is unnecessary if called from same thread
    if (m_moduleProxyMap.count(module) > 0) {
        webrtc::Module* moduleProxy = m_moduleProxyMap[module].get();
        if (g_processThreads.count(m_name) > 0) {
            g_processThreads[m_name]->unwrap()->DeRegisterModule(moduleProxy);
        }
        m_moduleProxyMap.erase(module);
    }
}

} // namespace rtc_adapter
