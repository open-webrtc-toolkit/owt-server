// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "JobTimer.h"

#include <boost/thread.hpp>
#include <mutex>
#include <unordered_map>

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
            m_timer->expires_at(m_timer->expires_at() +
                                boost::posix_time::milliseconds(m_interval));
            m_timer->async_wait(boost::bind(&JobTimer::onTimeout, this,
                                            boost::asio::placeholders::error));
            handleJob();
        }
    }
}

void JobTimer::handleJob()
{
    if (m_listener)
        m_listener->onTimeout();
}

SharedJobTimer::SharedJobTimer(unsigned int frequency)
    : m_jobTimer(frequency, this)
{
    m_jobTimer.start();
}

SharedJobTimer::~SharedJobTimer()
{
    m_jobTimer.stop();
}

void SharedJobTimer::addListener(JobTimerListener* listener)
{
    if (listener) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_listeners.insert(listener);
    }
}

void SharedJobTimer::removeListener(JobTimerListener* listener)
{
    if (listener) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_listeners.erase(listener);
    }
}

void SharedJobTimer::onTimeout()
{
    boost::mutex::scoped_lock lock(m_mutex);
    for (JobTimerListener* listener : m_listeners) {
        listener->onTimeout();
    }
}

std::shared_ptr<SharedJobTimer> SharedJobTimer::GetSharedFrequencyTimer(unsigned int frequency)
{
    static boost::mutex timersMutex;
    static std::unordered_map<unsigned int, std::shared_ptr<SharedJobTimer>> sharedTimers;

    boost::mutex::scoped_lock lock(timersMutex);
    if (sharedTimers.count(frequency) > 0) {
        return sharedTimers[frequency];
    } else {
        auto sharedTimer = std::make_shared<SharedJobTimer>(frequency);
        sharedTimers.emplace(frequency, sharedTimer);
        return sharedTimer;
    }
}
