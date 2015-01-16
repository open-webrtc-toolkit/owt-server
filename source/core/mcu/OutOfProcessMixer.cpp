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

#include "OutOfProcessMixer.h"

namespace mcu {

OutOfProcessMixer::OutOfProcessMixer(boost::property_tree::ptree& videoConfig)
    : Mixer(videoConfig)
{
    m_audioInput.reset(new AudioDataReader(this));
    m_videoInput.reset(new VideoDataReader(this));

    m_audioMixer->addSourceOnDemand(true);
    m_videoMixer->addSourceOnDemand(true);
}

OutOfProcessMixer::~OutOfProcessMixer()
{
}

AudioDataReader::AudioDataReader(erizo::MediaSink* sink)
    : m_sink(sink)
{
    m_transport.reset(new woogeen_base::RawTransport(this));
    m_transport->listenTo(44444, woogeen_base::UDP);
}

AudioDataReader::~AudioDataReader()
{
    m_transport->close();
}

void AudioDataReader::onTransportData(char* buf, int len, woogeen_base::Protocol prot) 
{
    assert(prot == woogeen_base::UDP);
    if (m_sink)
        m_sink->deliverAudioData(buf, len);
}

VideoDataReader::VideoDataReader(erizo::MediaSink* sink)
    : m_sink(sink)
{
    m_transport.reset(new woogeen_base::RawTransport(this));
    m_transport->listenTo(55543, woogeen_base::UDP);
}

VideoDataReader::~VideoDataReader()
{
    m_transport->close();
}

void VideoDataReader::onTransportData(char* buf, int len, woogeen_base::Protocol prot) 
{
    assert(prot == woogeen_base::UDP);
    if (m_sink)
        m_sink->deliverVideoData(buf, len);
}

}/* namespace mcu */
