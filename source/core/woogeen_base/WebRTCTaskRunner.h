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

#ifndef WebRTCTaskRunner_h
#define WebRTCTaskRunner_h

#include <webrtc/modules/utility/interface/process_thread.h>

namespace woogeen_base {

/**
 * This thread is now responsible for running the non critical process for each modules.
 * It's now simply a wrapper of the webrtc ProcessThread, and we need it because it's shared
 * by different objects and we want to manage its lifetime automatically.
 */
class WebRTCTaskRunner {
public:
    WebRTCTaskRunner();
    ~WebRTCTaskRunner();

    int32_t Start();
    int32_t Stop();
    int32_t RegisterModule(webrtc::Module*);
    int32_t DeRegisterModule(webrtc::Module*);

    webrtc::ProcessThread* unwrap();

private:
    rtc::scoped_ptr<webrtc::ProcessThread> m_processThread;
};

inline WebRTCTaskRunner::WebRTCTaskRunner()
    : m_processThread(webrtc::ProcessThread::Create())
{
}

inline WebRTCTaskRunner::~WebRTCTaskRunner()
{
}

inline int32_t WebRTCTaskRunner::Start()
{
    return m_processThread->Start();
}

inline int32_t WebRTCTaskRunner::Stop()
{
    return m_processThread->Stop();
}

inline int32_t WebRTCTaskRunner::RegisterModule(webrtc::Module* module)
{
    return m_processThread->RegisterModule(module);
}

inline int32_t WebRTCTaskRunner::DeRegisterModule(webrtc::Module* module)
{
    return m_processThread->DeRegisterModule(module);
}

inline webrtc::ProcessThread* WebRTCTaskRunner::unwrap()
{
    return m_processThread.get();
}

} /* namespace woogeen_base */

#endif /* WebRTCTaskRunner_h */
