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

#ifndef OutOfProcessMixer_h
#define OutOfProcessMixer_h

#include "Mixer.h"

#include <boost/scoped_ptr.hpp>
#include <RawTransport.h>

namespace mcu {

class AudioDataReader : public woogeen_base::RawTransportListener {
public:
    AudioDataReader(erizo::MediaSink*);
    ~AudioDataReader();

    // Implements RawTransportListener.
    void onTransportData(char*, int len, woogeen_base::Protocol);
    void onTransportError(woogeen_base::Protocol) { }
    void onTransportConnected(woogeen_base::Protocol) { }

private:
    erizo::MediaSink* m_sink;
    boost::scoped_ptr<woogeen_base::RawTransport> m_transport;
};

class VideoDataReader : public woogeen_base::RawTransportListener {
public:
    VideoDataReader(erizo::MediaSink*);
    ~VideoDataReader();

    // Implements RawTransportListener.
    void onTransportData(char*, int len, woogeen_base::Protocol);
    void onTransportError(woogeen_base::Protocol) { }
    void onTransportConnected(woogeen_base::Protocol) { }

private:
    erizo::MediaSink* m_sink;
    boost::scoped_ptr<woogeen_base::RawTransport> m_transport;
};

/**
 * An OutOfProcessMixer refers to a media mixer which is run in a dedicated process as a woogeen_base::Gateway.
 */
class OutOfProcessMixer : public Mixer {
public:
    OutOfProcessMixer(boost::property_tree::ptree& videoConfig);
    ~OutOfProcessMixer();

private:
    boost::scoped_ptr<AudioDataReader> m_audioInput;
    boost::scoped_ptr<VideoDataReader> m_videoInput;
};

} /* namespace mcu */
#endif /* OutOfProcessMixer_h */
