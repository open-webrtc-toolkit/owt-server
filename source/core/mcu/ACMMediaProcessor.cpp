/*
 * ACMMediaProcessor.cpp
 *
 *  Created on: Sep 12, 2014
 *      Author: qzhang8
 */

#include "ACMMediaProcessor.h"

#include <webrtc/voice_engine/include/voe_errors.h>

using namespace erizo;
using namespace webrtc;

namespace mcu {
DEFINE_LOGGER(ACMInputProcessor, "media.ACMInputProcessor");

ACMInputProcessor::ACMInputProcessor(int32_t channelId) : rtp_header_parser_(RtpHeaderParser::Create()),
	    rtp_payload_registry_(new RTPPayloadRegistry(channelId,
	                               RTPPayloadStrategy::CreateStrategy(true))),
	    rtp_receive_statistics_(ReceiveStatistics::Create(
	        Clock::GetRealTimeClock())),
	    rtp_receiver_(RtpReceiver::CreateAudioReceiver(
	        channelId, Clock::GetRealTimeClock(), NULL,
	        this, this, rtp_payload_registry_.get())),
	    audio_coding_(AudioCodingModule::Create(
	        channelId)),
	    _externalTransport(false),
	    jitter_buffer_playout_timestamp_(0),
	    playout_timestamp_rtp_(0),
	    playout_timestamp_rtcp_(0),
	    playout_delay_ms_(0),
	    _numberOfDiscardedPackets(0),
	    send_sequence_number_(0),
	    _engineStatisticsPtr(NULL),
	    _transportPtr(NULL),
	    rx_audioproc_(AudioProcessing::Create(channelId)),
	    _lastLocalTimeStamp(0),
	    _lastRemoteTimeStamp(0),
	    _lastPayloadType(0),
	    _includeAudioLevelIndication(false),
	    video_channel_(-1),
	    _average_jitter_buffer_delay_us(0),
	    least_required_delay_ms_(0),
	    _previousTimestamp(0),
	    _recPacketDelayMs(20),
	    _RxVadDetection(false),
	    _rxAgcIsEnabled(false),
	    _rxNsIsEnabled(false),
	    restored_packet_in_use_(false)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(channelId,channelId),
                 "ACMInputProcessor::ACMInputProcessor() - ctor");

    RtpRtcp::Configuration configuration;
    configuration.id = VoEModuleId(channelId, channelId);
    configuration.audio = true;
    configuration.rtcp_feedback = this;
    configuration.receive_statistics = rtp_receive_statistics_.get();

    _rtpRtcpModule.reset(RtpRtcp::CreateRtpRtcp(configuration));

    //rtp_receive_statistics is not enabled
    /*
    statistics_proxy_.reset(new StatisticsProxy(_rtpRtcpModule->SSRC()));
    rtp_receive_statistics_->RegisterRtcpStatisticsCallback(
        statistics_proxy_.get());

    */
}

ACMInputProcessor::~ACMInputProcessor() {
    // The order to safely shutdown modules in a channel is:
    // 1. De-register callbacks in modules
    // 2. De-register modules in process thread
    // 3. Destroy modules
    if (audio_coding_->RegisterTransportCallback(NULL) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                     VoEId(_channelId,_channelId),
                     "~ACMInputProcessor() failed to de-register transport callback"
                     " (Audio coding module)");
    }
    if (audio_coding_->RegisterVADCallback(NULL) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                     VoEId(_channelId,_channelId),
                     "~ACMInputProcessor() failed to de-register VAD callback"
                     " (Audio coding module)");
    }
    // End of modules shutdown


}

int32_t ACMInputProcessor::Init(ACMOutputProcessor* aop) {
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
                 "ACMInputProcessor::Init()");
    aop_ = aop;

    if ((audio_coding_->InitializeReceiver() == -1)/* ||
        (audio_coding_->InitializeSender() == -1)*/)
    {
    	   WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
            "Channel::Init() unable to initialize the ACM - 1");
        return -1;
    }

    // --- RTP/RTCP module initialization

    // Ensure that RTCP is enabled by default for the created channel.
    // Note that, the module will keep generating RTCP until it is explicitly
    // disabled by the user.
    // After StopListen (when no sockets exists), RTCP packets will no longer
    // be transmitted since the Transport object will then be invalid.
    // RTCP is enabled by default.
    if (_rtpRtcpModule->SetRTCPStatus(kRtcpCompound) == -1)
    {
    	   WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
            "Channel::Init() RTP/RTCP module not initialized");
        return -1;
    }

