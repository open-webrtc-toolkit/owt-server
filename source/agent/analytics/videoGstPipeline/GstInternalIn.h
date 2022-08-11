// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef GstInternalIn_h
#define GstInternalIn_h

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <logger.h>

#include "RawTransport.h"
#include "MediaFramePipeline.h"


class GstInternalIn : public owt_base::FrameDestination {
    DECLARE_LOGGER();
public:
    GstInternalIn(GstAppSrc *data, int framerate);
    virtual ~GstInternalIn();

    void onFrame(const owt_base::Frame& frame);
    void setPushData(bool status);
    void setFramerate(int framerate);

private:
    bool m_start;
    bool m_needKeyFrame;
    bool m_dumpIn;
    size_t num_frames;
    int m_framerate;
    GstAppSrc *appsrc;
};

#endif /* GstInternalIn_h */
