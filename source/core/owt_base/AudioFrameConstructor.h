// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioFrameConstructor_h
#define AudioFrameConstructor_h

#include "MediaFramePipeline.h"

#include <logger.h>
#include <MediaDefinitions.h>
#include <MediaDefinitionExtra.h>

#include <webrtc/modules/rtp_rtcp/source/rtp_header_extension.h>

namespace owt_base {

/**
 * A class to process the incoming streams by leveraging video coding module from
 * webrtc engine, which will framize and decode the frames.
 */
class AudioFrameConstructor : public erizo::MediaSink,
                              public erizo::FeedbackSource,
                              public FrameSource {
    DECLARE_LOGGER();

public:
    AudioFrameConstructor();
    virtual ~AudioFrameConstructor();

    void bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink);
    void unbindTransport();
    void enable(bool enabled) {m_enabled = enabled;}

    // Implements the MediaSink interfaces.
    // int deliverAudioData(char*, int len);
    // int deliverVideoData(char*, int len);

    // Implements the FrameSource interfaces.
    void onFeedback(const FeedbackMsg& msg);

private:
    bool m_enabled;
    erizo::MediaSource* m_transport;
    boost::shared_mutex m_transport_mutex;

    ////////////// NEW INTERFACE ///////////
    int deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet) override;
    int deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet) override;
    int deliverEvent_(erizo::MediaEventPtr event) override;
    void close();

    webrtc::RtpHeaderExtensionMap m_extensions;
};

}
#endif /* AudioFrameConstructor_h */
