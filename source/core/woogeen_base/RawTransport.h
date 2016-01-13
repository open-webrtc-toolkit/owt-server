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
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <queue>

namespace woogeen_base {

const char TDT_FEEDBACK_MSG = 0x5A;
const char TDT_MEDIA_FRAME = 0x8F;

enum Protocol {
    TCP = 0,
    UDP
};

class RawTransportListener {
public:
    virtual ~RawTransportListener() { }
    virtual void onTransportData(char*, int len) = 0;
    virtual void onTransportError() = 0;
    virtual void onTransportConnected() = 0;
};

class RawTransportInterface {
public:
    virtual ~RawTransportInterface() { }
    virtual void createConnection(const std::string& ip, uint32_t port) = 0;
    virtual void listenTo(uint32_t port) = 0;
    virtual void sendData(const char*, int len) = 0;
    virtual void close() = 0;

    virtual unsigned short getListeningPort() = 0;
};

template<Protocol prot>
class RawTransport : public RawTransportInterface {
    DECLARE_LOGGER();
public:
    RawTransport(RawTransportListener* listener, size_t bufferSize = 256 * 1024, bool tag = true);
    ~RawTransport();

    void createConnection(const std::string& ip, uint32_t port);
    void listenTo(uint32_t port);
    void sendData(const char*, int len);
    void close();

    unsigned short getListeningPort();

private:
    typedef struct {
        boost::shared_ptr<char> buffer;
        int length;
    } TransportData;

    void doSend();
    void receiveData();
    void readHandler(const boost::system::error_code&, std::size_t);
    void readPacketHandler(const boost::system::error_code&, std::size_t);
    void writeHandler(const boost::system::error_code&, std::size_t);
    void connectHandler(const boost::system::error_code&);
    void acceptHandler(const boost::system::error_code&);
    void dumpTcpSSLv3Header(const char*, int len);

    bool m_isClosing;
    bool m_tag;
    char m_readHeader[4];
    size_t m_bufferSize;
    TransportData m_receiveData;
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
    union Socket {
        Socket() { }
        ~Socket() { }

        struct UDPSocket {
            UDPSocket() : connected(false) { }
            ~UDPSocket() { }

            boost::scoped_ptr<boost::asio::ip::udp::socket> socket;
            bool connected;
            boost::asio::ip::udp::endpoint remoteEndpoint;
        } udp;
        struct TCPSocket {
            TCPSocket() { }
            ~TCPSocket() { }

            boost::scoped_ptr<boost::asio::ip::tcp::socket> socket;
            boost::scoped_ptr<boost::asio::ip::tcp::acceptor> acceptor;
        } tcp;
    } m_socket;

    RawTransportListener* m_listener;
    uint32_t m_receivedBytes;
};

} /* namespace woogeen_base */
#endif /* RawTransport_h */
