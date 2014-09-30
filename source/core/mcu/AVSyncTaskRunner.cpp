/*
 * AVSyncTaskRunner.cpp
 *
 *  Created on: Sep 29, 2014
 *      Author: qzhang8
 */

#include "AVSyncTaskRunner.h"
#include "webrtc/modules/interface/module.h"

#include <algorithm>
#include <boost/bind.hpp>





namespace mcu {

AVSyncTaskRunner::AVSyncTaskRunner(int interval) :
						interval_(interval),
						taskLock_(CriticalSectionWrapper::CreateCriticalSection()){
}

AVSyncTaskRunner::~AVSyncTaskRunner() {
	// TODO Auto-generated destructor stub
}

void AVSyncTaskRunner::start() {
	timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::milliseconds(interval_)));
    timer_->async_wait(boost::bind(&AVSyncTaskRunner::run, this, boost::asio::placeholders::error));
    thread_.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)));
}

void AVSyncTaskRunner::run(const boost::system::error_code& ec) {
	if (!ec) {
		{
			CriticalSectionScoped cs(taskLock_.get());
			for(std::list<Module*>::iterator taskIter = taskList_.begin();
					taskIter != taskList_.end();
					taskIter++) {
				(*taskIter)->Process();
			}
		}
    	timer_->expires_at(timer_->expires_at() + boost::posix_time::milliseconds(interval_));
    	timer_->async_wait(boost::bind(&AVSyncTaskRunner::run, this, boost::asio::placeholders::error));

	} else {

	}
}

void AVSyncTaskRunner::stop() {

	if(timer_) timer_->cancel();
	io_service_.stop();

}

void AVSyncTaskRunner::registerModule(Module* module) {
	CriticalSectionScoped cs(taskLock_.get());
	taskList_.push_back(module);
}

void AVSyncTaskRunner::unregisterModule(Module* module) {
	CriticalSectionScoped cs(taskLock_.get());
	taskList_.remove(module);
}

} /* namespace erizo */