/*
     // --- Register all permanent callbacks
    const bool fail =
        (audio_coding_->RegisterTransportCallback(this) == -1) ||
        (audio_coding_->RegisterVADCallback(this) == -1);

    if (fail)
    {
    	   WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
            "Channel::Init() callbacks not registered");
        return -1;
    }
*/

    // --- Register all supported codecs to the receiving side of the
    // RTP/RTCP module

    CodecInst codec;
    const uint8_t nSupportedCodecs = AudioCodingModule::NumberOfCodecs();

    for (int idx = 0; idx < nSupportedCodecs; idx++)
    {
        // Open up the RTP/RTCP receiver for all supported codecs
        if ((audio_coding_->Codec(idx, &codec) == -1) ||
            (rtp_receiver_->RegisterReceivePayload(
                codec.plname,
                codec.pltype,
                codec.plfreq,
                codec.channels,
                (codec.rate < 0) ? 0 : codec.rate) == -1))
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(_channelId,_channelId),
                         "Channel::Init() unable to register %s (%d/%d/%d/%d) "
                         "to RTP/RTCP receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }
        else
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                         VoEId(_channelId,_channelId),
                         "Channel::Init() %s (%d/%d/%d/%d) has been added to "
                         "the RTP/RTCP receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }


        if (!STR_CASE_CMP(codec.plname, "CN"))
        {
            if ((audio_coding_->RegisterReceiveCodec(codec) == -1))
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                             VoEId(_channelId,_channelId),
                             "Channel::Init() failed to register CN (%d/%d) "
                             "correctly - 1",
                             codec.pltype, codec.plfreq);
            }
        }
#define WEBRTC_CODEC_RED
#ifdef WEBRTC_CODEC_RED
        // Register RED to the receiving side of the ACM.
        // We will not receive an OnInitializeDecoder() callback for RED.
        if (!STR_CASE_CMP(codec.plname, "RED"))
        {
            if (audio_coding_->RegisterReceiveCodec(codec) == -1)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                             VoEId(_channelId,_channelId),
                             "Channel::Init() failed to register RED (%d/%d) "
                             "correctly",
                             codec.pltype, codec.plfreq);
            }
        }
#endif
    }

    if (rx_audioproc_->noise_suppression()->set_level(kDefaultNsMode) != 0) {
      LOG_FERR1(LS_ERROR, noise_suppression()->set_level, kDefaultNsMode);
      return -1;
    }
    if (rx_audioproc_->gain_control()->set_mode(kDefaultRxAgcMode) != 0) {
      LOG_FERR1(LS_ERROR, gain_control()->set_mode, kDefaultRxAgcMode);
      return -1;
    }

    return 0;

}

void ACMInputProcessor::receiveRtpData(char* rtpdata, int len,
		erizo::DataType type, uint32_t streamId) {
	  WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_channelId,_channelId),
	               "Channel::ReceivedRTPPacket()");

	  // Store playout timestamp for the received RTP packet
	  UpdatePlayoutTimestamp(false);

	  const uint8_t* received_packet = reinterpret_cast<const uint8_t*>(rtpdata);
	  RTPHeader header;
	  if (!rtp_header_parser_->Parse(received_packet, len, &header)) {
	    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVoice, _channelId,
	                 "Incoming packet: invalid RTP header");
	    return ;
	  }
	  header.payload_type_frequency =
	      rtp_payload_registry_->GetPayloadTypeFrequency(header.payloadType);
	  if (header.payload_type_frequency < 0)
	    return ;
	  bool in_order = IsPacketInOrder(header);
	  rtp_receive_statistics_->IncomingPacket(header, len,
	      IsPacketRetransmitted(header, in_order));
	  rtp_payload_registry_->SetIncomingPayloadType(header);


	  ReceivePacket(received_packet, len, header, in_order);

}

bool ACMInputProcessor::ReceivePacket(const uint8_t* packet, int packet_length,
		const RTPHeader& header, bool in_order) {
	//should not be encapsulated at here
	/*
	  if (rtp_payload_registry_->IsEncapsulated(header)) {
	    return HandleEncapsulation(packet, packet_length, header);
	  }
	  */
	  const uint8_t* payload = packet + header.headerLength;
	  int payload_length = packet_length - header.headerLength;
	  assert(payload_length >= 0);
	  PayloadUnion payload_specific;
	  if (!rtp_payload_registry_->GetPayloadSpecifics(header.payloadType,
	                                                  &payload_specific)) {
	    return false;
	  }
	  return rtp_receiver_->IncomingRtpPacket(header, payload, payload_length,
	                                          payload_specific, in_order);

}

