// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "GstInternalOut.h"
#include <gst/gst.h>
#include <stdio.h>


DEFINE_LOGGER(GstInternalOut, "GstInternalOut");

GstInternalOut::GstInternalOut()
{
    encoder_pad = NULL;
}

GstInternalOut::~GstInternalOut()
{
}

void GstInternalOut::setPad(GstPad *pad) {
     encoder_pad = pad;
}

void GstInternalOut::onFeedback(const owt_base::FeedbackMsg& msg)
{
    if(encoder_pad != nullptr) {
        if(msg.type == owt_base::VIDEO_FEEDBACK){
            gst_pad_send_event(encoder_pad, gst_event_new_custom( GST_EVENT_CUSTOM_UPSTREAM, gst_structure_new( "GstForceKeyUnit", "all-headers", G_TYPE_BOOLEAN, TRUE, NULL)));
        }
    }
}

void GstInternalOut::onFrame(const owt_base::Frame& frame)
{  
    deliverFrame(frame);
}

