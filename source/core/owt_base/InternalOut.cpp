// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalOut.h"

namespace owt_base {

InternalOut::InternalOut(const std::string& protocol, const std::string& dest_ip, unsigned int dest_port)
{
    if (protocol == "tcp")
        m_transport.reset(new owt_base::RawTransport<TCP>(this));
    else
        m_transport.reset(new owt_base::RawTransport<UDP>(this));

    m_transport->createConnection(dest_ip, dest_port);
#ifdef BUILD_FOR_GST_ANALYTICS
    m_frameCount = 0;
    encoder_pad = NULL;
#endif
}

InternalOut::~InternalOut()
{
    m_transport->close();
}

void InternalOut::onFrame(const Frame& frame)
{
    char sendBuffer[sizeof(Frame) + 1];
    size_t header_len = sizeof(Frame);

    sendBuffer[0] = TDT_MEDIA_FRAME;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<Frame*>(&frame)), header_len);
    m_transport->sendData(sendBuffer, header_len + 1, reinterpret_cast<char*>(const_cast<uint8_t*>(frame.payload)), frame.length);
}

#ifdef BUILD_FOR_GST_ANALYTICS

static int getNextNaluPosition(uint8_t* buffer, int buffer_size, bool& is_aud_or_sei, bool& is_key_frame)
{
    if (buffer_size < 4) {
        return -1;
    }
    is_aud_or_sei = false;
    uint8_t* head = buffer;
    uint8_t* end = buffer + buffer_size - 4;
    while (head < end) {
        if (head[0]) {
            head++;
            continue;
        }
        if (head[1]) {
            head += 2;
            continue;
        }
        if (head[2]) {
            head += 3;
            continue;
        }
        if (head[3] != 0x01) {
            head++;
            continue;
        }
        if (((head[4] & 0x1F) == 9) || ((head[4] & 0x1F) == 6)) {
            is_aud_or_sei = true;
        }

        if (((head[4] & 0x1F) == 5)) {
            is_key_frame = true;
        }

        return static_cast<int>(head - buffer);
    }
    return -1;
}

#define MAX_NALS_PER_FRAME 128
static bool dropAUDandSEI(uint8_t* framePayload, int frameLength)
{
    uint8_t* origin_pkt_data = framePayload;
    int origin_pkt_length = frameLength;
    uint8_t* head = origin_pkt_data;

    std::vector<int> nal_offset;
    std::vector<bool> nal_type_is_aud_or_sei;
    std::vector<int> nal_size;
    bool is_aud_or_sei = false, has_aud_or_sei = false, is_key_frame = false;

    int sc_positions_length = 0;
    int sc_position = 0;
    while (sc_positions_length < MAX_NALS_PER_FRAME) {
        int nalu_position = getNextNaluPosition(origin_pkt_data + sc_position,
            origin_pkt_length - sc_position, is_aud_or_sei, is_key_frame);
        if (nalu_position < 0) {
            break;
        }
        sc_position += nalu_position;
        nal_offset.push_back(sc_position); //include start code.
        sc_position += 4;
        sc_positions_length++;
        if (is_aud_or_sei) {
            has_aud_or_sei = true;
            nal_type_is_aud_or_sei.push_back(true);
        } else {
            nal_type_is_aud_or_sei.push_back(false);
        }
    }
    if (sc_positions_length == 0 || !has_aud_or_sei)
        return frameLength;
    // Calculate size of each NALs
    for (unsigned int count = 0; count < nal_offset.size(); count++) {
        if (count + 1 == nal_offset.size()) {
            nal_size.push_back(origin_pkt_length - nal_offset[count]);
        } else {
            nal_size.push_back(nal_offset[count + 1] - nal_offset[count]);
        }
    }
    // remove in place the AUD NALs
    int new_size = 0;
    for (unsigned int i = 0; i < nal_offset.size(); i++) {
        if (!nal_type_is_aud_or_sei[i]) {
            memmove(head + new_size, head + nal_offset[i], nal_size[i]);
            new_size += nal_size[i];
        }
    }
    return is_key_frame;
}

void InternalOut::onFrame(uint8_t *buffer, int width, int height, uint32_t length)
{
    Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));

    outFrame.format = FRAME_FORMAT_H264;
    outFrame.length = length;
    outFrame.additionalInfo.video.width = width;
    outFrame.additionalInfo.video.height = height;
    outFrame.additionalInfo.video.isKeyFrame = dropAUDandSEI(buffer, length);
    outFrame.timeStamp = (m_frameCount++) * 1000 / 30 * 90;

    outFrame.payload = buffer;

    char sendBuffer[sizeof(Frame) + 1];
    size_t header_len = sizeof(Frame);

    sendBuffer[0] = TDT_MEDIA_FRAME;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<Frame*>(&outFrame)), header_len);
    m_transport->sendData(sendBuffer, header_len + 1, reinterpret_cast<char*>(const_cast<uint8_t*>(outFrame.payload)), outFrame.length);
#if 0
    FILE* bsDumpfp = fopen("/tmp/internalout.h264", "ab");
    if (bsDumpfp) {
        fwrite(buffer, 1, length, bsDumpfp);
        fclose(bsDumpfp);
    }
#endif
}

void InternalOut::setPad(GstPad *pad) {
     encoder_pad = pad;
}
#endif

void InternalOut::onTransportData(char* buf, int len)
{
    switch (buf[0]) {
        case TDT_FEEDBACK_MSG:
#ifdef BUILD_FOR_GST_ANALYTICS
        {
            FeedbackMsg* msg = reinterpret_cast<FeedbackMsg*>(buf + 1);
            if(encoder_pad != nullptr) {
                if(msg->type == VIDEO_FEEDBACK){
                    gst_pad_send_event(encoder_pad, gst_event_new_custom( GST_EVENT_CUSTOM_UPSTREAM, gst_structure_new( "GstForceKeyUnit", "all-headers", G_TYPE_BOOLEAN, TRUE, NULL)));
                }
            }
            else{
                deliverFeedbackMsg(*(reinterpret_cast<FeedbackMsg*>(buf + 1)));
            }
        }
#else
            deliverFeedbackMsg(*(reinterpret_cast<FeedbackMsg*>(buf + 1)));
#endif
        default:
            break;
    }
}


} /* namespace owt_base */

