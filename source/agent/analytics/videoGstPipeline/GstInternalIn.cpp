// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "GstInternalIn.h"
#include <gst/gst.h>
#include <stdio.h>


DEFINE_LOGGER(GstInternalIn, "GstInternalIn");
GstInternalIn::GstInternalIn(GstAppSrc *data, unsigned int minPort, unsigned int maxPort)
{
    m_transport.reset(new owt_base::RawTransport<owt_base::TCP>(this));

    if (minPort > 0 && minPort <= maxPort) {
        m_transport->listenTo(minPort, maxPort);
    } else {
        m_transport->listenTo(0);
    }
    appsrc = data;
    m_needKeyFrame = true;
    m_start = false;

}

GstInternalIn::~GstInternalIn()
{
    m_transport->close();
}

unsigned int GstInternalIn::getListeningPort()
{
    return m_transport->getListeningPort();
}

void GstInternalIn::setPushData(bool status){
    m_start = status;
}

void GstInternalIn::onFeedback(const owt_base::FeedbackMsg& msg)
{
    char sendBuffer[512];
    sendBuffer[0] = owt_base::TDT_FEEDBACK_MSG;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<owt_base::FeedbackMsg*>(&msg)), sizeof(owt_base::FeedbackMsg));
    m_transport->sendData((char*)sendBuffer, sizeof(owt_base::FeedbackMsg) + 1);
}

void GstInternalIn::onTransportData(char* buf, int len)
{
    if(!m_start) {
        ELOG_INFO("Not start yet, stop pushing data to appsrc\n");
        pthread_t tid;
        tid = pthread_self();
        return;
    }

    owt_base::Frame* frame = nullptr;
    switch (buf[0]) {
        case owt_base::TDT_MEDIA_FRAME:{
            frame = reinterpret_cast<owt_base::Frame*>(buf + 1);
            if(frame->additionalInfo.video.width == 1) {
                ELOG_DEBUG("Not a valid video frame\n");
                break;
            }
            frame->payload = reinterpret_cast<uint8_t*>(buf + 1 + sizeof(owt_base::Frame));
            size_t payloadLength       = frame->length;
            size_t headerLength       = sizeof(frame);

            GstBuffer *buffer;
            GstFlowReturn ret;
            GstMapInfo map;


            if (m_needKeyFrame) {
                if (frame->additionalInfo.video.isKeyFrame) {
                    m_needKeyFrame = false;
                } else {
                    ELOG_DEBUG("Request key frame\n");
                    owt_base::FeedbackMsg msg {.type = owt_base::VIDEO_FEEDBACK, .cmd = owt_base::REQUEST_KEY_FRAME};
                    onFeedback(msg);
                    return;
                }
            }

            /* Create a new empty buffer */
            buffer = gst_buffer_new_and_alloc (payloadLength + headerLength);
            gst_buffer_map(buffer, &map, GST_MAP_WRITE);
            memcpy(map.data, frame, headerLength);
            memcpy(map.data + headerLength, frame->payload, payloadLength);

            gst_buffer_unmap(buffer, &map);
            g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);

            gst_buffer_unref(buffer);
            if (ret != GST_FLOW_OK) {
                /* We got some error, stop sending data */
                ELOG_DEBUG("Push buffer to appsrc got error\n");
                m_start=false;
            }

            break;
        }
        default:
            break;
    }
}

