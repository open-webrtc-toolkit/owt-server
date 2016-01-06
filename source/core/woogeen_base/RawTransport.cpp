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

#include "RawTransport.h"

#include <netinet/in.h>

namespace woogeen_base {

using boost::asio::ip::udp;
using boost::asio::ip::tcp;

DEFINE_TEMPLATE_LOGGER(template<Protocol prot>, RawTransport<prot>, "woogeen.RawTransport");

template<Protocol prot>
RawTransport<prot>::RawTransport(RawTransportListener* listener, bool tag)
    : m_isClosing(false)
    , m_tag(tag)
    , m_listener(listener)
    , m_receivedBytes(0)
{
    memset(&m_socket, 0, sizeof(m_socket));
}

template<Protocol prot>
RawTransport<prot>::~RawTransport()
{
    // We need to wait for the work thread to finish its job.
    switch (prot) {
    case TCP:
        if (m_socket.tcp.acceptor)
            m_socket.tcp.acceptor->close();
        if (m_socket.tcp.socket)
            m_socket.tcp.socket->close();
        break;
    case UDP:
        if (m_socket.udp.socket)
            m_socket.udp.socket->close();
        break;
    default:
        break;
    }

    m_workThread.join();
}

template<Protocol prot>
void RawTransport<prot>::close()
{
    ELOG_DEBUG("Closing...");
    m_isClosing = true;
}

template<Protocol prot>
void RawTransport<prot>::createConnection(const std::string& ip, uint32_t port)
{
    switch (prot) {
    case TCP: {
        if (m_socket.tcp.socket) {
            ELOG_WARN("TCP transport existed, ignoring the connection request for ip %s port %d\n", ip.c_str(), port);
        } else {
            m_socket.tcp.socket.reset(new tcp::socket(m_ioService));
            tcp::resolver resolver(m_ioService);
            tcp::resolver::query query(tcp::v4(), ip.c_str(), boost::to_string(port).c_str());
            tcp::resolver::iterator iterator = resolver.resolve(query);

            m_socket.tcp.socket->async_connect(*iterator,
                boost::bind(&RawTransport::connectHandler, this,
                    boost::asio::placeholders::error));
        }
        break;
    }
    case UDP: {
        if (m_socket.udp.socket) {
            ELOG_WARN("UDP transport existed, ignoring the connection request for ip %s port %d\n", ip.c_str(), port);
        } else {
            m_socket.udp.socket.reset(new udp::socket(m_ioService));
            udp::resolver resolver(m_ioService);
            udp::resolver::query query(udp::v4(), ip.c_str(), boost::to_string(port).c_str());
            udp::resolver::iterator iterator = resolver.resolve(query);

            m_socket.udp.remoteEndpoint = *iterator;

            m_socket.udp.socket->async_connect(*iterator,
                boost::bind(&RawTransport::connectHandler, this,
                    boost::asio::placeholders::error));
        }
        break;
    }
    default:
        break;
    }

    if (m_workThread.get_id() == boost::thread::id()) // Not-A-Thread
        m_workThread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService));
}

template<Protocol prot>
void RawTransport<prot>::connectHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        switch (prot) {
        case TCP:
            ELOG_DEBUG("Local TCP port: %d", m_socket.tcp.socket->local_endpoint().port());
            // Disable Nagle's algorithm in the underlying TCP stack.
            // FIXME: Re-enable it later if we prove that the remote endpoing can correctly handle TCP reassembly
            // because that should improve the transmission efficiency.
            m_socket.tcp.socket->set_option(tcp::no_delay(true));
            break;
        case UDP:
            ELOG_DEBUG("Local UDP port: %d", m_socket.udp.socket->local_endpoint().port());
            m_socket.udp.connected = true;
            break;
        default:
            break;
        }

        m_listener->onTransportConnected();
        if (!m_isClosing)
            receiveData();
    } else {
        ELOG_DEBUG("Error establishing the %s connection: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError();
    }
}

template<Protocol prot>
void RawTransport<prot>::acceptHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        switch (prot) {
        case TCP:
            ELOG_DEBUG("Local TCP port: %d", m_socket.tcp.socket->local_endpoint().port());
            // Disable Nagle's algorithm in the underlying TCP stack.
            // FIXME: Re-enable it later if we prove that the remote endpoing can correctly handle TCP reassembly
            // because that should improve the transmission efficiency.
            m_socket.tcp.socket->set_option(tcp::no_delay(true));
            break;
        case UDP:
            ELOG_DEBUG("No need to accept via UDP");
            break;
        default:
            break;
        }

        m_listener->onTransportConnected();
        if (!m_isClosing)
            receiveData();
    } else {
        ELOG_DEBUG("Error accepting the %s connection: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError();
    }
}

