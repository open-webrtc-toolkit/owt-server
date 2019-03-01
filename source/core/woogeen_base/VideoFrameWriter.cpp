// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoFrameWriter.h"

#ifdef ENABLE_MSDK
#include "MsdkFrame.h"
#endif

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(VideoFrameWriter, "woogeen.VideoFrameWriter");

VideoFrameWriter::VideoFrameWriter(const std::string& name)
    : m_name(name)
    , m_fp(NULL)
    , m_index(0)
    , m_width(0)
    , m_height(0)
{
}

VideoFrameWriter::~VideoFrameWriter()
{
    if (m_fp) {
        fclose(m_fp);
        m_fp = NULL;
    }
}

FILE *VideoFrameWriter::getVideoFp(webrtc::VideoFrameBuffer *i420Buffer)
{
    if (m_width != i420Buffer->width() || m_height != i420Buffer->height()) {
        if (m_fp) {
            fclose(m_fp);
            m_fp = NULL;
        }

        m_index++;

        char fileName[128];
        snprintf(fileName, 128, "/tmp/%s-%d-%dx%d.i420", m_name.c_str(), m_index, i420Buffer->width(), i420Buffer->height());
        m_fp = fopen(fileName, "wb");
        if (!m_fp) {
            ELOG_ERROR_T("Can not open dump file, %s", fileName);
            return NULL;
        }

        ELOG_INFO_T("Dump: %s", fileName);

        m_width = i420Buffer->width();
        m_height = i420Buffer->height();
    }

    return m_fp;
}

void VideoFrameWriter::write(webrtc::VideoFrameBuffer *videoFrameBuffer)
{
    if (!videoFrameBuffer) {
        ELOG_ERROR_T("NULL pointer");
        return;
    }

    if (videoFrameBuffer->width() == 0 || videoFrameBuffer->height() == 0) {
        ELOG_ERROR_T("Invalid size(%dx%d)", videoFrameBuffer->width(), videoFrameBuffer->height());
        return;
    }

    FILE *fp = getVideoFp(videoFrameBuffer);
    if (!fp) {
        ELOG_ERROR_T("NULL fp");
        return;
    }

    if (videoFrameBuffer->StrideY() != videoFrameBuffer->width()
            || videoFrameBuffer->StrideU() * 2!= videoFrameBuffer->width()
            || videoFrameBuffer->StrideV() * 2!= videoFrameBuffer->width()
            ) {
        ELOG_ERROR_T("Invalid strideY(%d), strideU(%d), strideV(%d), width(%d)\n"
                , videoFrameBuffer->StrideY()
                , videoFrameBuffer->StrideU()
                , videoFrameBuffer->StrideV()
                , videoFrameBuffer->width()
              );
        return;
    }

    fwrite(videoFrameBuffer->DataY(), 1, videoFrameBuffer->height() * videoFrameBuffer->StrideY() , fp);
    fwrite(videoFrameBuffer->DataU(), 1, videoFrameBuffer->height() * videoFrameBuffer->StrideU() / 2, fp);
    fwrite(videoFrameBuffer->DataV(), 1, videoFrameBuffer->height() * videoFrameBuffer->StrideV() / 2, fp);
}

void VideoFrameWriter::write(const Frame& frame)
{
    switch (frame.format) {
        case FRAME_FORMAT_I420:
            {
                VideoFrame *inputFrame = reinterpret_cast<VideoFrame*>(frame.payload);
                rtc::scoped_refptr<webrtc::VideoFrameBuffer> inputBuffer = inputFrame->video_frame_buffer();

                write(inputBuffer);
            }
            break;

#ifdef ENABLE_MSDK
        case FRAME_FORMAT_MSDK:
            {
                MsdkFrameHolder *holder = (MsdkFrameHolder *)frame.payload;
                boost::shared_ptr<MsdkFrame> msdkFrame = holder->frame;
                rtc::scoped_refptr<webrtc::I420Buffer> inputBuffer;

                if (!m_bufferManager)
                    m_bufferManager.reset(new I420BufferManager(1));

                inputBuffer = m_bufferManager->getFreeBuffer(msdkFrame->getVideoWidth(), msdkFrame->getVideoHeight());

                msdkFrame->convertTo(inputBuffer);

                write(inputBuffer);
            }
            break;
#endif

        default:
            assert(false);
            return;
    }
}

} /* namespace mcu */
