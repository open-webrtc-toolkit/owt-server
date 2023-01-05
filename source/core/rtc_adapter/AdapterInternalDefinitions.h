// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_ADAPTER_INTERNAL_DEFINITIONS_H_
#define RTC_ADAPTER_ADAPTER_INTERNAL_DEFINITIONS_H_

#include <api/task_queue/task_queue_factory.h>
#include <api/transport/webrtc_key_value_config.h>
#include <call/call.h>
#include <call/rtp_transport_controller_send_interface.h>
#include <rtc_base/task_queue.h>

namespace rtc_adapter {

class CallOwner {
public:
    virtual std::shared_ptr<webrtc::Call> call() = 0;
    virtual std::shared_ptr<webrtc::TaskQueueFactory> taskQueueFactory() = 0;
    virtual std::shared_ptr<rtc::TaskQueue> taskQueue() = 0;
    virtual std::shared_ptr<webrtc::RtcEventLog> eventLog() = 0;
    virtual webrtc::WebRtcKeyValueConfig* trial() = 0;
    virtual std::shared_ptr<webrtc::RtpTransportControllerSendInterface>
        rtpTransportController() = 0;
    virtual uint32_t estimatedBandwidth(uint32_t ssrc) = 0;
    virtual void registerVideoSender(uint32_t ssrc) = 0;
    virtual void deregisterVideoSender(uint32_t ssrc) = 0;
};

} // namespace rtc_adaptor

#endif