template<Protocol prot>
void RawTransport<prot>::listenTo(uint32_t port)
{
    switch (prot) {
    case TCP: {
        if (m_socket.tcp.socket) {
            ELOG_WARN("TCP transport existed, ignoring the listening request for port %d\n", port);
        } else {
            m_socket.tcp.socket.reset(new tcp::socket(m_ioService));
            m_socket.tcp.acceptor.reset(new tcp::acceptor(m_ioService, tcp::endpoint(tcp::v4(), port)));
            m_socket.tcp.acceptor->async_accept(*(m_socket.tcp.socket.get()),
                boost::bind(&RawTransport::acceptHandler, this,
                    boost::asio::placeholders::error));
        }
        break;
    }
    case UDP: {
        if (m_socket.udp.socket) {
            ELOG_WARN("UDP transport existed, ignoring the listening request for port %d\n", port);
        } else {
            m_socket.udp.socket.reset(new udp::socket(m_ioService, udp::endpoint(udp::v4(), port)));
            receiveData();
        }
        break;
    }
    default:
        break;
    }

    if (m_workThread.get_id() == boost::thread::id()) // Not-A-Thread
        m_workThread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService));
}

template<Protocol prot>
unsigned short RawTransport<prot>::getListeningPort()
{
    unsigned short port = 0;
    switch (prot) {
    case TCP: {
        if (m_socket.tcp.acceptor)
            port = m_socket.tcp.acceptor->local_endpoint().port();
        break;
    }
    case UDP: {
        if (m_socket.udp.socket)
            port = m_socket.udp.socket->local_endpoint().port();
        break;
    }
    default:
        break;
    }
    return port;
}

template<Protocol prot>
void RawTransport<prot>::readHandler(const boost::system::error_code& ec, std::size_t bytes)
{
    if (!ec || ec == boost::asio::error::message_size) {
        if (!m_tag) {
            m_listener->onTransportData(m_receiveData.buffer, bytes);
            if (!m_isClosing)
                receiveData();

            return;
        }

        uint32_t payloadlen = 0;

        switch (prot) {
        case TCP:
            assert(m_socket.tcp.socket);

            payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_readHeader)));
            if (payloadlen > TRANSPORT_BUFFER_SIZE) {
                ELOG_WARN("Payload length is too big:%u", payloadlen);
                payloadlen = TRANSPORT_BUFFER_SIZE;
            }
            ELOG_DEBUG("readHandler(%zu):[%x,%x,%x,%x], payloadlen:%u", bytes, m_readHeader[0], m_readHeader[1], (unsigned char)m_readHeader[2], (unsigned char)m_readHeader[3], payloadlen);

            m_socket.tcp.socket->async_read_some(boost::asio::buffer(m_receiveData.buffer, payloadlen),
                boost::bind(&RawTransport::readPacketHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
            break;
        case UDP:
            assert(m_socket.udp.socket);

            payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_receiveData.buffer)));
            if (bytes != payloadlen + 4) {
                ELOG_WARN("Packet incomplete. with payloadlen:%u, bytes:%zu", payloadlen, bytes);
            } else {
                unsigned char *p = reinterpret_cast<unsigned char*>(&m_receiveData.buffer[4]);
                ELOG_DEBUG("readHandler(%zu): [%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x...%x,%x,%x,%x]", bytes, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[payloadlen-4], p[payloadlen-3], p[payloadlen-2], p[payloadlen-1]);
                m_listener->onTransportData(m_receiveData.buffer + 4, payloadlen);
            }

            if (!m_isClosing)
                receiveData();
            break;
        default:
            break;
        }
    } else {
        ELOG_DEBUG("Error receiving %s data: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError();
    }
}

