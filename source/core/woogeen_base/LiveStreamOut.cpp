// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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
    close();
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
    else if (url.find("dash://") == 0)
        return "dash";

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

    } else if (url.compare(0, 7, "dash://") == 0) {

        std::string::size_type last_slash = url.rfind('/');
        if(last_slash == std::string::npos) {
            ELOG_ERROR("Unexpected format of %s", url.c_str());
            return false;
        }
        std::string::size_type last_dot = url.rfind('.');
        if(last_dot == std::string::npos || last_dot < last_slash) {
            last_dot = url.length() - 1;
        }
        std::string dash_uri(url.substr(last_slash + 1, last_dot - last_slash - 1));

        std::string init_seg_uri(dash_uri);
        init_seg_uri.append("-init-stream$RepresentationID$.m4s");

        std::string media_seg_uri(dash_uri);
        media_seg_uri.append("-chunk-stream$RepresentationID$-$Number%05d$.m4s");

        av_dict_set(options, "init_seg_name", init_seg_uri.c_str(), 0);
        av_dict_set(options, "media_seg_name", media_seg_uri.c_str(), 0);

        av_dict_set(options, "streaming", "1", 0);
        av_dict_set(options, "use_template", "1", 0);
        av_dict_set(options, "use_timeline", "1", 0);
        av_dict_set(options, "seg_duration", "1"/*seconds*/, 0);
        av_dict_set(options, "dash_segment_type", "mp4", 0);
        av_dict_set(options, "window_size", "3", 0);
        av_dict_set(options, "extra_window_size", "0", 0);
        av_dict_set(options, "remove_at_exit", "1", 0);
    }

    return true;
}

} /* namespace mcu */
