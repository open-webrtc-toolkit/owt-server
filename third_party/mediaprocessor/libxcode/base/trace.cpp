/* <COPYRIGHT_TAG> */

/**
 *\file trace.cpp
 *\brief Implementation for Trace
 */

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "trace.h"
#include "trace_file.h"
#include "trace_filter.h"
#include "trace_info.h"

using namespace std;

TracePool Trace::pool_;
TraceFilter Trace::filter;
bool Trace::running_ = false;
std::queue<TraceInfo *> Trace::queue_;
static Mutex mutex;
pthread_t Trace::thread;

Trace::Trace(AppModId id, TraceLevel lev, const char *file, int line):
    mod_(id), level_(lev), file_(file), line_(line)
{
    //file is a relative path, but only need to display the filename
    const char *pos = strrchr(file, '/');

    if (pos) {
        file_ = pos + 1;
    }
}

void Trace::operator()(const char *fmt, ...)
{
    if (!running_) {
        return;
    }

    if (!filter.enable(mod_, level_, 0)) {
        return;
    }

    char temp[MAX_DESC_LEN] = {0};
    char buffer[MAX_DESC_LEN] = {0};
    va_list args;
    va_start(args, fmt);
#ifdef DEBUG

    if (snprintf(temp, MAX_DESC_LEN - 1, "%s,%d: %s", file_, line_, fmt) < 0)
#else
    if (snprintf(temp, MAX_DESC_LEN - 1, "%s", fmt) < 0)
#endif
    {
        temp[MAX_DESC_LEN - 1] = 0;
    }

    if (vsnprintf(buffer, MAX_DESC_LEN - 1, temp, args) < 0) {
        buffer[MAX_DESC_LEN - 1] = 0;
    }

    va_end(args);
    TraceInfo *info = new TraceInfo(mod_, level_, buffer);

    if (info == NULL) {
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        queue_.push(info);
    }
}

void Trace::operator()(TraceBase *obj, const char *fmt, ...)
{
    if (!running_) {
        return;
    }

    if (!filter.enable(mod_, level_, obj)) {
        return;
    }

    char temp[MAX_DESC_LEN] = {0};
    char buffer[MAX_DESC_LEN] = {0};
    va_list args;
    va_start(args, fmt);
#ifdef DEBUG
    if (snprintf(temp, MAX_DESC_LEN - 1, "%s,%d: %s", file_, line_, fmt) < 0)
#else
    if (snprintf(temp, MAX_DESC_LEN - 1, "%s", fmt) < 0)
#endif
    {
        temp[MAX_DESC_LEN - 1] = 0;
    }

    if (vsnprintf(buffer, MAX_DESC_LEN - 1, temp, args) < 0) {
        buffer[MAX_DESC_LEN - 1] = 0;
    }

    va_end(args);
    TraceInfo *info = new TraceInfo(mod_, level_, buffer);

    if (info == NULL) {
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        queue_.push(info);
    }
}

void Trace::start()
{
    running_ = true;
    pthread_create(&thread, NULL, worker_thread, NULL);
    TRC_DEBUG("Succeed to create trace thread");
}

void Trace::stop()
{
    running_ = false;
    pthread_join(thread, NULL);
    TRC_DEBUG("Stop trace thread successfully.");
}

void *Trace::worker_thread(void *arg)
{
    TraceInfo *info = NULL;

    while (running_) {
        usleep(2*1000);

        while (!queue_.empty()) {
            Locker<Mutex> lock(mutex);
            info = queue_.front();
            pool_.add(info);
            TraceFile::instance()->add_trace(info);
            queue_.pop();
            info = NULL;
        }
    }

    pool_.clear();
    return 0;
}


