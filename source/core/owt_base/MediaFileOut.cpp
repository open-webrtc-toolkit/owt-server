// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "MediaFileOut.h"

namespace owt_base {

DEFINE_LOGGER(MediaFileOut, "owt.media.MediaFileOut");

MediaFileOut::MediaFileOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int recordingTimeout)
    : AVStreamOut(url, hasAudio, hasVideo, handle, recordingTimeout)
{
}

MediaFileOut::~MediaFileOut()
{
    close();
}

bool MediaFileOut::isAudioFormatSupported(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_PCMU:
        case FRAME_FORMAT_PCMA:
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
        case FRAME_FORMAT_OPUS:
            return true;
        default:
            return false;
    }
}

bool MediaFileOut::isVideoFormatSupported(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_VP8:
        case FRAME_FORMAT_VP9:
        case FRAME_FORMAT_H264:
        case FRAME_FORMAT_H265:
            return true;
        default:
            return false;
    }
}

const char *MediaFileOut::getFormatName(std::string& url)
{
    size_t pos;

    if ((pos = url.rfind(".mkv")) != std::string::npos) {
        if (pos > 0 && pos == (url.length() - strlen(".mkv")))
            return "matroska";
    } else if ((pos = url.rfind(".mp4")) != std::string::npos) {
        if (pos > 0 && pos == (url.length() - strlen(".mp4")))
            return "mp4";
    }

    ELOG_ERROR("Invalid format for url(%s)", url.c_str());
    return NULL;
}

bool MediaFileOut::getHeaderOpt(std::string& url, AVDictionary **options)
{
    return true;
}

void MediaFileOut::onVideoSourceChanged()
{
    ELOG_DEBUG("onVideoSourceChanged");

    setVideoSourceChanged();
    deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME});
}

} // namespace owt_base
