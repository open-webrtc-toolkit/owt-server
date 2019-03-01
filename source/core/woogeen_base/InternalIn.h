// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef InternalIn_h
#define InternalIn_h

#include "MediaFramePipeline.h"
#include "RawTransport.h"

namespace woogeen_base {

class InternalIn : public FrameSource, public RawTransportListener {
public:
    InternalIn(const std::string& protocol, unsigned int minPort = 0, unsigned int maxPort = 0);
    virtual ~InternalIn();

    unsigned int getListeningPort();

    // Implements FrameSource
    void onFeedback(const FeedbackMsg&);

    // Implements RawTransportListener.
    void onTransportData(char* buf, int len);
    void onTransportError() { }
    void onTransportConnected() { }

private:
    boost::shared_ptr<woogeen_base::RawTransportInterface> m_transport;
};

} /* namespace woogeen_base */

#endif /* InternalIn_h */
