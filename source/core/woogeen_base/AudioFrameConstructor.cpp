/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#include "AudioFrameConstructor.h"
#include "AudioUtilities.h"

#include <rtputils.h>

namespace woogeen_base {

DEFINE_LOGGER(AudioFrameConstructor, "woogeen.AudioFrameConstructor");

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

    if (m_enabled) {
        deliverFrame(frame);
    }
    return audio_packet->length;
}

void AudioFrameConstructor::onFeedback(const FeedbackMsg& msg)
{
    if (msg.type == woogeen_base::AUDIO_FEEDBACK) {
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
