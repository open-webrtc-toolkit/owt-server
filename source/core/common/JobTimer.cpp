// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "JobTimer.h"

#include <boost/thread.hpp>
#include <mutex>

namespace {

// Global thread for timing
boost::asio::io_service g_service;
boost::asio::io_service::work g_work(g_service);
boost::scoped_ptr<boost::thread> g_timingThread;
std::once_flag g_startOnce;

void startTimingThread()
{
    g_timingThread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &g_service)));
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
    m_timer.reset(new boost::asio::deadline_timer(g_service, boost::posix_time::milliseconds(m_interval)));
    m_timer->async_wait(boost::bind(&JobTimer::onTimeout, this, boost::asio::placeholders::error));
}

JobTimer::~JobTimer()
{
    stop();
}

void JobTimer::start()
{
    // Keep the legacy interface work
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
    m_timer->wait();
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
