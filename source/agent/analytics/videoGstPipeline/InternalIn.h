// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef InternalIn_h
#define InternalIn_h

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <logger.h>

#include "RawTransport.h"

using namespace owt_base;

enum FrameFormat {
    FRAME_FORMAT_UNKNOWN    = 0,

    FRAME_FORMAT_I420       = 100,

    FRAME_FORMAT_VP8        = 200,
    FRAME_FORMAT_VP9,
    FRAME_FORMAT_H264,
    FRAME_FORMAT_H265,

    FRAME_FORMAT_MSDK       = 300,

    FRAME_FORMAT_PCM_48000_2    = 800,

    FRAME_FORMAT_PCMU       = 900,
    FRAME_FORMAT_PCMA,
    FRAME_FORMAT_OPUS,
    FRAME_FORMAT_ISAC16,
    FRAME_FORMAT_ISAC32,
    FRAME_FORMAT_ILBC,
    FRAME_FORMAT_G722_16000_1,
    FRAME_FORMAT_G722_16000_2,

    FRAME_FORMAT_AAC,           // ignore sample rate and channels for decoder, default is 48000_2
    FRAME_FORMAT_AAC_48000_2,   // specify sample rate and channels for encoder

    FRAME_FORMAT_AC3,
    FRAME_FORMAT_NELLYMOSER,
};

struct VideoFrameSpecificInfo {
    uint16_t width;
    uint16_t height;
    bool isKeyFrame;
};

struct AudioFrameSpecificInfo {
    /*AudioFrameSpecificInfo() : isRtpPacket(false) {}*/
    uint8_t isRtpPacket; // FIXME: Temporarily use Frame to carry rtp-packets due to the premature AudioFrameConstructor implementation.
    uint32_t nbSamples;
    uint32_t sampleRate;
    uint8_t channels;
};

typedef union MediaSpecInfo {
    VideoFrameSpecificInfo video;
    AudioFrameSpecificInfo audio;
} MediaSpecInfo;

struct Frame {
    FrameFormat     format;
    uint8_t*        payload;
    uint32_t        length;
    uint32_t        timeStamp;
    MediaSpecInfo   additionalInfo;
};

enum FeedbackType {
    VIDEO_FEEDBACK,
    AUDIO_FEEDBACK
};

enum FeedbackCmd {
    REQUEST_KEY_FRAME,
    SET_BITRATE,
    RTCP_PACKET  // FIXME: Temporarily use FeedbackMsg to carry audio rtcp-packets due to the premature AudioFrameConstructor implementation.
};

struct FeedbackMsg {
    FeedbackType type;
    FeedbackCmd  cmd;
    union {
        unsigned short kbps;
        struct RtcpPacket{// FIXME: Temporarily use FeedbackMsg to carry audio rtcp-packets due to the premature AudioFrameConstructor implementation.
            uint32_t len;
            char     buf[128];
        } rtcp;
    } data;
    FeedbackMsg(FeedbackType t, FeedbackCmd c) : type{t}, cmd{c} {}
};

class InternalIn : public RawTransportListener{
    DECLARE_LOGGER();
public:
    InternalIn(GstAppSrc *data, unsigned int minPort = 0, unsigned int maxPort = 0);
    virtual ~InternalIn();

    unsigned int getListeningPort();

    // Implements FrameSource
    void onFeedback(const FeedbackMsg&);

    // Implements RawTransportListener.
    void onTransportData(char* buf, int len);
    void onTransportError() { }
    void onTransportConnected() { }
    void setPushData(bool status);

private:
    bool m_start;
    bool m_needKeyFrame;
    GstAppSrc *appsrc;
    boost::shared_ptr<owt_base::RawTransportInterface> m_transport;
};


#endif /* InternalIn_h */
