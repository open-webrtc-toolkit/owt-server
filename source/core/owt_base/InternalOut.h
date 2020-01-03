// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef InternalOut_h
#define InternalOut_h

#ifdef BUILD_FOR_GST_ANALYTICS
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#endif
#include "MediaFramePipeline.h"
#include "RawTransport.h"

namespace owt_base {

class InternalOut : public FrameDestination, public RawTransportListener {
public:
    InternalOut(const std::string& protocol, const std::string& dest_ip, unsigned int dest_port);
    virtual ~InternalOut();

    void onFrame(const Frame&);
    #ifdef BUILD_FOR_GST_ANALYTICS
    void onFrame(uint8_t *buffer, uint32_t length);
    #endif
    void onTransportData(char*, int len);
    void onTransportError() { }
    void onTransportConnected() { }

private:
    boost::shared_ptr<owt_base::RawTransportInterface> m_transport;
    int m_frameCount;
};

} /* namespace owt_base */

#endif /* InternalOut_h */
