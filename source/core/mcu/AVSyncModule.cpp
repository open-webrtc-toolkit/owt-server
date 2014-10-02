/*
 * AVSyncModule.cpp
 *
 *  Created on: Sep 25, 2014
 *      Author: qzhang8
 */

#include "AVSyncModule.h"
#include "ACMMediaProcessor.h"
#include <webrtc/modules/video_coding/main/interface/video_coding.h>
#include "webrtc/system_wrappers/interface/trace.h"


namespace mcu {

int UpdateMeasurements(StreamSynchronization::Measurements* stream,
                       const RtpRtcp& rtp_rtcp, const RtpReceiver& receiver) {
  if (!receiver.Timestamp(&stream->latest_timestamp))
    return -1;
  if (!receiver.LastReceivedTimeMs(&stream->latest_receive_time_ms))
    return -1;
  synchronization::RtcpMeasurement measurement;
  if (0 != rtp_rtcp.RemoteNTP(&measurement.ntp_secs,
                              &measurement.ntp_frac,
                              NULL,
                              NULL,
                              &measurement.rtp_timestamp)) {
    return -1;
  }
  if (measurement.ntp_secs == 0 && measurement.ntp_frac == 0) {
    return -1;
  }
  for (synchronization::RtcpList::iterator it = stream->rtcp.begin();
       it != stream->rtcp.end(); ++it) {
    if (measurement.ntp_secs == (*it).ntp_secs &&
        measurement.ntp_frac == (*it).ntp_frac) {
      // This RTCP has already been added to the list.
      return 0;
    }
  }
  // We need two RTCP SR reports to map between RTP and NTP. More than two will
  // not improve the mapping.
  if (stream->rtcp.size() == 2) {
    stream->rtcp.pop_back();
  }
  stream->rtcp.push_front(measurement);
  return 0;
}

AVSyncModule::AVSyncModule(webrtc::VideoCodingModule* vcm,
		int videoChannelId) {
	vcm_ = vcm;
	videoChannelId_ = videoChannelId;
	video_receiver_ = NULL;
	video_rtp_rtcp_ = NULL;
}

AVSyncModule::~AVSyncModule() {
	// TODO Auto-generated destructor stub
}

int AVSyncModule::ConfigureSync(boost::shared_ptr<ACMInputProcessor> acmInput,
		webrtc::RtpRtcp* video_rtcp_module,
		webrtc::RtpReceiver* video_receiver) {
	acmInput_ = acmInput;
	video_rtp_rtcp_ = video_rtcp_module;
	video_receiver_ = video_receiver;
	sync_.reset(new StreamSynchronization(acmInput_->channelId(), videoChannelId_));
	return 0;
}

int AVSyncModule::SetTargetBufferingDelay(int target_delay_ms) {

	if (!acmInput_) {
	    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, videoChannelId_,
	                 "voe_sync_interface_ NULL, can't set playout delay.");
	    return -1;
	  }
	  sync_->SetTargetBufferingDelay(target_delay_ms);
	  // Setting initial playout delay to voice engine (video engine is updated via
	  // the VCM interface).
	  acmInput_->SetInitialPlayoutDelay(target_delay_ms);
	  return 0;

}

enum { kSyncInterval = 1000};

int32_t AVSyncModule::TimeUntilNextProcess() {
	  return static_cast<int32_t>(kSyncInterval -
	      (TickTime::Now() - last_sync_time_).Milliseconds());
}

int32_t AVSyncModule::Process() {
	  last_sync_time_ = TickTime::Now();

	  const int current_video_delay_ms = vcm_->Delay();
	  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, videoChannelId_,
	               "Video delay (JB + decoder) is %d ms", current_video_delay_ms);

	  if (acmInput_->channelId() == -1) {
	    return 0;
	  }
	  assert(video_rtp_rtcp_ && acmInput_);
	  assert(sync_.get());

	  int audio_jitter_buffer_delay_ms = 0;
	  int playout_buffer_delay_ms = 0;
	  if (acmInput_->GetDelayEstimate(&audio_jitter_buffer_delay_ms,
	                                            &playout_buffer_delay_ms) != 0) {
	    // Could not get VoE delay value, probably not a valid channel Id or
	    // the channel have not received enough packets.
	    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceVideo, videoChannelId_,
	                 "%s: VE_GetDelayEstimate error for voice_channel %d",
	                 __FUNCTION__, acmInput_->channelId());
	    return 0;
	  }
	  const int current_audio_delay_ms = audio_jitter_buffer_delay_ms +
			  	  	  	  	  	  	  	  	  playout_buffer_delay_ms;

	  RtpRtcp* voice_rtp_rtcp = NULL;
	  RtpReceiver* voice_receiver = NULL;
	  if (0 != acmInput_->GetRtpRtcp(&voice_rtp_rtcp, &voice_receiver)) {
	    return 0;
	  }
	  assert(voice_rtp_rtcp);
	  assert(voice_receiver);

	  if (UpdateMeasurements(&video_measurement_, *video_rtp_rtcp_,
	                         *video_receiver_) != 0) {
	    return 0;
	  }

	  if (UpdateMeasurements(&audio_measurement_, *voice_rtp_rtcp,
	                         *voice_receiver) != 0) {
	    return 0;
	  }

	  int relative_delay_ms;
	  // Calculate how much later or earlier the audio stream is compared to video.
	  if (!sync_->ComputeRelativeDelay(audio_measurement_, video_measurement_,
	                                   &relative_delay_ms)) {
	    return 0;
	  }

	  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideo, videoChannelId_,
			  "SyncCurrentVideoDelay", current_video_delay_ms);
	  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideo, videoChannelId_,
			  "SyncCurrentAudioDelay", current_audio_delay_ms);
	  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideo, videoChannelId_,
			  "SyncRelativeDelay", relative_delay_ms);
	  int target_audio_delay_ms = 0;
	  int target_video_delay_ms = current_video_delay_ms;
	  // Calculate the necessary extra audio delay and desired total video
	  // delay to get the streams in sync.
	  if (!sync_->ComputeDelays(relative_delay_ms,
	                            current_audio_delay_ms,
	                            &target_audio_delay_ms,
	                            &target_video_delay_ms)) {
	    return 0;
	  }

	  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideo, videoChannelId_,
	               "Set delay current(a=%d v=%d rel=%d) target(a=%d v=%d)",
	               current_audio_delay_ms, current_video_delay_ms,
	               relative_delay_ms,
	               target_audio_delay_ms, target_video_delay_ms);
	  if (acmInput_->SetMinimumPlayoutDelay(target_audio_delay_ms) == -1) {
	    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideo, videoChannelId_,
	                 "Error setting voice delay");
	  }
	  vcm_->SetMinimumPlayoutDelay(target_video_delay_ms);
	  return 0;
}

} /* namespace erizo */
