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

#include "EventRegistry.h"

namespace woogeen_base {

EventRegistry::EventRegistry()
{
    m_uvHandle = reinterpret_cast<uv_async_t*>(malloc(sizeof(uv_async_t)));
    m_uvHandle->data = this;
    m_uvHandle->close_cb = closeCallback;
    uv_async_init(uv_default_loop(), m_uvHandle, callback);
}

EventRegistry::~EventRegistry()
{
    uv_close((uv_handle_t*)m_uvHandle, closeCallback);
}

// main thread
void EventRegistry::process()
{
    while (!m_buffer.empty()) {
        {
            boost::lock_guard<boost::mutex> lock(m_lock);
            m_data = m_buffer.front();
            m_buffer.pop();
        }
        process(m_data);
    }
    // fprintf(stderr, "Queue length: %lu\n", m_buffer.size());
}

// other thread
bool EventRegistry::notify(const std::string& data)
{
    if (uv_is_active(reinterpret_cast<uv_handle_t*>(m_uvHandle))) {
        {
            boost::lock_guard<boost::mutex> lock(m_lock);
            m_buffer.push(data);
        }
        uv_async_send(m_uvHandle);
        return true;
    }
    return false;
}

void EventRegistry::closeCallback(uv_handle_t* handle)
{
    free(handle);
}

void EventRegistry::callback(uv_async_t* handle, int status)
{
    EventRegistry* registry = reinterpret_cast<EventRegistry*>(handle->data);
    registry->process();
}

void EventRegistry::callback(uv_async_t* handle)
{
    EventRegistry* registry = reinterpret_cast<EventRegistry*>(handle->data);
    registry->process();
}

} /* namespace woogeen_base */
