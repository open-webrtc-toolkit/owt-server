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

#ifndef EventRegistry_h
#define EventRegistry_h

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <Compiler.h>
#include <functional>
#include <queue>
#include <stdlib.h>
#include <string>
#include <uv.h>

namespace woogeen_base {

class DLL_PUBLIC EventRegistry {
public:
    DLL_PUBLIC EventRegistry();
    DLL_PUBLIC virtual ~EventRegistry();
    bool notify(const std::string& data);
    void process();
    DLL_PUBLIC virtual void process(const std::string& data) = 0;
private:
    uv_async_t* m_uvHandle;
    boost::mutex m_lock;
    std::queue<std::string> m_buffer;
    std::string m_data;
    static void closeCallback(uv_handle_t*);
    static void callback(uv_async_t*, int status); // libuv <stable>
    static void callback(uv_async_t*); // libuv <HEAD>
};

class Notification {
public:
    virtual ~Notification() { }
    virtual void setupNotification(std::function<void (const std::string&, const std::string&)> f) { m_fn = f; }
    void notify(const std::string& event, const std::string& data) { if (m_fn) m_fn(event, data); }
protected:
    std::function<void (const std::string&, const std::string&)> m_fn;
    Notification(std::function<void (const std::string&, const std::string&)> f = nullptr) : m_fn (f) { }
};

} /* namespace woogeen_base */

#endif // EventRegistry_h
