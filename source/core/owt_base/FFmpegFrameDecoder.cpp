// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "FFmpegFrameDecoder.h"

namespace owt_base {

DEFINE_LOGGER(FFmpegFrameDecoder, "owt.FFmpegFrameDecoder");

int FFmpegFrameDecoder::AVGetBuffer(AVCodecContext *s, AVFrame *frame, int flags)
{
    FFmpegFrameDecoder *FFmpegDecoder = static_cast<FFmpegFrameDecoder *>(s->opaque);
    int width = frame->width;
    int height = frame->height;

    avcodec_align_dimensions(s, &width, &height);

    rtc::scoped_refptr<webrtc::I420Buffer> frame_buffer = FFmpegDecoder->m_bufferManager->getFreeBuffer(width, height);
    if (!frame_buffer) {
        ELOG_ERROR("No free video buffer");
        return -1;
    }

    int y_size = width * height;
    int uv_size = ((width + 1) / 2) * ((height + 1) / 2);
    int total_size = y_size + 2 * uv_size;

    frame->format = s->pix_fmt;
    frame->reordered_opaque = s->reordered_opaque;

    frame->data[0]       = frame_buffer->MutableDataY();
    frame->linesize[0]   = frame_buffer->StrideY();
    frame->data[1]       = frame_buffer->MutableDataU();
    frame->linesize[1]   = frame_buffer->StrideU();
    frame->data[2]       = frame_buffer->MutableDataV();
    frame->linesize[2]   = frame_buffer->StrideV();

    frame->buf[0] = av_buffer_create(
            frame->data[0],
            total_size,
            AVFreeBuffer,
            static_cast<void*>(new webrtc::VideoFrame(frame_buffer,
                    0 /* timestamp */,
                    0 /* render_time_ms */,
                    webrtc::kVideoRotation_0)),
            0);

    return 0;
}

void FFmpegFrameDecoder::AVFreeBuffer(void* opaque, uint8_t* data)
{
    webrtc::VideoFrame* video_frame = static_cast<webrtc::VideoFrame*>(opaque);
    delete video_frame;

    return;
}

FFmpegFrameDecoder::FFmpegFrameDecoder()
    : m_decCtx(NULL)
    , m_decFrame(NULL)
{
}

FFmpegFrameDecoder::~FFmpegFrameDecoder()
{
    if (m_decFrame) {
        av_frame_free(&m_decFrame);
        m_decFrame = NULL;
    }

    if (m_decCtx) {
        avcodec_free_context(&m_decCtx);
        m_decCtx = NULL;
    }
}

bool FFmpegFrameDecoder::init(FrameFormat format)
{
    int ret = 0;
    AVCodecID codec_id = AV_CODEC_ID_NONE;
    AVCodec* dec = NULL;

    switch (format) {
        case FRAME_FORMAT_H264:
            codec_id = AV_CODEC_ID_H264;
            break;

        case FRAME_FORMAT_H265:
            codec_id = AV_CODEC_ID_H265;
            break;

        case FRAME_FORMAT_VP8:
            codec_id = AV_CODEC_ID_VP8;
            break;

        case FRAME_FORMAT_VP9:
            codec_id = AV_CODEC_ID_VP9;
            break;

        default:
            ELOG_ERROR_T("Unspported video frame format %s(%d)", getFormatStr(format), format);
            return false;
    }

    ELOG_DEBUG_T("Created %s decoder.", getFormatStr(format));

    dec = avcodec_find_decoder(codec_id);
    if (!dec) {
        ELOG_ERROR_T("Could not find ffmpeg decoder %s", avcodec_get_name(codec_id));
        return false;
    }

    m_decCtx = avcodec_alloc_context3(dec);
    if (!m_decCtx ) {
        ELOG_ERROR_T("Could not alloc ffmpeg decoder context");
        return false;
    }

    m_decCtx->get_buffer2 = AVGetBuffer;
    m_decCtx->opaque = this;
    ret = avcodec_open2(m_decCtx, dec , NULL);
    if (ret < 0) {
        ELOG_ERROR_T("Could not open ffmpeg decoder context, %s", ff_err2str(ret));
        return false;
    }

    m_decFrame = av_frame_alloc();
    if (!m_decFrame) {
        ELOG_ERROR_T("Could not allocate dec frame");
        return false;
    }

    memset(&m_packet, 0, sizeof(m_packet));

    m_bufferManager.reset(new I420BufferManager(50));

    return true;
}

void FFmpegFrameDecoder::onFrame(const Frame& frame)
{
    int ret;

    av_init_packet(&m_packet);
    m_packet.data = frame.payload;
    m_packet.size = frame.length;

    ret = avcodec_send_packet(m_decCtx, &m_packet);
    if (ret < 0) {
        ELOG_ERROR_T("Error while send packet, %s", ff_err2str(ret));
        return;
    }

    ret = avcodec_receive_frame(m_decCtx, m_decFrame);
    if (ret == AVERROR(EAGAIN)) {
        ELOG_DEBUG_T("Retry receive frame, %s", ff_err2str(ret));
        return;
    }else if (ret < 0) {
        ELOG_ERROR_T("Error while receive frame, %s", ff_err2str(ret));
        return;
    }

    webrtc::VideoFrame *video_frame = static_cast<webrtc::VideoFrame*>(
            av_buffer_get_opaque(m_decFrame->buf[0]));

    {
        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = FRAME_FORMAT_I420;
        frame.payload = reinterpret_cast<uint8_t*>(video_frame);
        frame.length = 0;
        frame.timeStamp = video_frame->timestamp();
        frame.additionalInfo.video.width = video_frame->width();
        frame.additionalInfo.video.height = video_frame->height();

        ELOG_TRACE_T("deliverFrame, %dx%d, timeStamp %d",
                frame.additionalInfo.video.width,
                frame.additionalInfo.video.height,
                frame.timeStamp);
        deliverFrame(frame);
    }
}

char *FFmpegFrameDecoder::ff_err2str(int errRet)
{
    av_strerror(errRet, (char*)(&m_errbuff), 500);
    return m_errbuff;
}

}//namespace owt_base
