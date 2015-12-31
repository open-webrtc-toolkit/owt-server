/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#ifndef OoVooConnection_h
#define OoVooConnection_h

#include "OoVooProtocolHeader.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <RawTransport.h>

namespace oovoo_gateway {

template <woogeen_base::Protocol prot>
class AVSTransportListener : public woogeen_base::RawTransportListener {
    DECLARE_LOGGER();

public:
    AVSTransportListener(boost::shared_ptr<OoVooProtocolStack> ooVoo)
        : m_ooVoo(ooVoo)
    {
    }
    ~AVSTransportListener() { }

    void onTransportData(char*, int len);
    void onTransportError();
    void onTransportConnected();

private:
    boost::shared_ptr<OoVooProtocolStack> m_ooVoo;
};

/**
 * A ooVoo Connection. This class represents a simple connection between the GW and AVS server.
 * it comprises all the necessary Transport components.
 */
class OoVooConnection {
    DECLARE_LOGGER();
public:
    OoVooConnection(boost::shared_ptr<OoVooProtocolStack> ooVoo);
    virtual ~OoVooConnection();

    void connect(const std::string& ip, uint32_t port, bool isUdp);
    int sendData(const char*, int len, bool isUdp);
    void close();

private:
    // We need to ensure the order of the object destructions. In this case we
    // want to keep the listener objects alive until the transport is dead,
    // because in the transport destructor it may wait for the pending work to
    // be finished, and the pending work may trigger the transport listener (me)
    // to do something which requires the existence of m_ooVoo. Although m_ooVoo
    // is ref counted, it can be only referenced by me when I'm destructed.
    // According to C++ standard the non-static members are destructed in the
    // reverse order they were created.
    boost::scoped_ptr<AVSTransportListener<woogeen_base::TCP>> m_tcpListener;
    boost::scoped_ptr<AVSTransportListener<woogeen_base::UDP>> m_udpListener;
    boost::scoped_ptr<woogeen_base::RawTransport> m_tcpTransport;
    boost::scoped_ptr<woogeen_base::RawTransport> m_udpTransport;
};

} /* namespace oovoo_gateway */
#endif /* OoVooConnection_h */