bool ACMInputProcessor::IsPacketInOrder(const RTPHeader& header) const {
  StreamStatistician* statistician =
      rtp_receive_statistics_->GetStatistician(header.ssrc);
  if (!statistician)
    return false;
  return statistician->IsPacketInOrder(header.sequenceNumber);
}


int32_t ACMInputProcessor::OnReceivedPayloadData(const uint8_t* payloadData,
		const uint16_t payloadSize, const WebRtcRTPHeader* rtpHeader) {

    _lastRemoteTimeStamp = rtpHeader->header.timestamp;

#if 0
    if (!channel_state_.Get().playing)
    {
        // Avoid inserting into NetEQ when we are not playing. Count the
        // packet as discarded.
        WEBRTC_TRACE(kTraceStream, kTraceVoice,
                     VoEId(_channelId, _channelId),
                     "received packet is discarded since playing is not"
                     " activated");
        _numberOfDiscardedPackets++;
        return 0;
    }
#endif
    // Push the incoming payload (parsed and ready for decoding) into the ACM
    if (audio_coding_->IncomingPacket(payloadData,
                                      payloadSize,
                                      *rtpHeader) != 0)
    {
        WEBRTC_TRACE(kTraceStream, kTraceVoice,
                     VoEId(_channelId, _channelId),
            "Channel::OnReceivedPayloadData() unable to push data to the ACM");
        return -1;
    }

    // Update the packet delay.
    UpdatePacketDelay(rtpHeader->header.timestamp,
                      rtpHeader->header.sequenceNumber);

#if 0
    uint16_t round_trip_time = 0;
    _rtpRtcpModule->RTT(rtp_receiver_->SSRC(), &round_trip_time,
                        NULL, NULL, NULL);

    std::vector<uint16_t> nack_list = audio_coding_->GetNackList(
        round_trip_time);
    if (!nack_list.empty()) {
      // Can't use nack_list.data() since it's not supported by all
      // compilers.
      ResendPackets(&(nack_list[0]), static_cast<int>(nack_list.size()));
    }
#endif
    return 0;

}

int32_t ACMInputProcessor::OnInitializeDecoder(const int32_t id,
		const int8_t payloadType, const char payloadName[RTP_PAYLOAD_NAME_SIZE],
		const int frequency, const uint8_t channels, const uint32_t rate) {

    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
                 "Channel::OnInitializeDecoder(id=%d, payloadType=%d, "
                 "payloadName=%s, frequency=%u, channels=%u, rate=%u)",
                 id, payloadType, payloadName, frequency, channels, rate);

   // assert(VoEChannelId(id) == _channelId);

    CodecInst receiveCodec = {0};
    CodecInst dummyCodec = {0};

    receiveCodec.pltype = payloadType;
    receiveCodec.plfreq = frequency;
    receiveCodec.channels = channels;
    receiveCodec.rate = rate;
    strncpy(receiveCodec.plname, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);

    audio_coding_->Codec(payloadName, &dummyCodec, frequency, channels);
    receiveCodec.pacsize = dummyCodec.pacsize;

    // Register the new codec to the ACM
    if (audio_coding_->RegisterReceiveCodec(receiveCodec) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                     VoEId(_channelId, _channelId),
                     "Channel::OnInitializeDecoder() invalid codec ("
                     "pt=%d, name=%s) received - 1", payloadType, payloadName);
       // _engineStatisticsPtr->SetLastError(VE_AUDIO_CODING_MODULE_ERROR);
        return -1;
    }

    return 0;

}

bool ACMInputProcessor::OnRecoveredPacket(const uint8_t* packet,
		int packet_length) {
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
    	"OnRecoveredPacket packet_length=%d", packet_length);
    return true;
}

void ACMInputProcessor::OnIncomingSSRCChanged(const int32_t id,
		const uint32_t ssrc) {

	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
    	"OnIncomingSSRCChanged id=%d, ssrc=%d", id, ssrc);

}

void ACMInputProcessor::OnIncomingCSRCChanged(const int32_t id,
		const uint32_t CSRC, const bool added) {
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
    	"OnIncomingCSRCChanged id=%d, CSRC=%d", id, CSRC);

}

void ACMInputProcessor::ResetStatistics(uint32_t ssrc) {
}

