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

