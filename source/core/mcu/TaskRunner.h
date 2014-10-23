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

#ifndef TaskRunner_h
#define TaskRunner_h

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <list>
#include <webrtc/modules/interface/module.h>
#include <webrtc/system_wrappers/interface/scoped_ptr.h>
#include <webrtc/system_wrappers/interface/critical_section_wrapper.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

using namespace webrtc;
/**
 * This thread is now responsible for running the non critical process for each modules
 */
namespace mcu {

class TaskRunner {
public:
    TaskRunner(int interval);
    virtual ~TaskRunner();
    void start();
    void stop();
    void registerModule(Module*);
    void unregisterModule(Module*);
    void run(const boost::system::error_code& /*e*/);

private:
    int m_interval; // milliseconds
    std::list<Module*> m_taskList;
    scoped_ptr<CriticalSectionWrapper> m_taskLock;

    scoped_ptr<boost::thread> m_thread;
    boost::asio::io_service m_ioService;
    scoped_ptr<boost::asio::deadline_timer> m_timer;
};

} /* namespace mcu */
#endif /* TaskRunner_h */
