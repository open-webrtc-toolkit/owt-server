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

#include "MediaFileOut.h"

namespace woogeen_base {

DEFINE_LOGGER(MediaFileOut, "woogeen.media.MediaFileOut");

MediaFileOut::MediaFileOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int recordingTimeout)
    : AVStreamOut(url, hasAudio, hasVideo, handle, recordingTimeout)
{
    start();
}

MediaFileOut::~MediaFileOut()
{
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

void MediaFileOut::onVideoSourceChanged()
{
    ELOG_DEBUG("onVideoSourceChanged");

    m_videoSourceChanged = true;
    deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME});
}

} // namespace woogeen_base
