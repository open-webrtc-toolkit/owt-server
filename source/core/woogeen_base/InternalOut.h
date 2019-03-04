// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef InternalOut_h
#define InternalOut_h

#include "MediaFramePipeline.h"
#include "RawTransport.h"

namespace woogeen_base {

class InternalOut : public FrameDestination, public RawTransportListener {
public:
    InternalOut(const std::string& protocol, const std::string& dest_ip, unsigned int dest_port);
    virtual ~InternalOut();

    void onFrame(const Frame&);

    void onTransportData(char*, int len);
    void onTransportError() { }
    void onTransportConnected() { }

private:
    boost::shared_ptr<woogeen_base::RawTransportInterface> m_transport;
};

} /* namespace woogeen_base */

#endif /* InternalOut_h */
