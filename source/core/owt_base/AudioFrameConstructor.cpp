// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioFrameConstructor.h"
#include "AudioUtilities.h"

#include <rtputils.h>

#include <webrtc/modules/rtp_rtcp/source/rtp_packet.h>
#include <webrtc/modules/rtp_rtcp/source/rtp_packet_received.h>
#include <webrtc/modules/rtp_rtcp/source/rtp_header_extensions.h>


namespace owt_base {

DEFINE_LOGGER(AudioFrameConstructor, "owt.AudioFrameConstructor");

// TODO: Get the extension ID from SDP
constexpr uint8_t kAudioLevelExtensionId = 1;

AudioFrameConstructor::AudioFrameConstructor()
  : m_enabled(true)
  , m_transport(nullptr)
{
    sink_fb_source_ = this;
    m_extensions.Register<webrtc::AudioLevel>(kAudioLevelExtensionId);
}

AudioFrameConstructor::~AudioFrameConstructor()
{
    unbindTransport();
}

void AudioFrameConstructor::bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    m_transport = source;
    m_transport->setAudioSink(this);
    m_transport->setEventSink(this);
    setFeedbackSink(fbSink);
}

void AudioFrameConstructor::unbindTransport()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (m_transport) {
        setFeedbackSink(nullptr);
        m_transport = nullptr;
    }
}

int AudioFrameConstructor::deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet)
{
    assert(false);
    return 0;
}

int AudioFrameConstructor::deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet)
{
    if (audio_packet->length <= 0)
      return 0;

    FrameFormat frameFormat;
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    RTPHeader* head = (RTPHeader*) (audio_packet->data);

    frameFormat = getAudioFrameFormat(head->getPayloadType());
    if (frameFormat == FRAME_FORMAT_UNKNOWN)
        return 0;

    frame.additionalInfo.audio.sampleRate = getAudioSampleRate(frameFormat);
    frame.additionalInfo.audio.channels = getAudioChannels(frameFormat);

    frame.format = frameFormat;
    frame.payload = reinterpret_cast<uint8_t*>(audio_packet->data);
    frame.length = audio_packet->length;
    frame.timeStamp = head->getTimestamp();
    frame.additionalInfo.audio.isRtpPacket = 1;

    webrtc::RtpPacketReceived rtp_packet(&m_extensions);
    if(rtp_packet.Parse(frame.payload, frame.length)) {
        uint8_t audio_level = 0;
        bool voice = false;
        if (rtp_packet.GetExtension<webrtc::AudioLevel>(&voice, &audio_level)) {
            frame.additionalInfo.audio.audioLevel = audio_level;
            frame.additionalInfo.audio.voice = voice;
        } else {
            ELOG_DEBUG("No audio level extension %u, %d", audio_level, voice);
        }
    } else {
        ELOG_DEBUG("RtpPacket parse fail");
    }

    if (m_enabled) {
        deliverFrame(frame);
    }
    return audio_packet->length;
}

void AudioFrameConstructor::onFeedback(const FeedbackMsg& msg)
{
    if (msg.type == owt_base::AUDIO_FEEDBACK) {
        boost::shared_lock<boost::shared_mutex> lock(m_transport_mutex);
        if (msg.cmd == RTCP_PACKET && fb_sink_)
            fb_sink_->deliverFeedback(std::make_shared<erizo::DataPacket>(0, msg.data.rtcp.buf, msg.data.rtcp.len, erizo::AUDIO_PACKET));
    }
}

int AudioFrameConstructor::deliverEvent_(erizo::MediaEventPtr event){
    return 0;
}

void AudioFrameConstructor::close(){
    unbindTransport();
}

}
