// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "JobTimer.h"

#include <boost/thread.hpp>
#include <mutex>

namespace {

// Global thread for timing
class IOServiceThread {
public:
    IOServiceThread()
    : m_service{}
    , m_work{m_service}
    , m_thread{new boost::thread(boost::bind(&boost::asio::io_service::run, &m_service))} {}

    ~IOServiceThread()
    {
        m_service.stop();
        m_thread->join();
    }

    boost::asio::io_service& service()
    {
        return m_service;
    }

private:
    boost::asio::io_service m_service;
    boost::asio::io_service::work m_work;
    boost::scoped_ptr<boost::thread> m_thread;
};

boost::scoped_ptr<IOServiceThread> g_timingThread;
std::once_flag g_startOnce;

void startTimingThread()
{
    g_timingThread.reset(new IOServiceThread());
}

}

JobTimer::JobTimer(unsigned int frequency, JobTimerListener* listener)
    : m_isClosing(false)
    , m_isRunning(false)
    , m_interval(1000 / frequency)
    , m_listener(listener)
{
    // Start the global thread once
    std::call_once(g_startOnce, startTimingThread);
    m_timer.reset(new boost::asio::deadline_timer(
        g_timingThread->service(), boost::posix_time::milliseconds(m_interval)));
    m_timer->async_wait(boost::bind(&JobTimer::onTimeout, this, boost::asio::placeholders::error));
}

JobTimer::~JobTimer()
{
    stop();
}

void JobTimer::start()
{
    // Keep the legacy interface working
    if (!m_isRunning) {
        m_isRunning = true;
    }
}

void JobTimer::stop()
{
    m_timer->cancel();

    if (m_isRunning) {
        m_isClosing = true;
        m_isRunning = false;
    }
    boost::system::error_code ec;
    m_timer->wait(ec);
    if (ec) {
        // Ignore error during stop
    }
}

void JobTimer::onTimeout(const boost::system::error_code& ec)
{
    if (!ec) {
        if (!m_isClosing) {
            handleJob();
            m_timer->expires_from_now(boost::posix_time::milliseconds(m_interval));
            m_timer->async_wait(boost::bind(&JobTimer::onTimeout, this, boost::asio::placeholders::error));
        }
    }
}

void JobTimer::handleJob()
{
    if (m_listener)
        m_listener->onTimeout();
}
