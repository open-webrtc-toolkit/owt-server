/*
 * AVSyncTaskRunner.h
 *
 *  Created on: Sep 29, 2014
 *      Author: qzhang8
 */

#ifndef AVSYNCTASKRUNNER_H_
#define AVSYNCTASKRUNNER_H_

#include <list>
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

namespace webrtc{
class Module;
}

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
	void registerModule(Module* module);
	void unregisterModule(Module* module);
	void run(const boost::system::error_code& /*e*/);

private:
	int  interval_; 	//milliseconds
	std::list<Module*> taskList_;
	scoped_ptr<CriticalSectionWrapper> taskLock_;

	scoped_ptr<boost::thread> thread_;
    boost::asio::io_service io_service_;
    scoped_ptr<boost::asio::deadline_timer> timer_;


};

} /* namespace erizo */
#endif /* AVSYNCTASKRUNNER_H_ */
