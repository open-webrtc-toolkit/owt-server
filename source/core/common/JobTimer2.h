// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef JobTimer_h
#define JobTimer_h

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

class JobTimerListener {
public:
    virtual void onTimeout() = 0;
};

class JobTimer {
public:
    JobTimer(unsigned int frequency, JobTimerListener* listener)
        : m_isClosing(false)
        , m_isRunning(false)
        , m_interval(1000 / frequency)
        , m_listener(listener)
    {
        m_timer.reset(new boost::asio::deadline_timer(m_ioService, boost::posix_time::milliseconds(m_interval)));
        m_timer->async_wait(boost::bind(&JobTimer::onTimeout, this, boost::asio::placeholders::error));
        start();
    }

    ~JobTimer()
    {
        stop();
    }

    void start()
    {
        if (!m_isRunning) {
            m_timingThread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));
            m_isRunning = true;
        }
    }

    void stop()
    {
        m_timer->cancel();

        if (m_isRunning) {
            m_isClosing = true;
            m_timingThread->join();
            m_isRunning = false;
            m_isClosing = false;
        }
    }

private:
    void onTimeout(const boost::system::error_code& ec)
    {
        if (!ec) {
            if (!m_isClosing) {
                m_timer->expires_from_now(boost::posix_time::milliseconds(m_interval));
                handleJob();
                m_timer->async_wait(boost::bind(&JobTimer::onTimeout, this, boost::asio::placeholders::error));
            }
        }
    }

    void handleJob()
    {
        if (m_listener)
            m_listener->onTimeout();
    }

private:
    std::atomic<bool> m_isClosing;
    bool m_isRunning;

    unsigned int m_interval;
    JobTimerListener* m_listener;

    boost::scoped_ptr<boost::thread> m_timingThread;
    boost::asio::io_service m_ioService;
    boost::scoped_ptr<boost::asio::deadline_timer> m_timer;
};

#endif
