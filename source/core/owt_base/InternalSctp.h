// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef InternalSctp_h
#define InternalSctp_h

#include "MediaFramePipeline.h"
#include "RawTransport.h"
#include "SctpTransport.h"

namespace owt_base {

class InternalSctp : public FrameSource, public FrameDestination, public RawTransportListener {
public:
    InternalSctp();
    virtual ~InternalSctp();

    unsigned int getLocalUdpPort() { return m_transport->getLocalUdpPort(); }
    unsigned int getLocalSctpPort() { return m_transport->getLocalSctpPort(); }

    void connect(const std::string &ip, unsigned int udpPort, unsigned int sctpPort);

    // Implements FrameSource
    void onFeedback(const FeedbackMsg&);

    // Implements FrameDestination
    void onFrame(const Frame&);

    // Implements RawTransportListener.
    void onTransportData(char* buf, int len);
    void onTransportError() { }
    void onTransportConnected() { }

private:
    boost::shared_ptr<owt_base::SctpTransport> m_transport;
};

} /* namespace owt_base */

#endif /* InternalSctp_h */