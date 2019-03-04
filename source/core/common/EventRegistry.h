// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef EventRegistry_h
#define EventRegistry_h

#include <string>

class EventRegistry {
public:
    virtual ~EventRegistry() {}
    // `notifyAsyncEvent()' sends a notification to the event handler, which would be handled with FIFO rule.
    virtual bool notifyAsyncEvent(const std::string& event, const std::string& data) = 0;
    // In contrast to `notifyAsyncEvent()', `notifyAsyncEventInEmergency()' sends a notification to the event handler,
    // which would be handled before other normal notifications (LIFO).
    // Do not abuse it.
    virtual bool notifyAsyncEventInEmergency(const std::string& event, const std::string& data) = 0;
};

#endif // EventRegistry_h
