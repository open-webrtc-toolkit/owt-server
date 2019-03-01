// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FFmpegDrawText_h
#define FFmpegDrawText_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>

#include <webrtc/api/video/video_frame.h>
#include <webrtc/api/video/i420_buffer.h>

#include "MediaFramePipeline.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
}

namespace woogeen_base {

class FFmpegDrawText {
    DECLARE_LOGGER();

public:
    FFmpegDrawText();
    ~FFmpegDrawText();

    int drawFrame(Frame&);
    int setText(std::string arg);
    void enable(bool enabled) {m_enabled = enabled;}

protected:
    bool init(int width, int height);
    int configure(std::string arg);
    void deinit();

    int copyFrame(AVFrame *dstAVFrame, Frame &srcFrame);
    int copyFrame(Frame &dstFrame, AVFrame *srcAVFrame);

private:
    AVFilterGraph *m_filter_graph;
    AVFilterContext *m_buffersink_ctx;
    AVFilterContext *m_buffersrc_ctx;

    AVFilterInOut *m_filter_inputs;
    AVFilterInOut *m_filter_outputs;

    AVFrame *m_input_frame;
    AVFrame *m_filt_frame;

    int m_width;
    int m_height;

    std::string m_filter_desc;
    bool m_reconfigured;
    bool m_validConfig;

    bool m_enabled;

    char m_errbuff[500];
    char *ff_err2str(int errRet);
};

} /* namespace woogeen_base */

#endif /* FFmpegDrawText_h */

