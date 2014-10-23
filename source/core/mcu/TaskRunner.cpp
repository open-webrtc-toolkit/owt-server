/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#include "TaskRunner.h"

#include <boost/bind.hpp>
#include <webrtc/system_wrappers/interface/trace.h>

namespace mcu {

TaskRunner::TaskRunner(int interval) :
    m_interval(interval),
    m_taskLock(CriticalSectionWrapper::CreateCriticalSection())
{
}

TaskRunner::~TaskRunner()
{
}

void TaskRunner::start()
{
    m_timer.reset(new boost::asio::deadline_timer(m_ioService, boost::posix_time::milliseconds(m_interval)));
    m_timer->async_wait(boost::bind(&TaskRunner::run, this, boost::asio::placeholders::error));
    m_thread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));
}

void TaskRunner::run(const boost::system::error_code& ec)
{
    if (!ec) {
        {
            CriticalSectionScoped cs(m_taskLock.get());
            for (std::list<Module*>::iterator taskIter = m_taskList.begin();
                    taskIter != m_taskList.end();
                    ++taskIter) {
                if ((*taskIter)->TimeUntilNextProcess() <= 0)
                    (*taskIter)->Process();
            }
        }
        m_timer->expires_at(m_timer->expires_at() + boost::posix_time::milliseconds(m_interval));
        m_timer->async_wait(boost::bind(&TaskRunner::run, this, boost::asio::placeholders::error));
    }
}

void TaskRunner::stop()
{
    if (m_timer)
        m_timer->cancel();
    m_ioService.stop();
}

void TaskRunner::registerModule(Module* module)
{
    CriticalSectionScoped cs(m_taskLock.get());
    m_interval = MIN(m_interval, module->TimeUntilNextProcess());
    WEBRTC_TRACE(kTraceStateInfo, kTraceUtility,
                 -1,
                "registering module, interval changed to %d", m_interval);

    m_taskList.push_back(module);
}

void TaskRunner::unregisterModule(Module* module)
{
    CriticalSectionScoped cs(m_taskLock.get());
    m_taskList.remove(module);
}

} /* namespace mcu */
