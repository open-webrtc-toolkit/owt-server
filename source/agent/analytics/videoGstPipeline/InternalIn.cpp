// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalIn.h"
#include <gst/gst.h>
#include <stdio.h>

using namespace std;

DEFINE_LOGGER(InternalIn, "InternalIn");
InternalIn::InternalIn(GstAppSrc *data, unsigned int minPort, unsigned int maxPort)
{
    ELOG_INFO(">>>>>>>>>>>>>>Internal in constructor\n");
    m_transport.reset(new owt_base::RawTransport<TCP>(this));

    if (minPort > 0 && minPort <= maxPort) {
        m_transport->listenTo(minPort, maxPort);
    } else {
        m_transport->listenTo(0);
    }
     ELOG_INFO(">>>>>>>>>>>>>>Listen in constructor\n");
    appsrc = data;
    m_needKeyFrame = true;
    m_start = false;

}

InternalIn::~InternalIn()
{
    m_transport->close();
}

unsigned int InternalIn::getListeningPort()
{
    return m_transport->getListeningPort();
}

void InternalIn::setPushData(bool status){
    m_start = status;
}

void InternalIn::onFeedback(const FeedbackMsg& msg)
{
    char sendBuffer[512];
    sendBuffer[0] = TDT_FEEDBACK_MSG;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<FeedbackMsg*>(&msg)), sizeof(FeedbackMsg));
    m_transport->sendData((char*)sendBuffer, sizeof(FeedbackMsg) + 1);
}

void InternalIn::onTransportData(char* buf, int len)
{
    if(!m_start) {
        ELOG_DEBUG(">>>>>>>>>>>>>>Stop pushing data to appsrc\n");
        pthread_t tid;
        tid = pthread_self();
        ELOG_DEBUG("*****internal in  is in thread tid: %u (0x%x)\n", (unsigned int)tid, (unsigned int)tid);
        return;
    }

    Frame* frame = nullptr;
    switch (buf[0]) {
        case TDT_MEDIA_FRAME:{
            frame = reinterpret_cast<Frame*>(buf + 1);
            if(frame->additionalInfo.video.width == 1) {
                ELOG_DEBUG("============Not a valid video frame\n");
                break;
            }
            frame->payload = reinterpret_cast<uint8_t*>(buf + 1 + sizeof(Frame));
            size_t payloadLength       = frame->length;
            size_t headerLength       = sizeof(frame);

            GstBuffer *buffer;
            GstFlowReturn ret;
            GstMapInfo map;


            if (m_needKeyFrame) {
                if (frame->additionalInfo.video.isKeyFrame) {
                    m_needKeyFrame = false;
                } else {
                    ELOG_DEBUG("============Request key frame\n");
                    FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
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

            ELOG_DEBUG("============Push buffer to appsrc with length: %d %p\n",(payloadLength + headerLength), this);

            gst_buffer_unref(buffer);
            if (ret != GST_FLOW_OK) {
                /* We got some error, stop sending data */
                ELOG_DEBUG("============Push buffer to appsrc got error\n");
                m_start=false;
            }

            break;
        }
        default:
            break;
    }
}

