// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "TaskRunnerPool.h"

namespace owt_base {

constexpr int kTaskRunnerPoolSize = 4;

TaskRunnerPool& TaskRunnerPool::GetInstance()
{
    static TaskRunnerPool taskRunnerPool;
    return taskRunnerPool;
}

boost::shared_ptr<WebRTCTaskRunner> TaskRunnerPool::GetTaskRunner()
{
    // This function would always be called in Node's main thread
    boost::shared_ptr<WebRTCTaskRunner> runner = m_taskRunners[m_nextRunner];
    m_nextRunner = (m_nextRunner + 1) % m_taskRunners.size();
    return runner;
}

TaskRunnerPool::TaskRunnerPool()
    : m_nextRunner(0)
    , m_taskRunners(kTaskRunnerPoolSize)
{
    for (size_t i = 0; i < m_taskRunners.size(); i++) {
        m_taskRunners[i].reset(new WebRTCTaskRunner("TaskRunner"));
        m_taskRunners[i]->Start();
    }
}

TaskRunnerPool::~TaskRunnerPool()
{
    for (size_t i = 0; i < m_taskRunners.size(); i++) {
        m_taskRunners[i]->Stop();
        m_taskRunners[i].reset();
    }
}

} // namespace owt_base
