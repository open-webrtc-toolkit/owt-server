/* <COPYRIGHT_TAG> */

#include <assert.h>

#include "mutex.h"

Mutex::Mutex ()
{
#if defined(PLATFORM_OS_WINDOWS)
    m_hmutex = CreateMutex(NULL, FALSE, NULL);
    m_hcond = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
    int rs = pthread_mutex_init(&m_mutex, NULL);
    assert(0 == rs);
    int rt = pthread_cond_init(&m_cond, NULL);
    assert(0 == rt);
#endif
}

Mutex::~Mutex ()
{
#if defined(PLATFORM_OS_WINDOWS)
    BOOL rs = CloseHandle(m_hmutex);
    assert(0 != rs);
    BOOL rt = CloseHandle(m_hcond);
    assert(0 != rt);
#else
    int rs = pthread_mutex_destroy(&m_mutex);
    assert(0 == rs);
    int rt = pthread_cond_destroy(&m_cond);
    assert(0 == rt);
#endif
}

bool Mutex::Lock()
{
#if defined(PLATFORM_OS_WINDOWS)
    DWORD rs = WaitForSingleObject(m_hmutex, INFINITE);
    if (WAIT_OBJECT_0 != rs) {
        return false;
    }
    return true;
#else
    int rs = pthread_mutex_lock(&m_mutex);
    if (0 != rs) {
        return false;
    }
    return true;
#endif
}


bool Mutex::Unlock()
{
#if defined(PLATFORM_OS_WINDOWS)
    BOOL rs = ReleaseMutex(m_hmutex);
    if (TRUE != rs) {
        return false;
    }
    return true;
#else
    int rs = pthread_mutex_unlock(&m_mutex);
    if (0 != rs) {
        return false;
    }
    return true;
#endif
}

bool Mutex::TryLock()
{
#if defined(PLATFORM_OS_WINDOWS)
    DWORD rs = WaitForSingleObject(m_hmutex, 0);
    if (WAIT_OBJECT_0 != rs) {
        return false;
    }
    return true;
#else
    int lock_result = pthread_mutex_trylock(&m_mutex);
    if (0 != lock_result) {
        return false;
    }
    return true;
#endif
}

bool Mutex::TimedWait(unsigned s, unsigned ms)
{
#if defined(PLATFORM_OS_WINDOWS)
    DWORD dwMilliseconds = s * 1000 + ms;
    DWORD rs = WaitForSingleObject(m_hcond, dwMilliseconds);
    if (WAIT_OBJECT_0 != rs) {
        return false;
    }
    return true;
#else
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += s;
    abstime.tv_nsec += ((long)ms) * 1000 * 1000;
    int lock_result = pthread_cond_timedwait(&m_cond, &m_mutex, &abstime);
    if (0 != lock_result) {
        return false;
    }
    return true;
#endif
}

bool Mutex::CondSignal()
{
#if defined(PLATFORM_OS_WINDOWS)
    BOOL rs = SetEvent(m_hcond);
    if (TRUE != rs) {
        return false;
    }
    return true;
#else
    int rs = pthread_cond_signal(&m_cond);
    if (0 != rs) {
        return false;
    }
    return true;
#endif
}

