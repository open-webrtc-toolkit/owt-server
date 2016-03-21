/* <COPYRIGHT_TAG> */
#ifdef ENABLE_VA
#ifndef _LOG_FUNCTION_H_

#define _LOG_FUNCTION_H_
#define RELEASE 0

#include <syslog.h>

#define MY_LOG_DEBUG    0
#define MY_LOG_INFO     1
#define MY_LOG_WARN     2
#define MY_LOG_ERROR    3

long get_current_time();

#if RELEASE

#define LOG_OPEN(name) do{openlog(name, LOG_PID|LOG_ODELAY, LOG_USER);}while(0)
#define LOG_FUNCTION(level,format,...) do{if(level>=MY_LOG_INFO) \
                        syslog(LOG_INFO, "[%s@%s,%d](usec:%ld)" format "\n", \
                        __func__, __FILE__, __LINE__, get_current_time(),##__VA_ARGS__);}while(0)
#define LOG_CLOSE()  do{closelog();}while(0)

#else

#define LOG_OPEN(name) do{openlog(name, LOG_PID|LOG_ODELAY, LOG_USER);}while(0)
#define LOG_FUNCTION(level,format,...) do{if(level>=MY_LOG_DEBUG) \
                        syslog(LOG_INFO, "[%s@%s,%d](usec:%ld)" format "\n", \
                        __func__, __FILE__, __LINE__, get_current_time(),##__VA_ARGS__);}while(0)
#define LOG_CLOSE()  do{closelog();}while(0)

#endif

#endif //_LOG_FUNCTION_H_

#endif
