// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "StaticTaskQueueFactory.h"
#include <rtc_base/logging.h>
#include <rtc_base/checks.h>
#include <rtc_base/event.h>
#include <rtc_base/task_utils/to_queued_task.h>
#include <api/task_queue/task_queue_base.h>
#include <api/task_queue/default_task_queue_factory.h>

namespace rtc_adapter {

// TaskQueueDummy never execute tasks
class TaskQueueDummy final : public webrtc::TaskQueueBase {
public:
    TaskQueueDummy() {}
    ~TaskQueueDummy() override = default;

    // Implements webrtc::TaskQueueBase
    void Delete() override {}
    void PostTask(std::unique_ptr<webrtc::QueuedTask> task) override {}
    void PostDelayedTask(std::unique_ptr<webrtc::QueuedTask> task,
                         uint32_t milliseconds) override {}
};

// QueuedTaskProxy only execute when the owner shared_ptr exists
class QueuedTaskProxy : public webrtc::QueuedTask {
public:
    QueuedTaskProxy(std::unique_ptr<webrtc::QueuedTask> task, std::shared_ptr<int> owner)
        : m_task(std::move(task)), m_owner(owner) {}

    // Implements webrtc::QueuedTask
    bool Run() override
    {
        if (auto owner = m_owner.lock()) {
            // Only run when owner exists
            return m_task->Run();
        }
        return true;
    }
private:
    std::unique_ptr<webrtc::QueuedTask> m_task;
    std::weak_ptr<int> m_owner;
};

// TaskQueueProxy holds a TaskQueueBase* and proxy its method without Delete
class TaskQueueProxy : public webrtc::TaskQueueBase {
public:
    TaskQueueProxy(webrtc::TaskQueueBase* taskQueue)
        : m_taskQueue(taskQueue), m_sp(std::make_shared<int>(1))
    {
        RTC_CHECK(m_taskQueue);
    }
    ~TaskQueueProxy() override = default;

    // Implements webrtc::TaskQueueBase
    void Delete() override
    {
        // Clear the shared_ptr so related tasks won't be run
        rtc::Event done;
        m_taskQueue->PostTask(webrtc::ToQueuedTask([this, &done] {
            m_sp.reset();
            done.Set();
        }));
        done.Wait(rtc::Event::kForever);
    }
    // Implements webrtc::TaskQueueBase
    void PostTask(std::unique_ptr<webrtc::QueuedTask> task) override
    {
        m_taskQueue->PostTask(
            std::make_unique<QueuedTaskProxy>(std::move(task), m_sp));
    }
    // Implements webrtc::TaskQueueBase
    void PostDelayedTask(std::unique_ptr<webrtc::QueuedTask> task,
                         uint32_t milliseconds) override
    {
        m_taskQueue->PostDelayedTask(
            std::make_unique<QueuedTaskProxy>(std::move(task), m_sp), milliseconds);
    }
private:
    webrtc::TaskQueueBase* m_taskQueue;
    // Use shared_ptr to track its tasks
    std::shared_ptr<int> m_sp;
};

// Provide static TaskQueues
class StaticTaskQueueFactory final : public webrtc::TaskQueueFactory {
 public:
    // Implements webrtc::TaskQueueFactory
    std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter> CreateTaskQueue(
        absl::string_view name,
        webrtc::TaskQueueFactory::Priority priority) const override
    {
        // Use a pool if following threads take too heavy load
        static std::unique_ptr<webrtc::TaskQueueFactory> defaultTaskQueueFactory =
            webrtc::CreateDefaultTaskQueueFactory();
        static std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter> callTaskQueue =
            defaultTaskQueueFactory->CreateTaskQueue(
                "CallTaskQueue", webrtc::TaskQueueFactory::Priority::NORMAL);
        static std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter> decodingQueue =
            defaultTaskQueueFactory->CreateTaskQueue(
                "DecodingQueue", webrtc::TaskQueueFactory::Priority::HIGH);
        static std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter> rtpSendCtrlQueue =
            defaultTaskQueueFactory->CreateTaskQueue(
                "rtp_send_controller", webrtc::TaskQueueFactory::Priority::NORMAL);

        if (name == absl::string_view("CallTaskQueue")) {
            return std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter>(
                new TaskQueueProxy(callTaskQueue.get()));
        } else if (name == absl::string_view("DecodingQueue")) {
            return std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter>(
                new TaskQueueProxy(decodingQueue.get()));
        } else if (name == absl::string_view("rtp_send_controller")) {
            return std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter>(
                new TaskQueueProxy(rtpSendCtrlQueue.get()));
        } else {
            // Return dummy task queue for other names like "IncomingVideoStream"
            RTC_DLOG(LS_INFO) << "Dummy TaskQueue for " << name;
            return std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter>(
                new TaskQueueDummy());
        }
    }
};

std::unique_ptr<webrtc::TaskQueueFactory> createStaticTaskQueueFactory()
{
    return std::unique_ptr<webrtc::TaskQueueFactory>(new StaticTaskQueueFactory());
}

} // namespace rtc_adapter
