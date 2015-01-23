/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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

#include "OutOfProcessMixerProxy.h"

#include "OutOfProcessMixerMessage.h"

namespace mcu {

OutOfProcessMixerProxy::OutOfProcessMixerProxy()
{
    m_audioOutput.reset(new AudioDataWriter());
    m_videoOutput.reset(new VideoDataWriter());
    m_messageOutput.reset(new MessageWriter());
}

OutOfProcessMixerProxy::~OutOfProcessMixerProxy()
{
}

int32_t OutOfProcessMixerProxy::addSource(uint32_t id, bool isAudio, erizo::FeedbackSink*, const std::string& participantId)
{
    OutOfProcessMixerMessage msg;
    msg.setType(isAudio ? ADD_AUDIO : ADD_VIDEO);
    msg.setMediaSource(id);

    char buf[1024];
    int len = sizeof(msg) + participantId.length();
    assert(len <= 1024);

    memcpy(buf, &msg, sizeof(msg));
    memcpy(buf + sizeof(msg), participantId.c_str(), participantId.length());

    return m_messageOutput->write(buf, len);
}

int32_t OutOfProcessMixerProxy::removeSource(uint32_t id, bool isAudio)
{
    OutOfProcessMixerMessage msg;
    msg.setType(isAudio ? REMOVE_AUDIO : REMOVE_VIDEO);
    msg.setMediaSource(id);

    return m_messageOutput->write(reinterpret_cast<char*>(&msg), sizeof(msg));
}

int32_t OutOfProcessMixerProxy::bindAV(uint32_t audioId, uint32_t videoId)
{
    OutOfProcessMixerMessage msg;
    msg.setType(BIND);
    msg.setMediaSource(audioId);

    char buf[1024];
    int len = sizeof(msg) + sizeof(uint32_t);
    assert(len <= 1024);

    memcpy(buf, &msg, sizeof(msg));
    memcpy(buf + sizeof(msg), &videoId, sizeof(uint32_t));

    return m_messageOutput->write(buf, len);
}

int OutOfProcessMixerProxy::deliverAudioData(char* buf, int len)
{
    return m_audioOutput->write(buf, len);
}

int OutOfProcessMixerProxy::deliverVideoData(char* buf, int len)
{
    return m_videoOutput->write(buf, len);
}

AudioDataWriter::AudioDataWriter()
{
    m_transport.reset(new woogeen_base::RawTransport(this));
    m_transport->createConnection("localhost", 44444, woogeen_base::UDP);
}

AudioDataWriter::~AudioDataWriter()
{
    m_transport->close();
}

void AudioDataWriter::onTransportConnected(woogeen_base::Protocol prot) 
{
    assert(prot == woogeen_base::UDP);
}

int32_t AudioDataWriter::write(char* buf, int len)
{
    m_transport->sendData(buf, len, woogeen_base::UDP);
    return len;
}

VideoDataWriter::VideoDataWriter()
{
    m_transport.reset(new woogeen_base::RawTransport(this));
    m_transport->createConnection("localhost", 55543, woogeen_base::UDP);
}

VideoDataWriter::~VideoDataWriter()
{
    m_transport->close();
}

void VideoDataWriter::onTransportConnected(woogeen_base::Protocol prot) 
{
    assert(prot == woogeen_base::UDP);
}

int32_t VideoDataWriter::write(char* buf, int len)
{
    m_transport->sendData(buf, len, woogeen_base::UDP);
    return len;
}

MessageWriter::MessageWriter()
{
    m_transport.reset(new woogeen_base::RawTransport(this));
    m_transport->createConnection("localhost", 33345, woogeen_base::UDP);
}

MessageWriter::~MessageWriter()
{
    m_transport->close();
}

void MessageWriter::onTransportConnected(woogeen_base::Protocol prot) 
{
    assert(prot == woogeen_base::UDP);
}

int32_t MessageWriter::write(char* buf, int len)
{
    m_transport->sendData(buf, len, woogeen_base::UDP);
    return len;
}

}/* namespace mcu */
