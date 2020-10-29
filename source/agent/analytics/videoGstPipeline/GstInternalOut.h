// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef GstInternalOut_h
#define GstInternalOut_h

#include <gst/gst.h>
#include <logger.h>

#include "RawTransport.h"
#include "MediaFramePipeline.h"


class GstInternalOut : public owt_base::FrameSource,  public owt_base::FrameDestination {
    DECLARE_LOGGER();
public:
    GstInternalOut();
    virtual ~GstInternalOut();

    void setPad(GstPad *pad);

    // Implements FrameSource
    void onFeedback(const owt_base::FeedbackMsg&);

    void onFrame(const owt_base::Frame& frame);

private:
    GstPad *encoder_pad;
};

#endif /* GstInternalOut_h */
