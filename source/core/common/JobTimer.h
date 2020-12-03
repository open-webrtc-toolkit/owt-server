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
    JobTimer(unsigned int frequency, JobTimerListener* listener);
    ~JobTimer();

    void start();
    void stop();

private:
    void onTimeout(const boost::system::error_code& ec);
    void handleJob();

private:
    std::atomic<bool> m_isClosing;
    bool m_isRunning;

    unsigned int m_interval;
    JobTimerListener* m_listener;

    boost::scoped_ptr<boost::asio::deadline_timer> m_timer;
};

class SharedJobTimer : public JobTimerListener {
public:
    SharedJobTimer(unsigned int frequency);
    ~SharedJobTimer();

    static std::shared_ptr<SharedJobTimer> GetSharedFrequencyTimer(unsigned int frequency);
    void addListener(JobTimerListener* listener);
    void removeListener(JobTimerListener* listener);

    virtual void onTimeout() override;

private:
    JobTimer m_jobTimer;
    boost::mutex m_mutex;
    std::set<JobTimerListener*> m_listeners;
};

#endif
