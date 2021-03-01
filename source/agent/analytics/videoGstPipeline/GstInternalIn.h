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


class GstInternalIn : public owt_base::RawTransportListener{
    DECLARE_LOGGER();
public:
    GstInternalIn(GstAppSrc *data, unsigned int minPort = 0, unsigned int maxPort = 0);
    virtual ~GstInternalIn();

    unsigned int getListeningPort();

    // Implements FrameSource
    void onFeedback(const owt_base::FeedbackMsg&);

    // Implements RawTransportListener.
    void onTransportData(char* buf, int len);
    void onTransportError() { }
    void onTransportConnected() { }
    void setPushData(bool status);

private:
    bool m_start;
    bool m_needKeyFrame;
    bool m_dumpIn;
    GstAppSrc *appsrc;
    boost::shared_ptr<owt_base::RawTransportInterface> m_transport;
    FILE *fp;
};

#endif /* GstInternalIn_h */
