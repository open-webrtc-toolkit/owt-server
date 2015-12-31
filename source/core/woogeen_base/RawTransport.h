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

#ifndef RawTransport_h
#define RawTransport_h

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <queue>

namespace woogeen_base {

class RawTransport;

const char TDT_FEEDBACK_MSG = 0x5A;
const char TDT_MEDIA_FRAME = 0x8F;

enum Protocol {
    TCP = 0,
    UDP
};

class RawTransportListener {
public:
    virtual ~RawTransportListener() = 0;
    virtual void onTransportData(char*, int len) = 0;
    virtual void onTransportError() = 0;
    virtual void onTransportConnected() = 0;
};

inline RawTransportListener::~RawTransportListener() { }

// The buffer with this size should be enough to hold one message from/to the
// network based on the MTU of network protocols.
static const int TRANSPORT_BUFFER_SIZE = 64*1024;

typedef struct {
    char buffer[TRANSPORT_BUFFER_SIZE];
    int length;
} TransportData;

class RawTransport {
    DECLARE_LOGGER();
public:
    RawTransport(RawTransportListener* listener, Protocol proto, bool tag = true);
    ~RawTransport();

    void createConnection(const std::string& ip, uint32_t port);
    void listenTo(uint32_t port);
    void sendData(const char*, int len);
    void close();

    unsigned short getListeningPort();

private:
    void doSend();
    void receiveData();
    void readHandler(const boost::system::error_code&, std::size_t);
    void readPacketHandler(const boost::system::error_code&, std::size_t);
    void writeHandler(const boost::system::error_code&, std::size_t);
    void connectHandler(const boost::system::error_code&);
    void acceptHandler(const boost::system::error_code&);
    void dumpTcpSSLv3Header(const char*, int len);

    Protocol m_protocol;
    bool m_isClosing;
    bool m_tag;
    char m_readHeader[4];
    TransportData m_udpReceiveData;
    TransportData m_tcpReceiveData;
    TransportData m_udpSendData;
    std::queue<TransportData> m_sendQueue;
    boost::mutex m_sendQueueMutex;

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
    boost::asio::io_service m_ioService;
    boost::thread m_workThread;
    boost::scoped_ptr<boost::asio::ip::udp::socket> m_udpSocket;
    bool m_connectedUdp;
    boost::asio::ip::udp::endpoint m_udpRemoteEndpoint;
    boost::scoped_ptr<boost::asio::ip::tcp::socket> m_tcpSocket;
    boost::scoped_ptr<boost::asio::ip::tcp::acceptor> m_tcpAcceptor;

    RawTransportListener* m_listener;
};

} /* namespace woogeen_base */
#endif /* RawTransport_h */
