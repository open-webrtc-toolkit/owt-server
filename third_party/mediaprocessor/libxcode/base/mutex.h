/* <COPYRIGHT_TAG> */

#ifndef _MUTEX_H_
#define _MUTEX_H_

#if defined(PLATFORM_OS_WINDOWS)
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "mutex_locker.h"
#include "platform.h"

class Mutex
{
public:
    Mutex();
    ~Mutex();

   /**
    * lock the mutex
    */
    bool Lock();

   /**
    * unlock the mutex
    */
    bool Unlock();

   /**
    * try lock the mutex
    * @return
    *      true, if lock successed
    */
    bool TryLock();

   /**
    * try get the condition variable in given time
    * @param s
    *      The timeout value in seconds
    * @param ms
    *      The timeout value in milliseconds
    * @return
    *      true, if lock successed
    */
    bool TimedWait(unsigned s, unsigned ms = 0);

   /**
    * try set the condition variable
    * @return
    *      true, if set successed
    */
    bool CondSignal();

#if defined(PLATFORM_OS_WINDOWS)
    HANDLE InnerMutex() { return m_hmutex; }
#else
    pthread_mutex_t* InnerMutex() { return &m_mutex; }
#endif


private:
#if defined(PLATFORM_OS_WINDOWS)
    HANDLE m_hmutex;
    HANDLE m_hcond;
#else
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
#endif

};

#endif
