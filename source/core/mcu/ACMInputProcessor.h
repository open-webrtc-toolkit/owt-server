/*
 * Copyright (c) 2012, The WebRTC project authors. All rights reserved.
 * Copyright (c) 2014, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:

 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of Google nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ACMInputProcessor_h
#define ACMInputProcessor_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <WoogeenTransport.h>
#include <webrtc/modules/audio_coding/main/interface/audio_coding_module.h>
#include <webrtc/modules/audio_conference_mixer/interface/audio_conference_mixer.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_payload_registry.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_receiver.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/system_wrappers/interface/scoped_ptr.h>
#include <webrtc/video_engine/overuse_frame_detector.h>
#include <webrtc/voice_engine/voice_engine_defines.h>

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

    // Implements RtpData.
    virtual int32_t OnReceivedPayloadData(
        const uint8_t* payloadData,
        const uint16_t payloadSize,
        const WebRtcRTPHeader* rtpHeader) ;

    virtual bool OnRecoveredPacket(const uint8_t* packet,
                                   int packet_length);


    /*
     * channels - number of channels in codec (1 = mono, 2 = stereo)
     */
    virtual int32_t OnInitializeDecoder(
        const int32_t id,
        const int8_t payloadType,
        const char payloadName[RTP_PAYLOAD_NAME_SIZE],
        const int frequency,
        const uint8_t channels,
        const uint32_t rate) ;

    // Implements RtpFeedback.

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
    int ResendPackets(const uint16_t* sequence_numbers, int length);
    bool ReceivePacket(const uint8_t* packet,
                                int packet_length,
                                const webrtc::RTPHeader& header,
                                bool in_order);

private:
    int32_t _channelId;    //index id of the participant

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

} /* namespace mcu */

#endif /* ACMInputProcessor_h */
