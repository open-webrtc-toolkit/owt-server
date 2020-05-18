// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MediaFramePipeline_h
#define MediaFramePipeline_h

#include <boost/thread/shared_mutex.hpp>
#include <list>
#include <map>
#include <stdint.h>
#include <string>

namespace owt_base {

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

    FRAME_FORMAT_DATA,  // Generic data frame. We don't know its detailed structure.
};

enum VideoCodecProfile {
    PROFILE_UNKNOWN                     = 0,

    /* AVC Profiles */
    PROFILE_AVC_CONSTRAINED_BASELINE    = 66 + (0x100 << 1),
    PROFILE_AVC_BASELINE                = 66,
    PROFILE_AVC_MAIN                    = 77,
    //PROFILE_AVC_EXTENDED                = 88,
    PROFILE_AVC_HIGH                    = 100,
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
    uint8_t voice;
    uint8_t audioLevel;
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

inline FrameFormat getFormat(const std::string& codec) {
    if (codec == "vp8") {
        return owt_base::FRAME_FORMAT_VP8;
    } else if (codec == "h264") {
        return owt_base::FRAME_FORMAT_H264;
    } else if (codec == "vp9") {
        return owt_base::FRAME_FORMAT_VP9;
    } else if (codec == "h265") {
        return owt_base::FRAME_FORMAT_H265;
    } else if (codec == "pcm_48000_2" || codec == "pcm_raw") {
        return owt_base::FRAME_FORMAT_PCM_48000_2;
    } else if (codec == "pcmu") {
        return owt_base::FRAME_FORMAT_PCMU;
    } else if (codec == "pcma") {
        return owt_base::FRAME_FORMAT_PCMA;
    } else if (codec == "isac_16000") {
        return owt_base::FRAME_FORMAT_ISAC16;
    } else if (codec == "isac_32000") {
        return owt_base::FRAME_FORMAT_ISAC32;
    } else if (codec == "ilbc") {
        return owt_base::FRAME_FORMAT_ILBC;
    } else if (codec == "g722_16000_1") {
        return owt_base::FRAME_FORMAT_G722_16000_1;
    } else if (codec == "g722_16000_2") {
        return owt_base::FRAME_FORMAT_G722_16000_2;
    } else if (codec == "opus_48000_2") {
        return owt_base::FRAME_FORMAT_OPUS;
    } else if (codec.compare(0, 3, "aac") == 0) {
        if (codec == "aac_48000_2")
            return owt_base::FRAME_FORMAT_AAC_48000_2;
        else
            return owt_base::FRAME_FORMAT_AAC;
    } else if (codec.compare(0, 3, "ac3") == 0) {
        return owt_base::FRAME_FORMAT_AC3;
    } else if (codec.compare(0, 10, "nellymoser") == 0) {
        return owt_base::FRAME_FORMAT_NELLYMOSER;
    } else {
        return owt_base::FRAME_FORMAT_UNKNOWN;
    }
}

inline const char *getFormatStr(const FrameFormat &format) {
    switch(format) {
        case FRAME_FORMAT_UNKNOWN:
            return "UNKNOWN";
        case FRAME_FORMAT_I420:
            return "I420";
        case FRAME_FORMAT_MSDK:
            return "MSDK";
        case FRAME_FORMAT_VP8:
            return "VP8";
        case FRAME_FORMAT_VP9:
            return "VP9";
        case FRAME_FORMAT_H264:
            return "H264";
        case FRAME_FORMAT_H265:
            return "H265";
        case FRAME_FORMAT_PCM_48000_2:
            return "PCM_48000_2";
        case FRAME_FORMAT_PCMU:
            return "PCMU";
        case FRAME_FORMAT_PCMA:
            return "PCMA";
        case FRAME_FORMAT_OPUS:
            return "OPUS";
        case FRAME_FORMAT_ISAC16:
            return "ISAC16";
        case FRAME_FORMAT_ISAC32:
            return "ISAC32";
        case FRAME_FORMAT_ILBC:
            return "ILBC";
        case FRAME_FORMAT_G722_16000_1:
            return "G722_16000_1";
        case FRAME_FORMAT_G722_16000_2:
            return "G722_16000_2";
        case FRAME_FORMAT_AAC:
            return "AAC";
        case FRAME_FORMAT_AAC_48000_2:
            return "AAC_48000_2";
        case FRAME_FORMAT_AC3:
            return "AC3";
        case FRAME_FORMAT_NELLYMOSER:
            return "NELLYMOSER";
        default:
            return "INVALID";
    }
}

inline bool isAudioFrame(const Frame& frame) {
    return frame.format == FRAME_FORMAT_PCM_48000_2
          || frame.format == FRAME_FORMAT_PCMU
          || frame.format == FRAME_FORMAT_PCMA
          || frame.format == FRAME_FORMAT_OPUS
          || frame.format == FRAME_FORMAT_ISAC16
          || frame.format == FRAME_FORMAT_ISAC32
          || frame.format == FRAME_FORMAT_ILBC
          || frame.format == FRAME_FORMAT_G722_16000_1
          || frame.format == FRAME_FORMAT_G722_16000_2
          || frame.format == FRAME_FORMAT_AAC
          || frame.format == FRAME_FORMAT_AAC_48000_2
          || frame.format == FRAME_FORMAT_AC3
          || frame.format == FRAME_FORMAT_NELLYMOSER;
}

inline bool isVideoFrame(const Frame& frame) {
    return frame.format == FRAME_FORMAT_I420
          || frame.format ==FRAME_FORMAT_MSDK
          || frame.format == FRAME_FORMAT_VP8
          || frame.format == FRAME_FORMAT_VP9
          || frame.format == FRAME_FORMAT_H264
          || frame.format == FRAME_FORMAT_H265;
}

inline bool isDataFrame(const Frame& frame) {
    return frame.format == FRAME_FORMAT_DATA;
}

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

class FrameDestination;
class FrameSource {
public:
    FrameSource() { }
    virtual ~FrameSource();

