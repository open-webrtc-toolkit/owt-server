// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioFrameConstructor.h"
#include "AudioUtilitiesNew.h"

#include <rtputils.h>
#include <rtp/RtpHeaders.h>


namespace owt_base {

DEFINE_LOGGER(AudioFrameConstructor, "owt.AudioFrameConstructor");

// TODO: Get the extension ID from SDP
constexpr uint8_t kAudioLevelExtensionId = 1;

AudioFrameConstructor::AudioFrameConstructor()
    : m_enabled(true)
    , m_transport(nullptr)
{
    sink_fb_source_ = this;
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

class AudioLevel {
 public:
  inline uint8_t getId() {
    return ext_info >> 4;
  }
  inline uint8_t getLength() {
    return (ext_info & 0x0F);
  }
  inline bool getVoice() {
    return (al_data & 0x80) != 0;
  }
  inline uint8_t getLevel() {
    return al_data & 0x7F;
  }
 private:
  uint32_t ext_info:8;
  uint8_t al_data:8;
};

std::unique_ptr<AudioLevel> parseAudioLevel(std::shared_ptr<erizo::DataPacket> p) {
    const erizo::RtpHeader* head = reinterpret_cast<const erizo::RtpHeader*>(p->data);
    std::unique_ptr<AudioLevel> ret;
    if (head->getExtension()) {
        uint16_t totalExtLength = head->getExtLength();
        if (head->getExtId() == 0xBEDE) {
            char* extBuffer = (char*)&head->extensions;  // NOLINT
            uint8_t extByte = 0;
            uint16_t currentPlace = 1;
            uint8_t extId = 0;
            uint8_t extLength = 0;
            while (currentPlace < (totalExtLength*4)) {
                extByte = (uint8_t)(*extBuffer);
                extId = extByte >> 4;
                extLength = extByte & 0x0F;
                if (extId == kAudioLevelExtensionId) {
                    ret.reset(new AudioLevel());
                    memcpy(ret.get(), extBuffer, sizeof(AudioLevel));
                    break;
                }
                extBuffer = extBuffer + extLength + 2;
                currentPlace = currentPlace + extLength + 2;
            }
        }
    }
    return ret;
}

int AudioFrameConstructor::deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet)
{
    if (audio_packet->length <= 0)
        return 0;

    FrameFormat frameFormat;
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    RTPHeader* head = (RTPHeader*)(audio_packet->data);

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

    std::unique_ptr<AudioLevel> audioLevel = parseAudioLevel(audio_packet);
    if (audioLevel) {
        frame.additionalInfo.audio.audioLevel = audioLevel->getLevel();
        frame.additionalInfo.audio.voice = audioLevel->getVoice();
        ELOG_DEBUG("Has audio level extension %u, %d", audioLevel->getLevel(), audioLevel->getVoice());
    } else {
        ELOG_DEBUG("No audio level extension");
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

int AudioFrameConstructor::deliverEvent_(erizo::MediaEventPtr event)
{
    return 0;
}

void AudioFrameConstructor::close()
{
    unbindTransport();
}
}
