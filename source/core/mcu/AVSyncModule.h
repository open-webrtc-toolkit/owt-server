/*
 * AVSyncModule.h
 *
 *  Created on: Sep 25, 2014
 *      Author: qzhang8
 */

#ifndef AVSYNCMODULE_H_
#define AVSYNCMODULE_H_

#include "stream_synchronization.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"


// AVSyncModule is responsible for synchronization audio and video for a given
// ACMInput and VCMInput channel couple.

namespace webrtc{
class CriticalSectionWrapper;
class RtpRtcp;
class RtpReceiver;
class VideoCodingModule;
}

using namespace webrtc;

namespace mcu {

class VCMInputProcessor;
class ACMInputProcessor;

class AVSyncModule: public webrtc::Module {
public:
	AVSyncModule(webrtc::VideoCodingModule* vcm,
            	VCMInputProcessor* vcmInput);
	virtual ~AVSyncModule();

	int ConfigureSync(ACMInputProcessor* acmInput,
			webrtc::RtpRtcp* video_rtcp_module,
			webrtc::RtpReceiver* video_receiver);


	// Set target delay for buffering mode (0 = real-time mode).
	int SetTargetBufferingDelay(int target_delay_ms);

	// Implements Module.
	virtual int32_t TimeUntilNextProcess();
	virtual int32_t Process();

private:
	scoped_ptr<CriticalSectionWrapper> data_cs_;
	VideoCodingModule* vcm_;
	VCMInputProcessor* vcmInput_;
	RtpReceiver* video_receiver_;
	RtpRtcp* video_rtp_rtcp_;
	ACMInputProcessor* acmInput_;
	TickTime last_sync_time_;
	scoped_ptr<StreamSynchronization> sync_;
	StreamSynchronization::Measurements audio_measurement_;
	StreamSynchronization::Measurements video_measurement_;
};

} /* namespace erizo */
#endif /* AVSYNCMODULE_H_ */
