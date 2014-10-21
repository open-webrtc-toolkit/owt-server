/*
 * ACMMediaProcessor.h
 *
 *  Created on: Sep 12, 2014
 *      Author: qzhang8
 */

#ifndef ACMMEDIAPROCESSOR_H_
#define ACMMEDIAPROCESSOR_H_
#include "MediaDefinitions.h"
#include "logger.h"
#include "WoogeenTransport.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/voice_engine/voice_engine_defines.h"	//for some const definitions
#include "webrtc/video_engine/overuse_frame_detector.h"	//for some const definitions
#include "webrtc/voice_engine/include/voe_errors.h"    //for some const definitions
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_conference_mixer/interface/audio_conference_mixer.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h"
#include "webrtc/modules/rtp_rtcp/interface/fec_receiver.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_receiver.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_payload_registry.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/modules/utility/interface/file_recorder.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


using namespace webrtc;

namespace mcu {

class ACMOutputProcessor;
class TaskRunner;

class ACMInputProcessor : public erizo::RTPDataReceiver,
						 public RtpData,
						 public RtpFeedback,
						 public RtcpFeedback,
						/*public ACMVADCallback, // receive voice activity from the ACM*/
						public MixerParticipant // supplies output mixer with audio frames
						{
	DECLARE_LOGGER();
public:
	ACMInputProcessor(int32_t channelId);
	virtual ~ACMInputProcessor();
    int32_t channelId() { return _channelId;}
    int32_t init(boost::shared_ptr<ACMOutputProcessor> aop, boost::shared_ptr<TaskRunner>);
	virtual void receiveRtpData(char* rtpdata, int len, erizo::DataType type = erizo::VIDEO, uint32_t streamId = 0);
    virtual int32_t GetAudioFrame(const int32_t id, AudioFrame& audioFrame);
    // This function specifies the sampling frequency needed for the AudioFrame
    // for future GetAudioFrame(..) calls.
    virtual int32_t NeededFrequency(const int32_t id);
    int32_t SetSendCodec(const CodecInst& codec);
    int32_t ReceivedRTCPPacket(const int8_t* data, int32_t length);

    //implements RtpData
    virtual int32_t OnReceivedPayloadData(
        const uint8_t* payloadData,
        const uint16_t payloadSize,
        const WebRtcRTPHeader* rtpHeader) ;

    virtual bool OnRecoveredPacket(const uint8_t* packet,
                                   int packet_length);


    /*
    *   channels    - number of channels in codec (1 = mono, 2 = stereo)
    */
    virtual int32_t OnInitializeDecoder(
        const int32_t id,
        const int8_t payloadType,
        const char payloadName[RTP_PAYLOAD_NAME_SIZE],
        const int frequency,
        const uint8_t channels,
        const uint32_t rate) ;

    //implements RtpFeedback

    // Receiving payload change or SSRC change. (return success!)
    virtual void OnIncomingSSRCChanged( const int32_t id,
                                        const uint32_t ssrc);

    virtual void OnIncomingCSRCChanged( const int32_t id,
                                        const uint32_t CSRC,
                                        const bool added);

    virtual void ResetStatistics(uint32_t ssrc);

    void UpdatePacketDelay(uint32_t rtp_timestamp,  uint16_t sequence_number);

    // VoEVideoSync
    bool GetDelayEstimate(int* jitter_buffer_delay_ms,
                          int* playout_buffer_delay_ms) const;
    int least_required_delay_ms() const { return least_required_delay_ms_; }
    int SetInitialPlayoutDelay(int delay_ms);
    int SetMinimumPlayoutDelay(int delayMs);
    int GetPlayoutTimestamp(unsigned int& timestamp);
    void UpdatePlayoutTimestamp(bool rtcp);
    int SetInitTimestamp(unsigned int timestamp);
    int SetInitSequenceNumber(short sequenceNumber);

    // VoEVideoSyncExtended
    int GetRtpRtcp(RtpRtcp** rtpRtcpModule, RtpReceiver** rtp_receiver) const;



private:
    bool IsPacketInOrder(const webrtc::RTPHeader& header) const;
    bool IsPacketRetransmitted(const webrtc::RTPHeader& header, bool in_order) const;
	bool ReceivePacket(const uint8_t* packet,
	                            int packet_length,
	                            const webrtc::RTPHeader& header,
	                            bool in_order);

private:
    int32_t _channelId;	//index id of the participant

    boost::shared_ptr<TaskRunner> taskRunner_;
    scoped_ptr<RtpHeaderParser> rtp_header_parser_;
    scoped_ptr<RTPPayloadRegistry> rtp_payload_registry_;
    scoped_ptr<ReceiveStatistics> rtp_receive_statistics_;
    scoped_ptr<RtpReceiver> rtp_receiver_;
    boost::mutex m_rtpReceiverMutex;
    scoped_ptr<RtpRtcp> _rtpRtcpModule;
    scoped_ptr<AudioCodingModule> audio_coding_;
    bool _externalTransport;
    AudioFrame _audioFrame;

    // Timestamp of the audio pulled from NetEq.
    uint32_t jitter_buffer_playout_timestamp_;
    uint32_t playout_timestamp_rtp_;
    uint32_t playout_timestamp_rtcp_;
    uint32_t playout_delay_ms_;
    uint32_t _numberOfDiscardedPackets;
    uint16_t send_sequence_number_;
    uint8_t restored_packet_[kVoiceEngineMaxIpPacketSizeBytes];

    // uses
    Statistics* _engineStatisticsPtr;

    Transport* _transportPtr; // WebRtc socket or external transport
    scoped_ptr<AudioProcessing> rtp_audioproc_;
    scoped_ptr<AudioProcessing> rx_audioproc_; // far end AudioProcessing


    // VoeRTP_RTCP
    uint32_t _lastLocalTimeStamp;
    uint32_t _lastRemoteTimeStamp;
    int8_t _lastPayloadType;
    bool _includeAudioLevelIndication;
    int video_channel_;
    uint32_t _timeStamp;
    // VoEVideoSync
    uint32_t _average_jitter_buffer_delay_us;
    int least_required_delay_ms_;
    uint32_t _previousTimestamp;
    uint16_t _recPacketDelayMs;
    // VoEAudioProcessing
    bool _RxVadDetection;
    bool _rxAgcIsEnabled;
    bool _rxNsIsEnabled;
    bool restored_packet_in_use_;
    boost::shared_ptr<ACMOutputProcessor> aop_;

};

class ACMOutputProcessor: public AudioPacketizationCallback,
							public AudioMixerStatusReceiver,
							public AudioMixerOutputReceiver,
							public FileCallback{
	DECLARE_LOGGER();
public:
	ACMOutputProcessor(uint32_t instanceId, woogeen_base::WoogeenTransport<erizo::AUDIO>* transport);
	virtual ~ACMOutputProcessor();
	bool init(boost::shared_ptr<TaskRunner>);

    int32_t SetAudioProcessingModule(
        AudioProcessing* audioProcessingModule) {
    	apm_ = audioProcessingModule;
    	return 0;
    }

    int32_t MixActiveChannels(const boost::system::error_code& ec);

    int32_t SetMixabilityStatus(MixerParticipant& participant,
                                bool mixable) {
    	return amixer_->SetMixabilityStatus(participant, mixable);
    }

    int32_t SetAnonymousMixabilityStatus(MixerParticipant& participant,
                                         bool mixable) {
    	return amixer_->SetAnonymousMixabilityStatus(participant, mixable);
    }

    int32_t SetSendCodec(const CodecInst& codec);
	//implements AudioPacketizationCallback
	virtual int32_t SendData(
	      FrameType frame_type,
	      uint8_t payload_type,
	      uint32_t timestamp,
	      const uint8_t* payload_data,
	      uint16_t payload_len_bytes,
	      const RTPFragmentationHeader* fragmentation);
	//implements AudioMixerOutputReceiver
    virtual void NewMixedAudio(const int32_t id,
                               const AudioFrame& generalAudioFrame,
                               const AudioFrame** uniqueAudioFrames,
                               const uint32_t size);
    // Callback function that provides an array of ParticipantStatistics for the
    // participants that were mixed last mix iteration.
    virtual void MixedParticipants(
        const int32_t id,
        const ParticipantStatistics* participantStatistics,
        const uint32_t size) {};
    // Callback function that provides an array of the ParticipantStatistics for
    // the participants that had a positiv VAD last mix iteration.
    virtual void VADPositiveParticipants(
        const int32_t id,
        const ParticipantStatistics* participantStatistics,
        const uint32_t size) {};
    // Callback function that provides the audio level of the mixed audio frame
    // from the last mix iteration.
    virtual void MixedAudioLevel(
        const int32_t  id,
        const uint32_t level) {};

    // This function is called by MediaFile when a file has been playing for
    // durationMs ms. id is the identifier for the MediaFile instance calling
    // the callback.
    virtual void PlayNotification(const int32_t id,
                                  const uint32_t durationMs);

    // This function is called by MediaFile when a file has been recording for
    // durationMs ms. id is the identifier for the MediaFile instance calling
    // the callback.
    virtual void RecordNotification(const int32_t id,
                                    const uint32_t durationMs);

    // This function is called by MediaFile when a file has been stopped
    // playing. id is the identifier for the MediaFile instance calling the
    // callback.
    virtual void PlayFileEnded(const int32_t id);

    // This function is called by MediaFile when a file has been stopped
    // recording. id is the identifier for the MediaFile instance calling the
    // callback.
    virtual void RecordFileEnded(const int32_t id);

    int StartRecordingPlayout(const char* fileName,  const CodecInst* codecInst);
    int StopRecordingPlayout();
    FileRecorder* getOutputFileRecorder() {
    	return _outputFileRecorderPtr;
    }

private:
	AudioConferenceMixer* amixer_;
	AudioProcessing*   apm_;
    AudioFrame audioFrame_;
    uint32_t instanceId_;
    boost::shared_ptr<TaskRunner> taskRunner_;
    scoped_ptr<RtpRtcp> _rtpRtcpModule;
    scoped_ptr<AudioCodingModule> audio_coding_;
    scoped_ptr<woogeen_base::WoogeenTransport<erizo::AUDIO>> m_audioTransport;	//owned by this ACMOutputProcessor

	boost::scoped_ptr<boost::thread> audioMixingThread_;
    boost::asio::io_service io_service_;
    boost::scoped_ptr<boost::asio::deadline_timer> timer_;
    std::atomic<bool> m_isStopping;

    FileRecorder* _outputFileRecorderPtr;
    bool _outputFileRecording;


};
} /* namespace mcu */
#endif /* ACMMEDIAPROCESSOR_H_ */
