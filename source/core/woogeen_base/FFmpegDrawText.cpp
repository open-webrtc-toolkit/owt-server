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

#include "FFmpegDrawText.h"

#include <libyuv/convert.h>
#include <libyuv/planar_functions.h>
#include <libyuv/scale.h>

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(FFmpegDrawText, "woogeen.FFmpegDrawText");

FFmpegDrawText::FFmpegDrawText()
    : m_filter_graph(NULL)
    , m_buffersink_ctx(NULL)
    , m_buffersrc_ctx(NULL)
    , m_filter_inputs(NULL)
    , m_filter_outputs(NULL)
    , m_input_frame(NULL)
    , m_filt_frame(NULL)
    , m_width(0)
    , m_height(0)
    , m_reconfigured(false)
    , m_validConfig(false)
    , m_enabled(false)
{
}

FFmpegDrawText::~FFmpegDrawText()
{
    deinit();
}

bool FFmpegDrawText::init(int width, int height)
{
    int ret = -1;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    char src_args[512];
    char default_desc[] = "drawtext=fontfile=/usr/share/fonts/gnu-free/FreeSerif.ttf:text=''"; //centos

    ELOG_TRACE_T("init: %s", default_desc);

    m_filter_outputs = avfilter_inout_alloc();
    m_filter_inputs  = avfilter_inout_alloc();

    m_filter_graph = avfilter_graph_alloc();
    if (!m_filter_outputs || !m_filter_inputs || !m_filter_graph) {
        ELOG_ERROR_T("Cannot alloc filter resource");
        goto end;
    }

    snprintf(src_args, sizeof(src_args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",
            width, height, AV_PIX_FMT_YUV420P,
            1, 1
            );
    ret = avfilter_graph_create_filter(&m_buffersrc_ctx, buffersrc, "in",
            src_args, NULL, m_filter_graph);
    if (ret < 0) {
        ELOG_ERROR_T("Cannot create buffer source");
        goto end;
    }

    ret = avfilter_graph_create_filter(&m_buffersink_ctx, buffersink, "out",
            NULL, NULL, m_filter_graph);
    if (ret < 0) {
        ELOG_ERROR_T("Cannot create buffer sink");
        goto end;
    }

    ret = av_opt_set_int_list(m_buffersink_ctx, "pix_fmts", pix_fmts,
            AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        ELOG_ERROR_T("Cannot set output pixel format");
        goto end;
    }

    m_filter_outputs->name       = av_strdup("in");
    m_filter_outputs->filter_ctx = m_buffersrc_ctx;
    m_filter_outputs->pad_idx    = 0;
    m_filter_outputs->next       = NULL;

    m_filter_inputs->name       = av_strdup("out");
    m_filter_inputs->filter_ctx = m_buffersink_ctx;
    m_filter_inputs->pad_idx    = 0;
    m_filter_inputs->next       = NULL;

    ret = avfilter_graph_parse_ptr(m_filter_graph, default_desc,
            &m_filter_inputs, &m_filter_outputs, NULL);
    if (ret < 0) {
        ELOG_ERROR_T("Cannot parse graph config: %s", default_desc);
        goto end;
    }

    ret = avfilter_graph_config(m_filter_graph, NULL);
    if (ret < 0) {
        ELOG_ERROR_T("Cannot set graph config");
        goto end;
    }

    m_input_frame = av_frame_alloc();
    m_filt_frame = av_frame_alloc();
    if (!m_input_frame || !m_filt_frame) {
        ELOG_ERROR_T("Could not allocate av frames");
        goto end;
    }

    m_input_frame->format = AV_PIX_FMT_YUV420P;
    m_input_frame->width  = m_width;
    m_input_frame->height = m_height;
    ret = av_frame_get_buffer(m_input_frame, 32);
    if (ret < 0) {
        ELOG_ERROR_T("Could not get  av frame buffer");
        goto end;
    }

    return true;

end:
    if (m_input_frame) {
        av_frame_free(&m_input_frame);
        m_input_frame = NULL;
    }

    if (m_filt_frame) {
        av_frame_free(&m_filt_frame);
        m_filt_frame = NULL;
    }

    if (m_filter_inputs) {
        avfilter_inout_free(&m_filter_inputs);
        m_filter_inputs = NULL;
    }

    if (m_filter_outputs) {
        avfilter_inout_free(&m_filter_outputs);
        m_filter_outputs = NULL;
    }

    if (m_filter_graph) {
        avfilter_graph_free(&m_filter_graph);
        m_filter_graph = NULL;
    }

    m_buffersrc_ctx = NULL;
    m_buffersink_ctx = NULL;

    return false;
}

void FFmpegDrawText::deinit()
{
    if (m_input_frame) {
        av_frame_free(&m_input_frame);
        m_input_frame = NULL;
    }

    if (m_filt_frame) {
        av_frame_free(&m_filt_frame);
        m_filt_frame = NULL;
    }

    if (m_filter_inputs) {
        avfilter_inout_free(&m_filter_inputs);
        m_filter_inputs = NULL;
    }

    if (m_filter_outputs) {
        avfilter_inout_free(&m_filter_outputs);
        m_filter_outputs = NULL;
    }

    if (m_filter_graph) {
        avfilter_graph_free(&m_filter_graph);
        m_filter_graph = NULL;
    }

    m_buffersrc_ctx = NULL;
    m_buffersink_ctx = NULL;
}

int FFmpegDrawText::configure(std::string arg)
{
    int ret;

    ELOG_INFO_T("config: %s", arg.c_str());

    if (!m_filter_graph) {
        ELOG_TRACE_T("Invalid filter graph");
        return 0;
    }

    ret = avfilter_graph_send_command(m_filter_graph, "drawtext", "reinit", arg.c_str(), NULL, 0, 0);
    if (ret < 0) {
        ELOG_ERROR_T("Cannot configure filter: %s", arg.c_str());
        return 0;
    }

    return 1;
}

int FFmpegDrawText::setText(std::string arg)
{
    m_filter_desc = arg;
    m_reconfigured = true;

    return 1;
}

int FFmpegDrawText::drawFrame(Frame& frame)
{
    return 0;//disable

    int ret;

    switch (frame.format) {
        case FRAME_FORMAT_I420:
            break;

        default:
            ELOG_TRACE_T("Unspported video frame format: %s", getFormatStr(frame.format));
            return 0;
    }

    if (frame.additionalInfo.video.width == 0
        || frame.additionalInfo.video.height == 0) {
        ELOG_ERROR_T("Invalid size: %dx%d",
                frame.additionalInfo.video.width,
                frame.additionalInfo.video.height
                );
        return 0;
    }

    if (m_width == 0 || m_height == 0) {
        m_width = frame.additionalInfo.video.width;
        m_height = frame.additionalInfo.video.height;

        init(m_width, m_height);
    }

    if (m_width != frame.additionalInfo.video.width
            || m_height != frame.additionalInfo.video.height) {
        ELOG_DEBUG_T("re-config size: %dx%d -> %dx%d",
                m_width, m_height,
                frame.additionalInfo.video.width,
                frame.additionalInfo.video.height);

        m_width = frame.additionalInfo.video.width;
        m_height = frame.additionalInfo.video.height;
    }

    if (m_reconfigured) {
        if (configure(m_filter_desc))
            m_validConfig = true;
        else {
            m_validConfig = false;

            deinit();
            init(m_width, m_height);
        }

        m_reconfigured = false;
    }

    if (!m_enabled || !m_validConfig) {
        return 1;
    }

    if (!m_filter_graph) {
        ELOG_TRACE_T("filter graph not ready!");
        return 0;
    }

    ELOG_TRACE_T("do drawFrame");

    if (!copyFrame(m_input_frame, frame)) {
        return 0;
    }

    if (av_buffersrc_add_frame_flags(m_buffersrc_ctx, m_input_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        ELOG_ERROR_T("Error while feeding the filtergraph");
        return 0;
    }

    ret = av_buffersink_get_frame(m_buffersink_ctx, m_filt_frame);
    if (ret < 0) {
        ELOG_ERROR_T("av_buffersink_get_frame error");
        return 0;
    }

    if (!copyFrame(frame, m_filt_frame)) {
        return 0;
    }

    av_frame_unref(m_filt_frame);

    return 1;
}

int FFmpegDrawText::copyFrame(AVFrame *dstAVFrame, Frame &srcFrame)
{
    int ret;
    VideoFrame *videoFrame = reinterpret_cast<VideoFrame*>(srcFrame.payload);
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> i420Buffer = videoFrame->video_frame_buffer();

    if (i420Buffer->width() == dstAVFrame->width && i420Buffer->height() == dstAVFrame->height) {
        ret = libyuv::I420Copy(
                i420Buffer->DataY(), i420Buffer->StrideY(),
                i420Buffer->DataU(), i420Buffer->StrideU(),
                i420Buffer->DataV(), i420Buffer->StrideV(),
                dstAVFrame->data[0], dstAVFrame->linesize[0],
                dstAVFrame->data[1], dstAVFrame->linesize[1],
                dstAVFrame->data[2], dstAVFrame->linesize[2],
                i420Buffer->width(), i420Buffer->height());
        if (ret != 0) {
            ELOG_ERROR_T("libyuv::I420Copy failed(%d)", ret);
            return false;
        }
    } else {
        ret = libyuv::I420Scale(
                i420Buffer->DataY(), i420Buffer->StrideY(),
                i420Buffer->DataU(), i420Buffer->StrideU(),
                i420Buffer->DataV(), i420Buffer->StrideV(),
                i420Buffer->width(), i420Buffer->height(),
                dstAVFrame->data[0], dstAVFrame->linesize[0],
                dstAVFrame->data[1], dstAVFrame->linesize[1],
                dstAVFrame->data[2], dstAVFrame->linesize[2],
                dstAVFrame->width,   dstAVFrame->height,
                libyuv::kFilterBox);
        if (ret != 0) {
            ELOG_ERROR_T("libyuv::I420Scale failed(%d)", ret);
            return false;
        }
    }

    return true;
}

int FFmpegDrawText::copyFrame(Frame &dstFrame, AVFrame *srcAVFrame)
{
    int ret;
    VideoFrame *videoFrame = reinterpret_cast<VideoFrame*>(dstFrame.payload);
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> i420Buffer = videoFrame->video_frame_buffer();

    if (i420Buffer->width() == srcAVFrame->width && i420Buffer->height() == srcAVFrame->height) {
        ret = libyuv::I420Copy(
                srcAVFrame->data[0], srcAVFrame->linesize[0],
                srcAVFrame->data[1], srcAVFrame->linesize[1],
                srcAVFrame->data[2], srcAVFrame->linesize[2],
                const_cast< uint8_t*>(i420Buffer->DataY()), i420Buffer->StrideY(),
                const_cast< uint8_t*>(i420Buffer->DataU()), i420Buffer->StrideU(),
                const_cast< uint8_t*>(i420Buffer->DataV()), i420Buffer->StrideV(),
                i420Buffer->width(), i420Buffer->height());
        if (ret != 0) {
            ELOG_ERROR_T("libyuv::I420Copy failed(%d)", ret);
            return false;
        }
    } else {
        ret = libyuv::I420Scale(
                srcAVFrame->data[0], srcAVFrame->linesize[0],
                srcAVFrame->data[1], srcAVFrame->linesize[1],
                srcAVFrame->data[2], srcAVFrame->linesize[2],
                srcAVFrame->width, srcAVFrame->height,
                const_cast< uint8_t*>(i420Buffer->DataY()), i420Buffer->StrideY(),
                const_cast< uint8_t*>(i420Buffer->DataU()), i420Buffer->StrideU(),
                const_cast< uint8_t*>(i420Buffer->DataV()), i420Buffer->StrideV(),
                i420Buffer->width(), i420Buffer->height(),
                libyuv::kFilterBox);
        if (ret != 0) {
            ELOG_ERROR_T("libyuv::I420Scale failed(%d)", ret);
            return false;
        }
    }

    return true;
}

char *FFmpegDrawText::ff_err2str(int errRet)
{
    av_strerror(errRet, (char*)(&m_errbuff), 500);
    return m_errbuff;
}

}//namespace woogeen_base
