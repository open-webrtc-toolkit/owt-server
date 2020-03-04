#ifndef LTRACE_H
#define LTRACE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

static int mTraceFD = -1;

static int getTraceFD()
{
    if(mTraceFD < 0) {
        int fd;

        fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);
        if(fd < 0) {
            fprintf(stdout, "Init ltrace failed\n");
            mTraceFD = STDOUT_FILENO;
        } else {
            fprintf(stdout, "Init ltrace successful\n");
            mTraceFD = fd;
        }
    }

    return mTraceFD;
}

#define LTRACE_WRITE(format, args...) \
    do { \
        int fd; \
        fd = getTraceFD(); \
        if(fd > STDOUT_FILENO) { \
            char buf[1024]; \
            size_t len; \
            size_t writed_len; \
            len = snprintf(buf, 1024, format, ##args); \
            writed_len = write(fd, buf, len); \
            if (writed_len != len) {\
                ; \
            } \
        } \
    } while(0)

#define LTRACE_ASYNC_BEGIN(name, cookie)    LTRACE_WRITE("S|%d|%s|%lu\n", getpid(), name, (unsigned long)cookie)
#define LTRACE_ASYNC_END(name, cookie)      LTRACE_WRITE("F|%d|%s|%lu\n", getpid(), name, (unsigned long)cookie)

#endif
