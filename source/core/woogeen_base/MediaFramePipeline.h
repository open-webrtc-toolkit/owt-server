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

#ifndef MediaFramePipeline_h
#define MediaFramePipeline_h

#include <boost/thread/shared_mutex.hpp>
#include <list>
#include <map>
#include <stdint.h>
#include <string>

namespace woogeen_base {

enum FrameFormat {
    FRAME_FORMAT_UNKNOWN    = 0,

    FRAME_FORMAT_I420       = 100,

    FRAME_FORMAT_VP8        = 200,
    FRAME_FORMAT_VP9,
    FRAME_FORMAT_H264,
    FRAME_FORMAT_H265,

    FRAME_FORMAT_MSDK       = 300,

    FRAME_FORMAT_YAMI       = 400,

    FRAME_FORMAT_PCM_RAW    = 800,

    FRAME_FORMAT_PCMU       = 900,
    FRAME_FORMAT_PCMA,
    FRAME_FORMAT_OPUS,
    FRAME_FORMAT_ISAC16,
    FRAME_FORMAT_ISAC32,
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

inline FrameFormat getFormat(const std::string& codec) {
    if (codec == "vp8") {
        return woogeen_base::FRAME_FORMAT_VP8;
    } else if (codec == "h264") {
        return woogeen_base::FRAME_FORMAT_H264;
    } else if (codec == "vp9") {
        return woogeen_base::FRAME_FORMAT_VP9;
    } else if (codec == "h265") {
        return woogeen_base::FRAME_FORMAT_H265;
    } else {
        return woogeen_base::FRAME_FORMAT_UNKNOWN;
    }
}

inline const char *getFormatStr(const FrameFormat &format) {
    switch(format) {
        case FRAME_FORMAT_UNKNOWN:
            return "UNKNOWN";
        case FRAME_FORMAT_I420:
            return "I420";
        case FRAME_FORMAT_YAMI:
            return "YAMI";
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
        case FRAME_FORMAT_PCM_RAW:
            return "PCM_RAW";
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
        default:
            return "INVALID";
    }
}

inline bool isAudioFrame(const Frame& frame) {
    return frame.format == FRAME_FORMAT_PCM_RAW
          || frame.format == FRAME_FORMAT_PCMU
          || frame.format == FRAME_FORMAT_PCMA
          || frame.format == FRAME_FORMAT_OPUS
          || frame.format == FRAME_FORMAT_ISAC16
          || frame.format == FRAME_FORMAT_ISAC32;
}

inline bool isVideoFrame(const Frame& frame) {
    return frame.format == FRAME_FORMAT_I420
          || frame.format ==FRAME_FORMAT_YAMI
          || frame.format ==FRAME_FORMAT_MSDK
          || frame.format == FRAME_FORMAT_VP8
          || frame.format == FRAME_FORMAT_VP9
          || frame.format == FRAME_FORMAT_H264
          || frame.format == FRAME_FORMAT_H265;
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

protected:
    void deliverFrame(const Frame&);

private:
    std::list<FrameDestination*> m_audio_dests;
    boost::shared_mutex m_audio_dests_mutex;
    std::list<FrameDestination*> m_video_dests;
    boost::shared_mutex m_video_dests_mutex;
};


class FrameDestination {
public:
    FrameDestination() : m_audio_src(nullptr), m_video_src(nullptr) { }
    virtual ~FrameDestination() { }

    virtual void onFrame(const Frame&) = 0;
    virtual void onVideoSourceChanged() {}

    void setAudioSource(FrameSource*);
    void unsetAudioSource();

    void setVideoSource(FrameSource*);
    void unsetVideoSource();

    bool hasAudioSource() { return m_audio_src != nullptr; }
    bool hasVideoSource() { return m_video_src != nullptr; }

protected:
    void deliverFeedbackMsg(const FeedbackMsg& msg);

private:
    FrameSource* m_audio_src;
    boost::shared_mutex m_audio_src_mutex;
    FrameSource* m_video_src;
    boost::shared_mutex m_video_src_mutex;
};

class VideoFrameDecoder : public FrameSource, public FrameDestination {
public:
    virtual ~VideoFrameDecoder() { }
    virtual bool init(FrameFormat) = 0;
};

class VideoFrameProcesser : public FrameSource, public FrameDestination {
public:
    virtual ~VideoFrameProcesser() { }
    virtual bool init(FrameFormat format) = 0;
};

class VideoFrameEncoder : public FrameDestination {
public:
    virtual ~VideoFrameEncoder() { }

    virtual FrameFormat getInputFormat() = 0;

    virtual bool canSimulcast(FrameFormat, uint32_t width, uint32_t height) = 0;
    virtual bool isIdle() = 0;
    virtual int32_t generateStream(uint32_t width, uint32_t height, uint32_t bitrateKbps, FrameDestination*) = 0;
    virtual void degenerateStream(int32_t streamId) = 0;
    virtual void setBitrate(unsigned short kbps, int32_t streamId) = 0;
    virtual void requestKeyFrame(int32_t streamId) = 0;
};

enum QualityLevel {
    QUALITY_LEVEL_BEST_QUALITY = 0,   //1.4
    QUALITY_LEVEL_QUALITY,            //1.2
    QUALITY_LEVEL_STANDARD,           //1.0
    QUALITY_LEVEL_SPEED,              //0.8
    QUALITY_LEVEL_BEST_SPEED,         //0.6

    QUALITY_LEVEL_AUTO,
};

inline double getQualityLevelMultiplier(const QualityLevel &level) {
    double multiplier = 0;
    switch(level) {
        case QUALITY_LEVEL_BEST_QUALITY:
            multiplier = 1.4;
            break;
        case QUALITY_LEVEL_QUALITY:
            multiplier = 1.2;
            break;
        case QUALITY_LEVEL_STANDARD:
            multiplier = 1;
            break;
        case QUALITY_LEVEL_SPEED:
            multiplier = 0.8;
            break;
        case QUALITY_LEVEL_BEST_SPEED:
            multiplier = 0.6;
            break;
        case QUALITY_LEVEL_AUTO:
        default:
            multiplier = 0;
            break;
    }
    return multiplier;
}

}
#endif
