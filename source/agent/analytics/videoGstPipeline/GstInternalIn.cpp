// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "GstInternalIn.h"
#include <gst/gst.h>

static void dump(void* index, uint8_t* buf, int len)
{
    char dumpFileName[128];

    snprintf(dumpFileName, 128, "/tmp/analyticsIn-%p", index);
    FILE* bsDumpfp = fopen(dumpFileName, "ab");
    if (bsDumpfp) {
        fwrite(buf, 1, len, bsDumpfp);
        fclose(bsDumpfp);
    }
}

static void mem_put_le16(uint8_t *mem, unsigned int val) {
    mem[0] = val;
    mem[1] = val >> 8;
}
 
static void mem_put_le32(uint8_t *mem, unsigned int val) {
    mem[0] = val;
    mem[1] = val >> 8;
    mem[2] = val >> 16;
    mem[3] = val >> 24;
}

DEFINE_LOGGER(GstInternalIn, "GstInternalIn");
GstInternalIn::GstInternalIn(GstAppSrc *data, unsigned int minPort, unsigned int maxPort, std::string ticket)
{
    m_transport.reset(new owt_base::RawTransport<owt_base::TCP>(this));

    m_transport->initTicket(ticket);
    if (minPort > 0 && minPort <= maxPort) {
        m_transport->listenTo(minPort, maxPort);
    } else {
        m_transport->listenTo(0);
    }
    appsrc = data;
    m_needKeyFrame = true;
    m_start = false;
    m_dumpIn = false;
    num_frames = 0;
    m_framerate = 30;
    char* pIn = std::getenv("DUMP_ANALYTICS_IN");
    if(pIn != NULL) {
        ELOG_INFO("Dump analytics in stream");
        m_dumpIn = true;
    }
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
    if (!m_start) {
        m_needKeyFrame = true;
    }
}

void GstInternalIn::setFramerate(int framerate) {
    m_framerate = framerate;
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
        return;
    }

    owt_base::Frame* frame = nullptr;
    switch (buf[0]) {
        case owt_base::TDT_MEDIA_FRAME:{
            frame = reinterpret_cast<owt_base::Frame*>(buf + 1);
	    if(frame->format > owt_base::FRAME_FORMAT_H265 || frame->format < owt_base::FRAME_FORMAT_VP8) {
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


            size_t allocLength = payloadLength + headerLength;
            uint8_t ivf_header[32] = {0};
            uint8_t ivf_frame_header[12] = {0};
            size_t ivf_header_length = 0;
            size_t ivf_frame_header_length = 0;

            if (frame->format == owt_base::FRAME_FORMAT_VP8) {
                if (num_frames == 0) {
                    ivf_header[0] = 'D';
                    ivf_header[1] = 'K';
                    ivf_header[2] = 'I';
                    ivf_header[3] = 'F';

                    mem_put_le16(ivf_header+4,  0);
                    mem_put_le16(ivf_header+6,  32);
                    ivf_header[8] = 'V';
                    ivf_header[9] = 'P';
                    ivf_header[10] = '8';
                    ivf_header[11] = '0';
                    mem_put_le16(ivf_header+12, frame->additionalInfo.video.width);
                    mem_put_le16(ivf_header+14, frame->additionalInfo.video.height);
                    mem_put_le32(ivf_header+16, m_framerate);
                    mem_put_le32(ivf_header+20, 1);
                    mem_put_le32(ivf_header+24, num_frames);
                    mem_put_le32(ivf_header+28, 0);
                    ivf_header_length = 32;
                }
                mem_put_le32(ivf_frame_header, payloadLength);
                mem_put_le32(ivf_frame_header+4, num_frames);
                ivf_frame_header_length = 12;
            }

            /* Create a new empty buffer */
            buffer = gst_buffer_new_and_alloc (payloadLength + ivf_header_length + ivf_frame_header_length);
            gst_buffer_map(buffer, &map, GST_MAP_WRITE);
            if (frame->format == owt_base::FRAME_FORMAT_VP8) {
                if (num_frames == 0) {
                    memcpy(map.data, ivf_header, ivf_header_length);
                }
                memcpy(map.data + ivf_header_length, ivf_frame_header, ivf_frame_header_length);
            }

            memcpy(map.data + ivf_header_length + ivf_frame_header_length, frame->payload, payloadLength);

            if(m_dumpIn) {
                dump(this, map.data, map.size);
            }


            gst_buffer_unmap(buffer, &map);
            g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);

            gst_buffer_unref(buffer);
            if (ret != GST_FLOW_OK) {
                /* We got some error, stop sending data */
                ELOG_DEBUG("Push buffer to appsrc got error\n");
                m_start=false;
            }
            num_frames++;

            break;
        }
        default:
            break;
    }
}

