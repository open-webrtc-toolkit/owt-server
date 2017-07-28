/*
 * Copyright 2017 Intel Corporation All Rights Reserved. 
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

#ifndef WebRTCTaskRunner_h
#define WebRTCTaskRunner_h

#include <webrtc/base/location.h>
#include <webrtc/modules/utility/include/process_thread.h>

namespace woogeen_base {

/**
 * This thread is now responsible for running the non critical process for each modules.
 * It's now simply a wrapper of the webrtc ProcessThread, and we need it because it's shared
 * by different objects and we want to manage its lifetime automatically.
 */
class WebRTCTaskRunner {
public:
    WebRTCTaskRunner(const char* task_name);
    ~WebRTCTaskRunner();

    void Start();
    void Stop();
    void RegisterModule(webrtc::Module*);
    void DeRegisterModule(webrtc::Module*);

    webrtc::ProcessThread* unwrap();

private:
    std::unique_ptr<webrtc::ProcessThread> m_processThread;
};

inline WebRTCTaskRunner::WebRTCTaskRunner(const char* task_name)
    : m_processThread(webrtc::ProcessThread::Create(task_name))
{
}

inline WebRTCTaskRunner::~WebRTCTaskRunner()
{
}

inline void WebRTCTaskRunner::Start()
{
    m_processThread->Start();
}

inline void WebRTCTaskRunner::Stop()
{
    m_processThread->Stop();
}

inline void WebRTCTaskRunner::RegisterModule(webrtc::Module* module)
{
    m_processThread->RegisterModule(module, RTC_FROM_HERE);
}

inline void WebRTCTaskRunner::DeRegisterModule(webrtc::Module* module)
{
    m_processThread->DeRegisterModule(module);
}

inline webrtc::ProcessThread* WebRTCTaskRunner::unwrap()
{
    return m_processThread.get();
}

} /* namespace woogeen_base */

#endif /* WebRTCTaskRunner_h */