template<Protocol prot>
void RawTransport<prot>::readPacketHandler(const boost::system::error_code& ec, std::size_t bytes)
{
    unsigned char *p = reinterpret_cast<unsigned char*>(&m_receiveData.buffer[0]);
    ELOG_DEBUG("readPacketHandler(%zu): [%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x...%x,%x,%x,%x]", bytes, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[bytes-4], p[bytes-3], p[bytes-2], p[bytes-1]);
    uint32_t expectedLen = ntohl(*(reinterpret_cast<uint32_t*>(m_readHeader)));
    if (!ec || ec == boost::asio::error::message_size) {
        switch (prot) {
        case TCP:
            m_receivedBytes += bytes;
            if (expectedLen > m_receivedBytes) {
                ELOG_WARN("Expect to receive %u bytes, but actually received %zu bytes.", expectedLen, bytes);
                ELOG_INFO("Continue receiving %u bytes.", expectedLen - m_receivedBytes);
                m_socket.tcp.socket->async_read_some(boost::asio::buffer(m_receiveData.buffer + m_receivedBytes, expectedLen - m_receivedBytes),
                        boost::bind(&RawTransport::readPacketHandler, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
            } else {
                m_receivedBytes = 0;
                m_listener->onTransportData(m_receiveData.buffer, expectedLen);
                if (!m_isClosing)
                    receiveData();
            }
            break;
        case UDP:
            ELOG_WARN("Should not run into readPacketHandler under udp mode");
            break;
        default:
            break;
        }
    } else {
        ELOG_DEBUG("Error receiving %s data: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError();
    }
}

template<Protocol prot>
void RawTransport<prot>::doSend()
{
    TransportData& data = m_sendQueue.front();

    unsigned char *p = reinterpret_cast<unsigned char *>(&data.buffer[0]);
    ELOG_DEBUG("doSend(%d): [%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x...%x,%x,%x,%x]", data.length, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[data.length-4], p[data.length-3], p[data.length-2], p[data.length-1]);

    switch (prot) {
    case TCP:
        assert(m_socket.tcp.socket);
        boost::asio::async_write(*(m_socket.tcp.socket), boost::asio::buffer(data.buffer, data.length),
            boost::bind(&RawTransport::writeHandler, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        break;
    case UDP:
        assert(m_socket.udp.socket);
        if (!m_socket.udp.connected) {
            boost::system::error_code ignored_error;
            m_socket.udp.socket->async_send(boost::asio::buffer(data.buffer, data.length),
                boost::bind(&RawTransport::writeHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            boost::system::error_code ignored_error;
            m_socket.udp.socket->async_send_to(boost::asio::buffer(data.buffer, data.length),
                m_socket.udp.remoteEndpoint,
                boost::bind(&RawTransport::writeHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        break;
    default:
        break;
    }
}

template<Protocol prot>
void RawTransport<prot>::writeHandler(const boost::system::error_code& ec, std::size_t bytes)
{
    if (ec) {
        ELOG_DEBUG("%s wrote data error: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
    }

    ELOG_DEBUG("writeHandler(%zu)", bytes);

    boost::lock_guard<boost::mutex> lock(m_sendQueueMutex);
    assert(m_sendQueue.size() > 0);
    m_sendQueue.pop();

    if (m_sendQueue.size() > 0)
        doSend();
}

template<Protocol prot>
void RawTransport<prot>::dumpTcpSSLv3Header(const char* buf, int len)
{
    if (len >= 5) {
        // FIXME: A single TCP message can contain multiple SSLv3 Record Layers,
        // but we just examine the first layer for now.
        uint8_t type = buf[0];
        uint16_t version = ntohs(*reinterpret_cast<const uint16_t*>(&buf[1]));
        uint16_t length = ntohs(*reinterpret_cast<const uint16_t*>(&buf[3]));
        if (version == 0x0300) {
            switch (type) {
            case 23:
                ELOG_TRACE("TCP SSLv3 Application Data length %u; total message length %d", length, len);
                break;
            case 22:
                ELOG_TRACE("TCP SSLv3 Handshake message length %u; total message length %d", length, len);
                break;
            case 21:
                ELOG_TRACE("TCP SSLv3 Alert message length %u; total message length %d", length, len);
                break;
            case 20:
                ELOG_TRACE("TCP SSLv3 Change Cipher Spec message length %u; total message length %d", length, len);
                break;
            default:
                ELOG_TRACE("Unrecognized TCP SSLv3 message type %u; total message length %d", type, len);
                break;
            }
        } else {
            ELOG_TRACE("Not a TCP SSLv3 message: type %u, version %u; total message length %d", type, version, len);
        }
    }
}

template<Protocol prot>
void RawTransport<prot>::sendData(const char* buf, int len)
{
    if (len > TRANSPORT_BUFFER_SIZE) {
        ELOG_WARN("Discarding the %s message as the length %d exceeds the allowed maximum size.",
            prot == UDP ? "UDP" : "TCP", len);
        return;
    }

    unsigned char *p = reinterpret_cast<unsigned char *>(const_cast<char *>(buf));
    ELOG_DEBUG("sendData(%d): [%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x...%x,%x,%x,%x]", len, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[len-4], p[len-3], p[len-2], p[len-1]);

    TransportData data;
    if (m_tag) {
        *(reinterpret_cast<uint32_t*>(data.buffer)) = htonl(len);
        memcpy(data.buffer + 4, buf, len);
        data.length = len + 4;
    } else {
        memcpy(data.buffer, buf, len);
        data.length = len;
    }

    boost::lock_guard<boost::mutex> lock(m_sendQueueMutex);
    m_sendQueue.push(data);
    if (m_sendQueue.size() == 1)
        doSend();
}

template<Protocol prot>
void RawTransport<prot>::receiveData()
{
    switch (prot) {
    case TCP:
        assert(m_socket.tcp.socket);
        if (m_tag) {
            m_socket.tcp.socket->async_read_some(boost::asio::buffer(m_readHeader, 4),
                boost::bind(&RawTransport::readHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            m_socket.tcp.socket->async_read_some(boost::asio::buffer(m_receiveData.buffer, TRANSPORT_BUFFER_SIZE),
                boost::bind(&RawTransport::readHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        break;
    case UDP:
        assert(m_socket.udp.socket);
        if (!m_socket.udp.connected) {
            m_socket.udp.socket->async_receive(boost::asio::buffer(m_receiveData.buffer, TRANSPORT_BUFFER_SIZE),
                boost::bind(&RawTransport::readHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            m_socket.udp.socket->async_receive_from(boost::asio::buffer(m_receiveData.buffer, TRANSPORT_BUFFER_SIZE),
                m_socket.udp.remoteEndpoint,
                boost::bind(&RawTransport::readHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        break;
    default:
        break;
    }
}

template class RawTransport<TCP>;
template class RawTransport<UDP>;

}
/* namespace woogeen_base */
