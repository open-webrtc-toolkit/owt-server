/* <COPYRIGHT_TAG> */
#include "log_function.h"

#ifdef ENABLE_VA

#include <stdio.h>
#include <sys/time.h>

long get_current_time()
{
    struct timeval tv;
    long ms = 0;
    int ret = gettimeofday(&tv, NULL);
    if (ret == 0)
    {
        ms = tv.tv_sec * 1000000 + tv.tv_usec;
    }
    return ms;
}

#endif
