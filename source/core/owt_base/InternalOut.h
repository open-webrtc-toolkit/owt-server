// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef InternalOut_h
#define InternalOut_h

#ifdef BUILD_FOR_GST_ANALYTICS
#include <gst/gst.h>
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
    void onFrame(uint8_t *buffer, int width, int height, uint32_t length);
    void setPad(GstPad *pad);
#endif
    void onTransportData(char*, int len);
    void onTransportError() { }
    void onTransportConnected() { }

private:
    boost::shared_ptr<owt_base::RawTransportInterface> m_transport;
#ifdef BUILD_FOR_GST_ANALYTICS
    int m_frameCount;
    GstPad *encoder_pad;
#endif
};

} /* namespace owt_base */

#endif /* InternalOut_h */