// Called for incoming RTP packets after successful RTP header parsing.
void ACMInputProcessor::UpdatePacketDelay(uint32_t rtp_timestamp,
                                uint16_t sequence_number) {
  WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_channelId,_channelId),
               "Channel::UpdatePacketDelay(timestamp=%lu, sequenceNumber=%u)",
               rtp_timestamp, sequence_number);

  // Get frequency of last received payload
  int rtp_receive_frequency = audio_coding_->ReceiveFrequency();

  CodecInst current_receive_codec;
  if (audio_coding_->ReceiveCodec(&current_receive_codec) != 0) {
    return;
  }

  // Update the least required delay.
  least_required_delay_ms_ = audio_coding_->LeastRequiredDelayMs();

  if (STR_CASE_CMP("G722", current_receive_codec.plname) == 0) {
    // Even though the actual sampling rate for G.722 audio is
    // 16,000 Hz, the RTP clock rate for the G722 payload format is
    // 8,000 Hz because that value was erroneously assigned in
    // RFC 1890 and must remain unchanged for backward compatibility.
    rtp_receive_frequency = 8000;
  } else if (STR_CASE_CMP("opus", current_receive_codec.plname) == 0) {
    // We are resampling Opus internally to 32,000 Hz until all our
    // DSP routines can operate at 48,000 Hz, but the RTP clock
    // rate for the Opus payload format is standardized to 48,000 Hz,
    // because that is the maximum supported decoding sampling rate.
    rtp_receive_frequency = 48000;
  }

  // |jitter_buffer_playout_timestamp_| updated in UpdatePlayoutTimestamp for
  // every incoming packet.
  uint32_t timestamp_diff_ms = (rtp_timestamp -
      jitter_buffer_playout_timestamp_) / (rtp_receive_frequency / 1000);
  if (!IsNewerTimestamp(rtp_timestamp, jitter_buffer_playout_timestamp_) ||
      timestamp_diff_ms > (2 * kVoiceEngineMaxMinPlayoutDelayMs)) {
    // If |jitter_buffer_playout_timestamp_| is newer than the incoming RTP
    // timestamp, the resulting difference is negative, but is set to zero.
    // This can happen when a network glitch causes a packet to arrive late,
    // and during long comfort noise periods with clock drift.
    timestamp_diff_ms = 0;
  }

  uint16_t packet_delay_ms = (rtp_timestamp - _previousTimestamp) /
      (rtp_receive_frequency / 1000);

  _previousTimestamp = rtp_timestamp;

  if (timestamp_diff_ms == 0) return;

  if (packet_delay_ms >= 10 && packet_delay_ms <= 60) {
    _recPacketDelayMs = packet_delay_ms;
  }

  if (_average_jitter_buffer_delay_us == 0) {
    _average_jitter_buffer_delay_us = timestamp_diff_ms * 1000;
    return;
  }

  // Filter average delay value using exponential filter (alpha is
  // 7/8). We derive 1000 *_average_jitter_buffer_delay_us here (reduces
  // risk of rounding error) and compensate for it in GetDelayEstimate()
  // later.
  _average_jitter_buffer_delay_us = (_average_jitter_buffer_delay_us * 7 +
      1000 * timestamp_diff_ms + 500) / 8;
}

bool ACMInputProcessor::GetDelayEstimate(int* jitter_buffer_delay_ms,
		int* playout_buffer_delay_ms) const {
  if (_average_jitter_buffer_delay_us == 0) {
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
                 "Channel::GetDelayEstimate() no valid estimate.");
    return false;
  }
  *jitter_buffer_delay_ms = (_average_jitter_buffer_delay_us + 500) / 1000 +
      _recPacketDelayMs;
  *playout_buffer_delay_ms = playout_delay_ms_;
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
               "Channel::GetDelayEstimate()");
  return true;
}

int ACMInputProcessor::SetInitialPlayoutDelay(int delay_ms) {
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
	               "Channel::SetInitialPlayoutDelay()");
	  if ((delay_ms < kVoiceEngineMinMinPlayoutDelayMs) ||
	      (delay_ms > kVoiceEngineMaxMinPlayoutDelayMs))
	  {
		WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
	        "SetInitialPlayoutDelay() invalid min delay (error %d)", VE_INVALID_ARGUMENT);
	    return -1;
	  }
	  if (audio_coding_->SetInitialPlayoutDelay(delay_ms) != 0)
	  {
		WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
	        "SetInitialPlayoutDelay() failed to set min playout delay (error %d)", VE_AUDIO_CODING_MODULE_ERROR);
	    return -1;
	  }
	  return 0;

}

