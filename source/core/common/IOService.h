// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef IOService_h
#define IOService_h

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <memory>

namespace owt_base {

// Wrapped io_service for transport usage
class IOService {
public:
    IOService();
    virtual ~IOService();

    // Get in-process counted tasks number
    int getInProcessCount() const { return m_count; }
    // Post counted tasks
    void post(std::function<void()> task);
    // Get raw io_service
    boost::asio::io_service& service() { return m_service; }

private:
    std::atomic<int> m_count;
    boost::asio::io_service m_service;
    boost::asio::io_service::work m_work;
    boost::thread m_thread;
};

// Get a IOService from service pool
std::shared_ptr<IOService> getIOService();

} /* namespace owt_base */

#endif /* IOService_h */
