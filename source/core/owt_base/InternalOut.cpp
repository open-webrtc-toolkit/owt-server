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

