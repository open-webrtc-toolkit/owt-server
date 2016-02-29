/*
 * Copyright 2015 Intel Corporation All Rights Reserved.
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

#include <stdint.h>
#include <list>
#include <map>
#include <string>
#include <boost/thread/shared_mutex.hpp>

namespace woogeen_base {

enum FrameFormat {
    FRAME_FORMAT_UNKNOWN,

    FRAME_FORMAT_I420,
    FRAME_FORMAT_YAMI,
    FRAME_FORMAT_VP8,
    FRAME_FORMAT_H264,

    FRAME_FORMAT_PCM_RAW,
    FRAME_FORMAT_PCMU,
    FRAME_FORMAT_PCMA,
    FRAME_FORMAT_OPUS,
    FRAME_FORMAT_ISAC16,
    FRAME_FORMAT_ISAC32
};

struct VideoFrameSpecificInfo {
    uint16_t width;
    uint16_t height;
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

class FrameDestination;
class FrameSource {
public:
    FrameSource() {}
    virtual ~FrameSource();

    virtual void onFeedback(const FeedbackMsg&) {};

    void addAudioDestination(FrameDestination* dest);
    void removeAudioDestination(FrameDestination* dest);

    void addVideoDestination(FrameDestination* dest);
    void removeVideoDestination(FrameDestination* dest);

protected:
    void deliverFrame(const Frame& frame);

private:
    std::list<FrameDestination*> m_audio_dests;
    boost::shared_mutex m_audio_dests_mutex;
    std::list<FrameDestination*> m_video_dests;
    boost::shared_mutex m_video_dests_mutex;
};


class FrameDestination {
public:
    FrameDestination() : m_audio_src(nullptr), m_video_src(nullptr) {}
    virtual ~FrameDestination() {}

    virtual void onFrame(const Frame& frame) = 0;

    void setAudioSource(FrameSource* src);
    void unsetAudioSource();

    void setVideoSource(FrameSource* src);
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
    virtual ~VideoFrameDecoder() {}
    virtual bool init(FrameFormat format) = 0;
};

class VideoFrameEncoder : public FrameDestination {
public:
    virtual ~VideoFrameEncoder() {}

    virtual bool canSimulcastFor(FrameFormat format, uint32_t width, uint32_t height) = 0;
    virtual bool isIdle() = 0;
    virtual int32_t generateStream(uint32_t width, uint32_t height, FrameDestination* dest) = 0;
    virtual void degenerateStream(int32_t streamId) = 0;
    virtual void setBitrate(unsigned short kbps, int32_t streamId) = 0;
    virtual void requestKeyFrame(int32_t streamId) = 0;

};

}
#endif
