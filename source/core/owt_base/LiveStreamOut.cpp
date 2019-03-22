// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "LiveStreamOut.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avstring.h>
}

namespace owt_base {

DEFINE_LOGGER(LiveStreamOut, "owt.LiveStreamOut");

LiveStreamOut::LiveStreamOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int streamingTimeout, StreamingOptions& options)
    : AVStreamOut(url, hasAudio, hasVideo, handle, streamingTimeout)
    , m_options(options)
{
    switch(m_options.format) {
        case STREAMING_FORMAT_RTSP:
            ELOG_DEBUG("format %s", "rtsp");
            break;

        case STREAMING_FORMAT_RTMP:
            ELOG_DEBUG("format %s", "rtmp");
            break;

        case STREAMING_FORMAT_HLS:
            ELOG_DEBUG("format %s, hls_time %d, hls_list_size %d"
                    , "hls"
                    , m_options.hls_time
                    , m_options.hls_list_size
                    );
            break;

        case STREAMING_FORMAT_DASH:
            ELOG_DEBUG("format %s, dash_seg_duration %d, dash_window_size %d"
                    , "dash"
                    , m_options.dash_seg_duration
                    , m_options.dash_window_size
                    );
            break;

        default:
            ELOG_ERROR("Invalid streaming format");
            break;
    }
}

LiveStreamOut::~LiveStreamOut()
{
    close();
}

bool LiveStreamOut::isAudioFormatSupported(FrameFormat format)
{
    switch (format) {
        case owt_base::FRAME_FORMAT_AAC:
        case owt_base::FRAME_FORMAT_AAC_48000_2:
            return true;
        default:
            return false;
    }
}

bool LiveStreamOut::isVideoFormatSupported(FrameFormat format)
{
    switch (format) {
        case owt_base::FRAME_FORMAT_H264:
            return true;
        default:
            return false;
    }
}

const char *LiveStreamOut::getFormatName(std::string& url)
{
    switch(m_options.format) {
        case STREAMING_FORMAT_RTSP:
            return "rtsp";
        case STREAMING_FORMAT_RTMP:
            return "flv";
        case STREAMING_FORMAT_HLS:
            return "hls";
        case STREAMING_FORMAT_DASH:
            return "dash";
        default:
            ELOG_ERROR("Invalid format for url(%s)", url.c_str());
            return NULL;
    }
}

bool LiveStreamOut::getHeaderOpt(std::string& url, AVDictionary **options)
{
    if (m_options.format == STREAMING_FORMAT_HLS) {
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
        segment_uri.append("/");
        segment_uri.append(url.substr(pos1 + 1, pos2 - pos1 - 1));
        segment_uri.append("_%05d.ts");

        ELOG_DEBUG("index url %s", url.c_str());
        ELOG_DEBUG("segment url %s", segment_uri.c_str());
        av_dict_set(options, "hls_segment_filename", segment_uri.c_str(), 0);

        av_dict_set(options, "hls_flags", "delete_segments", 0);

        av_dict_set_int(options, "hls_time", m_options.hls_time, 0);
        av_dict_set_int(options, "hls_list_size", m_options.hls_list_size, 0);
    } else if (m_options.format == STREAMING_FORMAT_DASH) {
        std::string::size_type last_slash = url.rfind('/');
        if(last_slash == std::string::npos) {
            ELOG_ERROR("Unexpected format of %s", url.c_str());
            return false;
        }
        std::string::size_type last_dot = url.rfind('.');
        if(last_dot == std::string::npos || last_dot < last_slash) {
            last_dot = url.length() - 1;
        }
        std::string basename(url.substr(last_slash + 1, last_dot - last_slash - 1));

        std::string init_seg_uri(basename);
        init_seg_uri.append("-init-stream$RepresentationID$.m4s");

        std::string media_seg_uri(basename);
        media_seg_uri.append("-chunk-stream$RepresentationID$-$Number%05d$.m4s");

        av_dict_set(options, "init_seg_name", init_seg_uri.c_str(), 0);
        av_dict_set(options, "media_seg_name", media_seg_uri.c_str(), 0);

        ELOG_DEBUG("init_seg_uri: %s", init_seg_uri.c_str());
        ELOG_DEBUG("media_seg_uri: %s", media_seg_uri.c_str());

        av_dict_set(options, "streaming", "1", 0);
        av_dict_set(options, "use_template", "1", 0);
        av_dict_set(options, "use_timeline", "1", 0);
        av_dict_set(options, "dash_segment_type", "mp4", 0);
        av_dict_set(options, "remove_at_exit", "1", 0);

        av_dict_set_int(options, "seg_duration", m_options.dash_seg_duration, 0);
        av_dict_set_int(options, "window_size", m_options.dash_window_size, 0);
        av_dict_set_int(options, "extra_window_size", m_options.dash_window_size * 2, 0);
    }

    return true;
}

} /* namespace mcu */
