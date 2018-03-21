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

#include "LiveStreamOut.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avstring.h>
}

namespace woogeen_base {

DEFINE_LOGGER(LiveStreamOut, "woogeen.LiveStreamOut");

LiveStreamOut::LiveStreamOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int streamingTimeout)
    : AVStreamOut(url, hasAudio, hasVideo, handle, streamingTimeout)
{
}

LiveStreamOut::~LiveStreamOut()
{
}

bool LiveStreamOut::isAudioFormatSupported(FrameFormat format)
{
    switch (format) {
        case woogeen_base::FRAME_FORMAT_AAC:
        case woogeen_base::FRAME_FORMAT_AAC_48000_2:
            return true;
        default:
            return false;
    }
}

bool LiveStreamOut::isVideoFormatSupported(FrameFormat format)
{
    switch (format) {
        case woogeen_base::FRAME_FORMAT_H264:
            return true;
        default:
            return false;
    }
}

const char *LiveStreamOut::getFormatName(std::string& url)
{
    if (url.find("rtsp://") == 0)
        return "rtsp";
    else if (url.find("rtmp://") == 0)
        return "flv";
    else if (url.find("http://") == 0)
        return "hls";

    ELOG_ERROR("Invalid format for url(%s)", url.c_str());
    return NULL;
}

bool LiveStreamOut::getHeaderOpt(std::string& url, AVDictionary **options)
{
    // hls
    if (url.compare(0, 7, "http://") == 0) {
        std::string::size_type pos1 = url.rfind('/');
        if (pos1 == std::string::npos) {
            ELOG_ERROR("Cant not find base url %s", url.c_str());
            return false;
        }

        std::string::size_type pos2 = url.rfind('.');
        if (pos2 == std::string::npos) {
            ELOG_ERROR("Cant not find base url %s", url.c_str());
            return false;
        }

        if (pos2 <= pos1) {
            ELOG_ERROR("Cant not find base url %s", url.c_str());
            return false;
        }

        std::string segment_uri(url.substr(0, pos1));
        segment_uri.append("/intel_");
        segment_uri.append(url.substr(pos1 + 1, pos2 - pos1 - 1));
        segment_uri.append("_%09d.ts");

        ELOG_TRACE("index url %s", url.c_str());
        ELOG_TRACE("segment url %s", segment_uri.c_str());

        av_dict_set(options, "hls_segment_filename", segment_uri.c_str(), 0);
        av_dict_set(options, "hls_time", "10", 0);
        av_dict_set(options, "hls_list_size", "4", 0);
        av_dict_set(options, "hls_flags", "delete_segments", 0);
        av_dict_set(options, "method", "PUT", 0);
    }

    return true;
}

} /* namespace mcu */