int ACMInputProcessor::SetMinimumPlayoutDelay(int delayMs) {

	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
                 "Channel::SetMinimumPlayoutDelay()");
    if ((delayMs < kVoiceEngineMinMinPlayoutDelayMs) ||
        (delayMs > kVoiceEngineMaxMinPlayoutDelayMs))
    {
		WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
            "SetMinimumPlayoutDelay() invalid min delay (error %d)", VE_INVALID_ARGUMENT);
        return -1;
    }
    if (audio_coding_->SetMinimumPlayoutDelay(delayMs) != 0)
    {
		WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
            "SetMinimumPlayoutDelay() failed to set min playout delay (error %d)", VE_AUDIO_CODING_MODULE_ERROR);
        return -1;
    }
    return 0;

}

void ACMInputProcessor::UpdatePlayoutTimestamp(bool rtcp) {
  uint32_t playout_timestamp = 0;

  if (audio_coding_->PlayoutTimestamp(&playout_timestamp) == -1)  {
    WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_channelId,_channelId),
                 "Channel::UpdatePlayoutTimestamp() failed to read playout"
                 " timestamp from the ACM");
#if 0
    _engineStatisticsPtr->SetLastError(
        VE_CANNOT_RETRIEVE_VALUE, kTraceError,
        "UpdatePlayoutTimestamp() failed to retrieve timestamp");
#endif
    return;
  }

  uint16_t delay_ms = 0;
#if 0
  if (_audioDeviceModulePtr->PlayoutDelay(&delay_ms) == -1) {
    WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_channelId,_channelId),
                 "Channel::UpdatePlayoutTimestamp() failed to read playout"
                 " delay from the ADM");
    _engineStatisticsPtr->SetLastError(
        VE_CANNOT_RETRIEVE_VALUE, kTraceError,
        "UpdatePlayoutTimestamp() failed to retrieve playout delay");
    return;
  }
#endif

  int32_t playout_frequency = audio_coding_->PlayoutFrequency();
  CodecInst current_recive_codec;
  if (audio_coding_->ReceiveCodec(&current_recive_codec) == 0) {
    if (STR_CASE_CMP("G722", current_recive_codec.plname) == 0) {
      playout_frequency = 8000;
    } else if (STR_CASE_CMP("opus", current_recive_codec.plname) == 0) {
      playout_frequency = 48000;
    }
  }

  jitter_buffer_playout_timestamp_ = playout_timestamp;

  // Remove the playout delay.
  playout_timestamp -= (delay_ms * (playout_frequency / 1000));

  WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_channelId,_channelId),
               "Channel::UpdatePlayoutTimestamp() => playoutTimestamp = %lu",
               playout_timestamp);

  if (rtcp) {
    playout_timestamp_rtcp_ = playout_timestamp;
  } else {
    playout_timestamp_rtp_ = playout_timestamp;
  }
  playout_delay_ms_ = delay_ms;
}


int ACMInputProcessor::GetPlayoutTimestamp(unsigned int& timestamp) {
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
	               "Channel::GetPlayoutTimestamp()");
	  if (playout_timestamp_rtp_ == 0)  {
#if 0
	    _engineStatisticsPtr->SetLastError(
	        VE_CANNOT_RETRIEVE_VALUE, kTraceError,
	        "GetPlayoutTimestamp() failed to retrieve timestamp");
#endif
	    return -1;
	  }
	  timestamp = playout_timestamp_rtp_;
	  WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
	               VoEId(_channelId,_channelId),
	               "GetPlayoutTimestamp() => timestamp=%u", timestamp);
	  return 0;

}

int ACMInputProcessor::SetInitTimestamp(unsigned int timestamp) {
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
               "Channel::SetInitTimestamp()");
#if 0
    if (channel_state_.Get().sending)
    {
        _engineStatisticsPtr->SetLastError(
            VE_SENDING, kTraceError, "SetInitTimestamp() already sending");
        return -1;
    }
#endif
    if (_rtpRtcpModule->SetStartTimestamp(timestamp) != 0)
    {
#if 0
        _engineStatisticsPtr->SetLastError(
            VE_RTP_RTCP_MODULE_ERROR, kTraceError,
            "SetInitTimestamp() failed to set timestamp");
#endif
        return -1;
    }
    return 0;

}

