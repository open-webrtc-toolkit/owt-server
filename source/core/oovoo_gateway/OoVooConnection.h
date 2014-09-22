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

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <queue>

namespace oovoo_gateway {

class RawTransport;

enum Protocol {
    TCP = 0,
    UDP
};

class RawTransportListener {
public:
    virtual ~RawTransportListener() = 0;
    virtual void onTransportData(char*, int len, Protocol) = 0;
    virtual void onTransportError(Protocol) = 0;
    virtual void onTransportConnected(Protocol) = 0;
};

inline RawTransportListener::~RawTransportListener() { }

/**
 * A ooVoo Connection. This class represents a simple connection between the GW and AVS server.
 * it comprises all the necessary Transport components.
 */
class OoVooConnection : public RawTransportListener {
    DECLARE_LOGGER();
public:
    OoVooConnection(boost::shared_ptr<OoVooProtocolStack> ooVoo);
    virtual ~OoVooConnection();

    void connect(const std::string& ip, uint32_t port, bool isUdp);
    int sendData(const char*, int len, bool isUdp);
    void close();

    virtual void onTransportData(char*, int len, Protocol);
    virtual void onTransportError(Protocol);
    virtual void onTransportConnected(Protocol);

private:
    // We need to ensure the order of the object destructions. In this case we
    // want to keep the m_ooVoo object alive until the transport is dead,
    // because in the transport destructor it may wait for the pending work to
    // be finished, and the pending work may trigger the transport listener (me)
    // to do something which requires the existence of m_ooVoo. Although m_ooVoo
    // is ref counted, it can be only referenced by me when I'm destructed.
    // According to C++ standard the non-static members are destructed in the
    // reverse order they were created.
    boost::shared_ptr<OoVooProtocolStack> m_ooVoo;
    boost::scoped_ptr<RawTransport> m_transport;
};

// The buffer with this size should be enough to hold one message from/to the
// network based on the MTU of network protocols.
static const int TRANSPORT_BUFFER_SIZE = 1600;

typedef struct {
    char buffer[TRANSPORT_BUFFER_SIZE];
    int length;
} TransportData;

class RawTransport {
    DECLARE_LOGGER();
public:
    RawTransport(RawTransportListener* listener);
    ~RawTransport();

    void createConnection(const std::string& ip, uint32_t port, Protocol);
    void sendData(const char*, int len, Protocol);
    void close();

private:
    void receiveData(Protocol);
    void readHandler(Protocol, const boost::system::error_code&, std::size_t);
    void writeHandler(Protocol, const boost::system::error_code&, std::size_t);
    void connectHandler(Protocol, const boost::system::error_code&);
    void dumpTcpSSLv3Header(const char*, int len);

    bool m_isClosing;
    TransportData m_udpReceiveData;
    TransportData m_tcpReceiveData;
    TransportData m_udpSendData;
    std::queue<TransportData> m_tcpSendQueue;
    boost::mutex m_tcpSendQueueMutex;

    // We need to ensure the order of the object destructions. In this case the
    // io_service object must be destructed after the socket objects, because in
    // the destructor of the socket objects it will tell the io_service to do
    // something like closing the descriptor, etc. In other words, the sockets
    // depend on io_service.
    // We ensure the order based on the C++ standard that the non-static members
    // are destructed in the reverse order they were created, so DON'T change
    // the order of the member declarations here.
    // Alternatively, we may make the io_service object reference counted but it
    // introduces unnecessary complexity.
    boost::asio::io_service m_io_service;
    boost::thread m_workThread;
    boost::scoped_ptr<boost::asio::ip::udp::socket> m_udpSocket;
    boost::scoped_ptr<boost::asio::ip::tcp::socket> m_tcpSocket;

    RawTransportListener* m_listener;
};

} /* namespace oovoo_gateway */
#endif /* OoVooConnection_h */
