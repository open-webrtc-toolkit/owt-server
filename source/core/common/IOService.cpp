// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "IOService.h"


namespace owt_base {

static constexpr uint32_t kServiceNum = 4;
static boost::mutex g_serviceMutex;
static std::vector<std::shared_ptr<IOService>> g_services;

IOService::IOService()
    : m_count(0)
    , m_service()
    , m_work(m_service)
    , m_thread(boost::bind(&boost::asio::io_service::run, &m_service))
{
}

IOService::~IOService()
{
    m_service.stop();
    m_thread.join();
}

void IOService::post(std::function<void()> task)
{
    m_count.fetch_add(1);
    m_service.post([this, task]()
    {
        task();
        m_count.fetch_sub(1);
    });
}

std::shared_ptr<IOService> getIOService()
{
    boost::mutex::scoped_lock lock(g_serviceMutex);
    if (g_services.empty()) {
        for (size_t i = 0; i < kServiceNum; i++) {
            g_services.push_back(std::make_shared<IOService>());
        }
    }
    int i = std::rand()/((RAND_MAX + 1u)/kServiceNum);
    return g_services[i];
}

}
/* namespace owt_base */