    virtual void onFeedback(const FeedbackMsg&) { };

    void addAudioDestination(FrameDestination*);
    void removeAudioDestination(FrameDestination*);

    void addVideoDestination(FrameDestination*);
    void removeVideoDestination(FrameDestination*);

    void addDataDestination(FrameDestination*);
    void removeDataDestination(FrameDestination*);

protected:
    void deliverFrame(const Frame&);

private:
    std::list<FrameDestination*> m_audio_dests;
    boost::shared_mutex m_audio_dests_mutex;
    std::list<FrameDestination*> m_video_dests;
    boost::shared_mutex m_video_dests_mutex;
    std::list<FrameDestination*> m_data_dests;
    boost::shared_mutex m_data_dests_mutex;
};


class FrameDestination {
public:
    FrameDestination() : m_audio_src(nullptr), m_video_src(nullptr), m_data_src(nullptr) { }
    virtual ~FrameDestination() { }

    virtual void onFrame(const Frame&) = 0;
    virtual void onVideoSourceChanged() {}

    void setAudioSource(FrameSource*);
    void unsetAudioSource();

    void setVideoSource(FrameSource*);
    void unsetVideoSource();

    void setDataSource(FrameSource*);
    void unsetDataSource();

    bool hasAudioSource() { return m_audio_src != nullptr; }
    bool hasVideoSource() { return m_video_src != nullptr; }
    bool hasDataSource() { return m_data_src != nullptr; }

protected:
    void deliverFeedbackMsg(const FeedbackMsg& msg);

private:
    FrameSource* m_audio_src;
    boost::shared_mutex m_audio_src_mutex;
    FrameSource* m_video_src;
    boost::shared_mutex m_video_src_mutex;
    FrameSource* m_data_src;
    boost::shared_mutex m_data_src_mutex;
};

class VideoFrameDecoder : public FrameSource, public FrameDestination {
public:
    virtual ~VideoFrameDecoder() { }
    virtual bool init(FrameFormat) = 0;
};

class VideoFrameProcesser : public FrameSource, public FrameDestination {
public:
    virtual ~VideoFrameProcesser() { }
    virtual bool init(FrameFormat format, const uint32_t width, const uint32_t height, const uint32_t frameRate) = 0;
    virtual void drawText(const std::string& textSpec) = 0;
    virtual void clearText() = 0;
};

class VideoFrameAnalyzer : public FrameSource, public FrameDestination {
public:
    virtual ~VideoFrameAnalyzer() { }
    virtual bool init(FrameFormat format, const uint32_t width, const uint32_t height, const uint32_t frameRate, const std::string& pluginName) = 0;
};

class VideoFrameEncoder : public FrameDestination {
public:
    virtual ~VideoFrameEncoder() { }

    virtual FrameFormat getInputFormat() = 0;

    virtual bool canSimulcast(FrameFormat, uint32_t width, uint32_t height) = 0;
    virtual bool isIdle() = 0;
    virtual int32_t generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, FrameDestination*) = 0;
    virtual void degenerateStream(int32_t streamId) = 0;
    virtual void setBitrate(unsigned short kbps, int32_t streamId) = 0;
    virtual void requestKeyFrame(int32_t streamId) = 0;
};

}
#endif
