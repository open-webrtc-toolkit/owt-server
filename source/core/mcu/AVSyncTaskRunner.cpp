/*
 * AVSyncTaskRunner.cpp
 *
 *  Created on: Sep 29, 2014
 *      Author: qzhang8
 */

#include "AVSyncTaskRunner.h"
#include "webrtc/modules/interface/module.h"
#include <webrtc/system_wrappers/interface/trace.h>

#include <algorithm>
#include <boost/bind.hpp>





namespace mcu {

TaskRunner::TaskRunner(int interval) :
						interval_(interval),
						taskLock_(CriticalSectionWrapper::CreateCriticalSection()){
}

TaskRunner::~TaskRunner() {
	// TODO Auto-generated destructor stub
}

void TaskRunner::start() {
	timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::milliseconds(interval_)));
    timer_->async_wait(boost::bind(&TaskRunner::run, this, boost::asio::placeholders::error));
    thread_.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)));
}

void TaskRunner::run(const boost::system::error_code& ec) {
	if (!ec) {
		{
			CriticalSectionScoped cs(taskLock_.get());
			for(std::list<Module*>::iterator taskIter = taskList_.begin();
					taskIter != taskList_.end();
					taskIter++) {
				if((*taskIter)->TimeUntilNextProcess() <= 0)
					(*taskIter)->Process();
			}
		}
    	timer_->expires_at(timer_->expires_at() + boost::posix_time::milliseconds(interval_));
    	timer_->async_wait(boost::bind(&TaskRunner::run, this, boost::asio::placeholders::error));

	} else {

	}
}

void TaskRunner::stop() {

	if(timer_) timer_->cancel();
	io_service_.stop();

}

void TaskRunner::registerModule(Module* module) {
	CriticalSectionScoped cs(taskLock_.get());
	interval_ = MIN(interval_, module->TimeUntilNextProcess());
    WEBRTC_TRACE(kTraceStateInfo, kTraceUtility,
                 -1,
    			"registering module, interval changed to %d", interval_);

	taskList_.push_back(module);
}

void TaskRunner::unregisterModule(Module* module) {
	CriticalSectionScoped cs(taskLock_.get());
	taskList_.remove(module);
}

} /* namespace erizo */
