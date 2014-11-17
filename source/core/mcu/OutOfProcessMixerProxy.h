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

#ifndef OutOfProcessMixerProxy_h
#define OutOfProcessMixerProxy_h

#include <boost/scoped_ptr.hpp>
#include <MediaDefinitions.h>
#include <MediaSourceConsumer.h>
#include <RawTransport.h>

namespace mcu {

class AudioDataWriter : public woogeen_base::RawTransportListener {
public:
    AudioDataWriter();
    ~AudioDataWriter();

    // Implements RawTransportListener.
    // TODO: onTransportData should be implemented to handle the feedback.
    void onTransportData(char*, int len, woogeen_base::Protocol) { }
    void onTransportError(woogeen_base::Protocol) { }
    void onTransportConnected(woogeen_base::Protocol);

    int32_t write(char*, int len);

private:
    boost::scoped_ptr<woogeen_base::RawTransport> m_transport;
};

class VideoDataWriter : public woogeen_base::RawTransportListener {
public:
    VideoDataWriter();
    ~VideoDataWriter();

    // Implements RawTransportListener.
    // TODO: onTransportData should be implemented to handle the feedback.
    void onTransportData(char*, int len, woogeen_base::Protocol) { }
    void onTransportError(woogeen_base::Protocol) { }
    void onTransportConnected(woogeen_base::Protocol);

    int32_t write(char*, int len);

private:
    boost::scoped_ptr<woogeen_base::RawTransport> m_transport;
};

class OutOfProcessMixerProxy : public woogeen_base::MediaSourceConsumer, public erizo::MediaSink {
public:
    OutOfProcessMixerProxy();
    ~OutOfProcessMixerProxy();

    // Implement MediaSourceConsumer.
    // TODO: Add real implementation later.
    int32_t addSource(uint32_t id, bool isAudio, erizo::FeedbackSink*) { return -1; }
    int32_t removeSource(uint32_t id, bool isAudio) { return -1; }
    int32_t bindAV(uint32_t audioId, uint32_t videoId) { return -1; }
    void configLayout(const std::string&) { }
    erizo::MediaSink* mediaSink() { return this; }

    // Implement MediaSink.
    int deliverAudioData(char* buf, int len);
    int deliverVideoData(char* buf, int len);

private:
    boost::scoped_ptr<AudioDataWriter> m_audioOutput;
    boost::scoped_ptr<VideoDataWriter> m_videoOutput;
};

} /* namespace mcu */
#endif /* OutOfProcessMixerProxy_h */