int ACMInputProcessor::SetInitSequenceNumber(short sequenceNumber) {
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
                 "Channel::SetInitSequenceNumber()");
#if 0
    if (channel_state_.Get().sending)
    {
        _engineStatisticsPtr->SetLastError(
            VE_SENDING, kTraceError,
            "SetInitSequenceNumber() already sending");
        return -1;
    }
#endif
    if (_rtpRtcpModule->SetSequenceNumber(sequenceNumber) != 0)
    {
#if 0
        _engineStatisticsPtr->SetLastError(
            VE_RTP_RTCP_MODULE_ERROR, kTraceError,
            "SetInitSequenceNumber() failed to set sequence number");
#endif
        return -1;
    }
    return 0;

}

int ACMInputProcessor::GetRtpRtcp(RtpRtcp** rtpRtcpModule,
		RtpReceiver** rtp_receiver) const {
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
                 "Channel::GetRtpRtcp()");
    *rtpRtcpModule = _rtpRtcpModule.get();
    *rtp_receiver = rtp_receiver_.get();
    return 0;

}

bool ACMInputProcessor::IsPacketRetransmitted(const RTPHeader& header,
                                    bool in_order) const {
  // Retransmissions are handled separately if RTX is enabled.
  if (rtp_payload_registry_->RtxEnabled())
    return false;
  StreamStatistician* statistician =
      rtp_receive_statistics_->GetStatistician(header.ssrc);
  if (!statistician)
    return false;
  // Check if this is a retransmission.
  uint16_t min_rtt = 0;
  _rtpRtcpModule->RTT(rtp_receiver_->SSRC(), NULL, NULL, &min_rtt, NULL);
  return !in_order &&
      statistician->IsRetransmitOfOldPacket(header, min_rtt);
}

int32_t ACMInputProcessor::ReceivedRTCPPacket(const int8_t* data, int32_t length) {
  WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_channelId,_channelId),
               "Channel::ReceivedRTCPPacket()");
  // Store playout timestamp for the received RTCP packet
  UpdatePlayoutTimestamp(true);


  // Deliver RTCP packet to RTP/RTCP module for parsing
  if (_rtpRtcpModule->IncomingRtcpPacket((const uint8_t*)data,
                                         (uint16_t)length) == -1) {
  	   WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_channelId,_channelId),
        "Channel::IncomingRTPPacket() RTCP packet is invalid");
  }
  return 0;
}


int32_t ACMInputProcessor::GetAudioFrame(const int32_t id,
		AudioFrame& audioFrame) {
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_channelId,_channelId),
                 "Channel::GetAudioFrame(id=%d)", id);

    // Get 10ms raw PCM data from the ACM (mixer limits output frequency)
    if (audio_coding_->PlayoutData10Ms(audioFrame.sample_rate_hz_,
                                       &audioFrame) == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice,
                     VoEId(_channelId,_channelId),
                     "Channel::GetAudioFrame() PlayoutData10Ms() failed!");
        // In all likelihood, the audio in this frame is garbage. We return an
        // error so that the audio mixer module doesn't add it to the mix. As
        // a result, it won't be played out and the actions skipped here are
        // irrelevant.
        return -1;
    }

  //  FileRecorder* recorder= aop_->getOutputFileRecorder();
//    recorder->RecordAudioToFile(audioFrame, NULL);

    if (true /*state.rx_apm_is_enabled*/) {
      int err = rx_audioproc_->ProcessStream(&audioFrame);
      if (err) {
        LOG(LS_ERROR) << "ProcessStream() error: " << err;
        assert(false);
      }
    }

    return 0;

}

int32_t ACMInputProcessor::NeededFrequency(const int32_t id) {
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_channelId,_channelId),
                 "Channel::NeededFrequency(id=%d)", id);

    int highestNeeded = 0;

    // Determine highest needed receive frequency
    int32_t receiveFrequency = audio_coding_->ReceiveFrequency();

    // Return the bigger of playout and receive frequency in the ACM.
    if (audio_coding_->PlayoutFrequency() > receiveFrequency)
    {
        highestNeeded = audio_coding_->PlayoutFrequency();
    }
    else
    {
        highestNeeded = receiveFrequency;
    }

    return(highestNeeded);

}

