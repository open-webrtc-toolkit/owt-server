/* <COPYRIGHT_TAG> */

#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

 /**
  * Allows temporarily locking an object (e.g. a Mutex) and automatically
  * releasing the lock when the Locker goes out of scope.
  *
 */
template <typename T>
class Locker {
public:
    Locker(T& lock) : m_lock(lock)
    {
        m_lock.Lock();
    }

    ~Locker()
    {
        m_lock.Unlock();
    }

private:
    T& m_lock;

    //Prevent copying or assignment.
    Locker(const T&);
    void operator= (const T& );
};

#endif

