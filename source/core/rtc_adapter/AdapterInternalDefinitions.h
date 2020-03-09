// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_ADAPTER_INTERNAL_DEFINITIONS_H_
#define RTC_ADAPTER_ADAPTER_INTERNAL_DEFINITIONS_H_

#include <rtc_base/task_queue.h>
#include <call/call.h>
#include <api/task_queue/task_queue_factory.h>

namespace rtc_adapter {

class CallOwner {
public:
    virtual std::shared_ptr<webrtc::Call> call() = 0;
    virtual std::shared_ptr<webrtc::TaskQueueFactory> taskQueueFactory() = 0;
    virtual std::shared_ptr<rtc::TaskQueue> taskQueue() = 0;
    virtual std::shared_ptr<webrtc::RtcEventLog> eventLog() = 0;
};

} // namespace rtc_adaptor

#endif