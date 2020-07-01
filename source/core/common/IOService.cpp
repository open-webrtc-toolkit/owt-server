// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "IOService.h"


namespace owt_base {

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
    return std::make_shared<IOService>();
}

}
/* namespace owt_base */