DEFINE_LOGGER(ACMOutputProcessor, "media.ACMOutputProcessor");
ACMOutputProcessor::ACMOutputProcessor(uint32_t instanceId, webrtc::Transport* transport):
		amixer_(AudioConferenceMixer::Create(instanceId)),
		apm_(NULL),
		instanceId_(instanceId),
	    audio_coding_(AudioCodingModule::Create(
	    		instanceId)) {

    WEBRTC_TRACE(kTraceMemory, kTraceVoice, instanceId,
                 "OutputMixer::OutputMixer() - ctor");

    if ((amixer_->RegisterMixedStreamCallback(*this) == -1) ||
        (amixer_->RegisterMixerStatusCallback(*this, 100) == -1))
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId,-1),
                     "OutputMixer::OutputMixer() failed to register mixer"
                     "callbacks");
    }

    RtpRtcp::Configuration configuration;
    configuration.outgoing_transport = transport;
    configuration.id = VoEModuleId(instanceId_, instanceId_);
    configuration.audio = true;
/*
    configuration.rtcp_feedback = this;
    configuration.receive_statistics = rtp_receive_statistics_.get();
*/
    _rtpRtcpModule.reset(RtpRtcp::CreateRtpRtcp(configuration));

    init();
}

ACMOutputProcessor::~ACMOutputProcessor() {
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, instanceId_,
                 "OutputMixer::~OutputMixer() - dtor");
    this->StopRecordingPlayout();
    amixer_->UnRegisterMixerStatusCallback();
    amixer_->UnRegisterMixedStreamCallback();
}

bool ACMOutputProcessor::init() {

    if ((audio_coding_->InitializeSender() == -1))
    {
    	   WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(instanceId_,instanceId_),
            "Channel::Init() unable to initialize the ACM - 1");
        return false;
    }

	audio_coding_->RegisterTransportCallback(this);

    CodecInst codec;
    const uint8_t nSupportedCodecs = AudioCodingModule::NumberOfCodecs();

    for (int idx = 0; idx < nSupportedCodecs; idx++)
    {
        // Open up the RTP/RTCP receiver for all supported codecs
        if ((audio_coding_->Codec(idx, &codec) == -1))
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(instanceId_,instanceId_),
                         "Channel::Init() unable to register %s (%d/%d/%d/%d) "
                         "to RTP/RTCP receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }
        else
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                         VoEId(instanceId_,instanceId_),
                         "Channel::Init() %s (%d/%d/%d/%d) has been added to "
                         "the RTP/RTCP receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }

        // Ensure that PCMU is used as default codec on the sending side
        if (!STR_CASE_CMP(codec.plname, "PCMU") && (codec.channels == 1))
        {
            SetSendCodec(codec);
        }

        if (!STR_CASE_CMP(codec.plname, "CN"))
        {
            if ((audio_coding_->RegisterSendCodec(codec) == -1) ||
                /*(audio_coding_->RegisterReceiveCodec(codec) == -1) ||*/
                (_rtpRtcpModule->RegisterSendPayload(codec) == -1))
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                             VoEId(instanceId_,instanceId_),
                             "Channel::Init() failed to register CN (%d/%d) "
                             "correctly - 1",
                             codec.pltype, codec.plfreq);
            }
        }
    }


    this->StartRecordingPlayout("mixer.audio", NULL);
    timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::milliseconds(10)));
    timer_->async_wait(boost::bind(&ACMOutputProcessor::MixActiveChannels, this, boost::asio::placeholders::error));

    audioMixingThread_.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)));
    return true;
}

int32_t
ACMOutputProcessor::SetSendCodec(const CodecInst& codec)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(instanceId_,instanceId_),
                 "Channel::SetSendCodec()");

    if (audio_coding_->RegisterSendCodec(codec) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId_,instanceId_),
                     "SetSendCodec() failed to register codec to ACM");
        return -1;
    }

    if (_rtpRtcpModule->RegisterSendPayload(codec) != 0)
    {
        _rtpRtcpModule->DeRegisterSendPayload(codec.pltype);
        if (_rtpRtcpModule->RegisterSendPayload(codec) != 0)
        {
            WEBRTC_TRACE(
                    kTraceError, kTraceVoice, VoEId(instanceId_,instanceId_),
                    "SetSendCodec() failed to register codec to"
                    " RTP/RTCP module");
            return -1;
        }
    }

    if (_rtpRtcpModule->SetAudioPacketSize(codec.pacsize) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId_,instanceId_),
                     "SetSendCodec() failed to set audio packet size");
        return -1;
    }

    return 0;
}

