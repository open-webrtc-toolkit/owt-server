// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "FrameConverter.h"

#include <libyuv/convert.h>
#include <libyuv/planar_functions.h>
#include <libyuv/scale.h>

namespace woogeen_base {

DEFINE_LOGGER(FrameConverter, "woogeen.FrameConverter");

FrameConverter::FrameConverter(bool useMsdkVpp)
{
    m_bufferManager.reset(new I420BufferManager(2));

#ifdef ENABLE_MSDK
    if (useMsdkVpp)
        m_scaler.reset(new MsdkScaler());
#endif
}

FrameConverter::~FrameConverter()
{
}

#ifdef ENABLE_MSDK
bool FrameConverter::convert(MsdkFrame *srcMsdkFrame, webrtc::I420Buffer *dstI420Buffer)
{
    if (srcMsdkFrame->getVideoWidth() == (uint32_t)dstI420Buffer->width()
            && srcMsdkFrame->getVideoHeight() == (uint32_t)dstI420Buffer->height()) {
        if (!srcMsdkFrame->convertTo(dstI420Buffer)) {
            ELOG_ERROR("Fail to convert msdkFrame to i420Buffer");
            return false;
        }
    } else { // scale
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer = m_bufferManager->getFreeBuffer(srcMsdkFrame->getVideoWidth(), srcMsdkFrame->getVideoHeight());
        if (!i420Buffer) {
            ELOG_ERROR("No valid i420Buffer");
            return false;
        }

        if (!srcMsdkFrame->convertTo(i420Buffer)) {
            ELOG_ERROR("Fail to convert msdkFrame to i420Buffer");
            return false;
        }

        if (!convert(i420Buffer, dstI420Buffer)) {
            ELOG_ERROR("Fail to convert i420Buffer to i420Buffer");
            return false;
        }
    }

    return true;
}

bool FrameConverter::convert(MsdkFrame *srcMsdkFrame, MsdkFrame *dstMsdkFrame)
{
    if (m_scaler)
        return m_scaler->convert(srcMsdkFrame, dstMsdkFrame);
    else {
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer = m_bufferManager->getFreeBuffer(srcMsdkFrame->getVideoWidth(), srcMsdkFrame->getVideoHeight());
        if (!i420Buffer) {
            ELOG_ERROR("No valid i420Buffer");
            return false;
        }

        if (!srcMsdkFrame->convertTo(i420Buffer)) {
            ELOG_ERROR("Fail to convert msdkFrame to i420Buffer");
            return false;
        }

        if (!convert(i420Buffer, dstMsdkFrame)) {
            ELOG_ERROR("Fail to convert i420Buffer to msdkFrame");
            return false;
        }

        return true;
    }
}

bool FrameConverter::convert(webrtc::VideoFrameBuffer *srcBuffer, MsdkFrame *dstMsdkFrame)
{
    if ((uint32_t)srcBuffer->width() == dstMsdkFrame->getVideoWidth() && (uint32_t)srcBuffer->height() == dstMsdkFrame->getVideoHeight()) {
        if (!dstMsdkFrame->convertFrom(srcBuffer)) {
            ELOG_ERROR("Fail to convert i420Buffer to msdkFrame");
            return false;
        }
    } else { // scale
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer = m_bufferManager->getFreeBuffer(dstMsdkFrame->getVideoWidth(), dstMsdkFrame->getVideoHeight());
        if (!i420Buffer) {
            ELOG_ERROR("No valid i420Buffer");
            return false;
        }

        if (!convert(srcBuffer, i420Buffer)) {
            ELOG_ERROR("Fail to convert i420Buffer to i420Buffer");
            return false;
        }

        if (!dstMsdkFrame->convertFrom(i420Buffer)) {
            ELOG_ERROR("Fail to convert i420Buffer to msdkFrame");
            return false;
        }
    }

    return true;
}
#endif

bool FrameConverter::convert(webrtc::VideoFrameBuffer *srcBuffer, webrtc::I420Buffer *dstI420Buffer)
{
    int ret;

    if (srcBuffer->width() == dstI420Buffer->width() && srcBuffer->height() == dstI420Buffer->height()) {
        ret = libyuv::I420Copy(
                srcBuffer->DataY(), srcBuffer->StrideY(),
                srcBuffer->DataU(), srcBuffer->StrideU(),
                srcBuffer->DataV(), srcBuffer->StrideV(),
                dstI420Buffer->MutableDataY(), dstI420Buffer->StrideY(),
                dstI420Buffer->MutableDataU(), dstI420Buffer->StrideU(),
                dstI420Buffer->MutableDataV(), dstI420Buffer->StrideV(),
                dstI420Buffer->width(),        dstI420Buffer->height());
        if (ret != 0) {
            ELOG_ERROR("libyuv::I420Copy failed(%d)", ret);
            return false;
        }
    } else { // scale
        ret = libyuv::I420Scale(
                srcBuffer->DataY(),   srcBuffer->StrideY(),
                srcBuffer->DataU(),   srcBuffer->StrideU(),
                srcBuffer->DataV(),   srcBuffer->StrideV(),
                srcBuffer->width(),   srcBuffer->height(),
                dstI420Buffer->MutableDataY(),  dstI420Buffer->StrideY(),
                dstI420Buffer->MutableDataU(),  dstI420Buffer->StrideU(),
                dstI420Buffer->MutableDataV(),  dstI420Buffer->StrideV(),
                dstI420Buffer->width(),         dstI420Buffer->height(),
                libyuv::kFilterBox);
        if (ret != 0) {
            ELOG_ERROR("libyuv::I420Scale failed(%d)", ret);
            return false;
        }
    }

    return true;
}

}//namespace woogeen_base
