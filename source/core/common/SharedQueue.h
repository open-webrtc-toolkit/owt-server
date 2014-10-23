/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#ifndef SharedQueue_h
#define SharedQueue_h

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <queue>

template <typename T>
class SharedQueue {
public:
    SharedQueue();
    ~SharedQueue();

    bool push(const T&);

    // Pops object from queue.
    // If pop operation is successful, object will be copied to element.
    // Returns true if the pop operation is successful, false if the queue was empty
    // or the queue is being deleted.
    bool pop(T&);

    // blockingPop will wait until the queue is non-empty,
    // or the queue is being deleted.
    // If pop operation is successful, object will be copied to element.
    // Returns true if the pop operation is successful, false if the queue is being deleted.
    bool blockingPop(T&);

private:
    std::queue<T> m_queue;
    boost::mutex m_mutex;
    boost::condition_variable m_cond;
    bool m_ending;
};

template <typename T>
SharedQueue<T>::SharedQueue()
    : m_ending(false)
{
}

template <typename T>
SharedQueue<T>::~SharedQueue()
{
    m_ending = true;
    m_cond.notify_one();
    boost::unique_lock<boost::mutex> lock(m_mutex);
}

template <typename T>
inline bool SharedQueue<T>::push(const T& element)
{
    if (m_ending)
        return false;

    boost::unique_lock<boost::mutex> lock(m_mutex);
    m_queue.push(element);
    lock.unlock();
    m_cond.notify_one();
    return true;
}

template <typename T>
inline bool SharedQueue<T>::pop(T& element)
{
    boost::unique_lock<boost::mutex> lock(m_mutex);

    if (m_queue.empty() || m_ending)
        return false;

    element = m_queue.front();
    m_queue.pop();
    return true;
}

template <typename T>
inline bool SharedQueue<T>::blockingPop(T& element)
{
    boost::unique_lock<boost::mutex> lock(m_mutex);

    while (m_queue.empty() && !m_ending)
        m_cond.wait(lock);

    if (m_ending)
        return false;

    element = m_queue.front();
    m_queue.pop();
    return true;
}

#endif /* SharedQueue_h */