int32_t ACMOutputProcessor::SendData(FrameType frame_type, uint8_t payload_type,
		uint32_t timestamp, const uint8_t* payload_data,
		uint16_t payload_len_bytes,
		const RTPFragmentationHeader* fragmentation) {
    // Push data from ACM to RTP/RTCP-module to deliver audio frame for
    // packetization.
    // This call will trigger Transport::SendPacket() from the RTP/RTCP module.
    if (_rtpRtcpModule->SendOutgoingData((FrameType&)frame_type,
    									payload_type,
                                        timestamp,
                                        // Leaving the time when this frame was
                                        // received from the capture device as
                                        // undefined for voice for now.
                                        -1,
                                        payload_data,
                                        payload_len_bytes,
                                        fragmentation) == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId_,instanceId_),
        		"Channel::SendData() failed to send data to RTP/RTCP module");
        return -1;
    }

    return 0;
}

void ACMOutputProcessor::NewMixedAudio(const int32_t id,
		const AudioFrame& generalAudioFrame,
		const AudioFrame** uniqueAudioFrames, const uint32_t size) {
		_outputFileRecorderPtr->RecordAudioToFile(generalAudioFrame, NULL);
		audio_coding_->Add10MsData(generalAudioFrame);
		audio_coding_->Process();
}

int32_t ACMOutputProcessor::MixActiveChannels(const boost::system::error_code& ec) {
    if (!ec) {
    	amixer_->Process();
    	timer_->expires_at(timer_->expires_at() + boost::posix_time::milliseconds(10));
    	timer_->async_wait(boost::bind(&ACMOutputProcessor::MixActiveChannels, this, boost::asio::placeholders::error));
    } else {
        ELOG_INFO("ACMOutputProcessor timer error: %s", ec.message().c_str());
    }
    return 0;
}

void ACMOutputProcessor::PlayNotification(int32_t id, uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::PlayNotification(id=%d, durationMs=%d)",
                 id, durationMs);
    // Not implement yet
}

void ACMOutputProcessor::RecordNotification(int32_t id,
                                     uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::RecordNotification(id=%d, durationMs=%d)",
                 id, durationMs);

    // Not implement yet
}

void ACMOutputProcessor::PlayFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::PlayFileEnded(id=%d)", id);

    // not needed
}

void ACMOutputProcessor::RecordFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::RecordFileEnded(id=%d)", id);
    assert(id == instanceId_);

    _outputFileRecording = false;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::RecordFileEnded() =>"
                 "output file recorder module is shutdown");
}

int ACMOutputProcessor::StartRecordingPlayout(const char* fileName,
                                       const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::StartRecordingPlayout(fileName=%s)", fileName);

    if (_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
                     "StartRecordingPlayout() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0);
    CodecInst dummyCodec={100,"L16",16000,320,1,320000};

    if ((codecInst != NULL) &&
      ((codecInst->channels < 1) || (codecInst->channels > 2)))
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
        		"StartRecordingPlayout() invalid compression");
        return(-1);
    }
    if(codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst=&dummyCodec;
    }
    else if((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    }
    else
    {
        format = kFileFormatCompressedFile;
    }


    // Destroy the old instance
    if (_outputFileRecorderPtr)
    {
        _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
    }

    _outputFileRecorderPtr = FileRecorder::CreateFileRecorder(
        instanceId_,
        (const FileFormats)format);
    if (_outputFileRecorderPtr == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
        		"StartRecordingPlayout() fileRecorder format isnot correct");
        return -1;
    }

    if (_outputFileRecorderPtr->StartRecordingAudioFile(
        fileName,
        (const CodecInst&)*codecInst,
        notificationTime) != 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
        		"StartRecordingAudioFile() failed to start file recording");
        _outputFileRecorderPtr->StopRecording();
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
        return -1;
    }
    _outputFileRecorderPtr->RegisterModuleFileCallback(this);
    _outputFileRecording = true;

    return 0;
}


int ACMOutputProcessor::StopRecordingPlayout()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(instanceId_,-1),
                 "OutputMixer::StopRecordingPlayout()");

    if (!_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(instanceId_,-1),
                     "StopRecordingPlayout() file isnot recording");
        return -1;
    }


    if (_outputFileRecorderPtr->StopRecording() != 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(instanceId_,-1),
        		"StopRecording(), could not stop recording");
        return -1;
    }
    _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
    FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
    _outputFileRecorderPtr = NULL;
    _outputFileRecording = false;

    return 0;
}


} /* namespace mcu */
