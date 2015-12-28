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

DEFINE_LOGGER(RawTransport, "woogeen.RawTransport");

RawTransport::RawTransport(RawTransportListener* listener, Protocol proto)
    : m_protocol(proto)
    , m_isClosing(false)
    , m_connectedUdp(false)
    , m_listener(listener)
{
}

RawTransport::~RawTransport()
{
    // We need to wait for the work thread to finish its job.
    if (m_tcpAcceptor)
        m_tcpAcceptor->close();
    if (m_tcpSocket)
        m_tcpSocket->close();
    if (m_udpSocket)
        m_udpSocket->close();

    m_workThread.join();
}

void RawTransport::close()
{
    ELOG_DEBUG("Closing...");
    m_isClosing = true;
}

void RawTransport::createConnection(const std::string& ip, uint32_t port)
{
    switch (m_protocol) {
    case TCP: {
        if (m_tcpSocket) {
            ELOG_WARN("TCP transport existed, ignoring the connection request for ip %s port %d\n", ip.c_str(), port);
        } else {
            m_tcpSocket.reset(new tcp::socket(m_ioService));
            tcp::resolver resolver(m_ioService);
            tcp::resolver::query query(tcp::v4(), ip.c_str(), boost::to_string(port).c_str());
            tcp::resolver::iterator iterator = resolver.resolve(query);

            m_tcpSocket->async_connect(*iterator,
                boost::bind(&RawTransport::connectHandler, this,
                    boost::asio::placeholders::error));
        }
        break;
    }
    case UDP: {
        if (m_udpSocket) {
            ELOG_WARN("UDP transport existed, ignoring the connection request for ip %s port %d\n", ip.c_str(), port);
        } else {
            m_udpSocket.reset(new udp::socket(m_ioService));
            udp::resolver resolver(m_ioService);
            udp::resolver::query query(udp::v4(), ip.c_str(), boost::to_string(port).c_str());
            udp::resolver::iterator iterator = resolver.resolve(query);

            m_udpRemoteEndpoint = *iterator;

            m_udpSocket->async_connect(*iterator,
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

void RawTransport::connectHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        switch (m_protocol) {
        case TCP:
            ELOG_DEBUG("Local TCP port: %d", m_tcpSocket->local_endpoint().port());
            // Disable Nagle's algorithm in the underlying TCP stack.
            // FIXME: Re-enable it later if we prove that the remote endpoing can correctly handle TCP reassembly
            // because that should improve the transmission efficiency.
            m_tcpSocket->set_option(tcp::no_delay(true));
            break;
        case UDP:
            ELOG_DEBUG("Local UDP port: %d", m_udpSocket->local_endpoint().port());
            m_connectedUdp = true;
            break;
        default:
            break;
        }

        m_listener->onTransportConnected();
        if (!m_isClosing)
            receiveData();
    } else {
        ELOG_DEBUG("Error establishing the %s connection: %s", m_protocol == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError();
    }
}

void RawTransport::acceptHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        switch (m_protocol) {
        case TCP:
            ELOG_DEBUG("Local TCP port: %d", m_tcpSocket->local_endpoint().port());
            // Disable Nagle's algorithm in the underlying TCP stack.
            // FIXME: Re-enable it later if we prove that the remote endpoing can correctly handle TCP reassembly
            // because that should improve the transmission efficiency.
            m_tcpSocket->set_option(tcp::no_delay(true));
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
        ELOG_DEBUG("Error accepting the %s connection: %s", m_protocol == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError();
    }
}

void RawTransport::listenTo(uint32_t port)
{
    switch (m_protocol) {
    case TCP: {
        if (m_tcpSocket) {
            ELOG_WARN("TCP transport existed, ignoring the listening request for port %d\n", port);
        } else {
            m_tcpSocket.reset(new tcp::socket(m_ioService));
            m_tcpAcceptor.reset(new tcp::acceptor(m_ioService, tcp::endpoint(tcp::v4(), port)));
            m_tcpAcceptor->async_accept(*(m_tcpSocket.get()),
                boost::bind(&RawTransport::acceptHandler, this,
                    boost::asio::placeholders::error));
        }
        break;
    }
    case UDP: {
        if (m_udpSocket) {
            ELOG_WARN("UDP transport existed, ignoring the listening request for port %d\n", port);
        } else {
            m_udpSocket.reset(new udp::socket(m_ioService, udp::endpoint(udp::v4(), port)));
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

unsigned short RawTransport::getListeningPort()
{
    unsigned short port = 0;
    switch (m_protocol) {
    case TCP: {
        if (m_tcpAcceptor)
            port = m_tcpAcceptor->local_endpoint().port();
        break;
    }
    case UDP: {
        if (m_udpSocket)
            port = m_udpSocket->local_endpoint().port();
        break;
    }
    default:
        break;
    }
    return port;
}

void RawTransport::readHandler(const boost::system::error_code& ec, std::size_t bytes)
{
    if (!ec || ec == boost::asio::error::message_size) {
        uint32_t payloadlen = 0;

        switch (m_protocol) {
        case TCP:
            assert(m_tcpSocket);

            payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_readHeader)));
            if (payloadlen > TRANSPORT_BUFFER_SIZE) {
                ELOG_WARN("Payload length is too big:%u", payloadlen);
                payloadlen = TRANSPORT_BUFFER_SIZE;
            }
            ELOG_DEBUG("readHandler(%zu):[%x,%x,%x,%x], payloadlen:%u", bytes, m_readHeader[0], m_readHeader[1], (unsigned char)m_readHeader[2], (unsigned char)m_readHeader[3], payloadlen);

            m_tcpSocket->async_read_some(boost::asio::buffer(m_tcpReceiveData.buffer, payloadlen),
                boost::bind(&RawTransport::readPacketHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
            break;
        case UDP:
            assert(m_udpSocket);

            payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_udpReceiveData.buffer)));
            if (bytes != payloadlen + 4) {
                ELOG_WARN("Packet incomplete. with payloadlen:%u, bytes:%zu", payloadlen, bytes);
            } else {
                unsigned char *p = reinterpret_cast<unsigned char*>(&m_udpReceiveData.buffer[4]);
                ELOG_DEBUG("readHandler(%zu): [%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x...%x,%x,%x,%x]", bytes, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[payloadlen-4], p[payloadlen-3], p[payloadlen-2], p[payloadlen-1]);
                m_listener->onTransportData(m_udpReceiveData.buffer + 4, payloadlen);
            }

            if (!m_isClosing)
                receiveData();
            break;
        default:
            break;
        }
    } else {
        ELOG_DEBUG("Error receiving %s data: %s", m_protocol == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError();
    }
}

void RawTransport::readPacketHandler(const boost::system::error_code& ec, std::size_t bytes)
{
    unsigned char *p = reinterpret_cast<unsigned char*>(&m_udpReceiveData.buffer[0]);
    ELOG_DEBUG("readHandler(%zu): [%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x...%x,%x,%x,%x]", bytes, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[bytes-4], p[bytes-3], p[bytes-2], p[bytes-1]);
    if (!ec || ec == boost::asio::error::message_size) {
        switch (m_protocol) {
        case TCP:
            m_listener->onTransportData(m_tcpReceiveData.buffer, bytes);
            break;
        case UDP:
            ELOG_WARN("Should not run into readPacketHandler under udp mode");
            break;
        default:
            break;
        }

        if (!m_isClosing)
            receiveData();
    } else {
        ELOG_DEBUG("Error receiving %s data: %s", m_protocol == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError();
    }
}

void RawTransport::doSend()
{
    TransportData& data = m_sendQueue.front();

    unsigned char *p = reinterpret_cast<unsigned char *>(&data.buffer[0]);
    ELOG_DEBUG("doSend(%d): [%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x...%x,%x,%x,%x]", data.length, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[data.length-4], p[data.length-3], p[data.length-2], p[data.length-1]);

    switch (m_protocol) {
    case TCP:
        assert(m_tcpSocket);
        boost::asio::async_write(*m_tcpSocket, boost::asio::buffer(data.buffer, data.length),
            boost::bind(&RawTransport::writeHandler, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        break;
    case UDP:
        assert(m_udpSocket);
        if (m_connectedUdp) {
            boost::system::error_code ignored_error;
            m_udpSocket->async_send(boost::asio::buffer(data.buffer, data.length),
                boost::bind(&RawTransport::writeHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            boost::system::error_code ignored_error;
            m_udpSocket->async_send_to(boost::asio::buffer(data.buffer, data.length),
                m_udpRemoteEndpoint,
                boost::bind(&RawTransport::writeHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        break;
    default:
        break;
    }
}

void RawTransport::writeHandler(const boost::system::error_code& ec, std::size_t bytes)
{
    if (ec) {
        ELOG_DEBUG("%s wrote data error: %s", m_protocol == UDP ? "UDP" : "TCP", ec.message().c_str());
    }

    ELOG_DEBUG("writeHandler(%zu)", bytes);

    if (m_protocol == TCP || m_protocol == UDP) {
        boost::lock_guard<boost::mutex> lock(m_sendQueueMutex);
        assert(m_sendQueue.size() > 0);
        m_sendQueue.pop();

        if (m_sendQueue.size() > 0) {
            doSend();
        }
    }
}

void RawTransport::dumpTcpSSLv3Header(const char* buf, int len)
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

void RawTransport::sendData(const char* buf, int len)
{
    if (len > TRANSPORT_BUFFER_SIZE) {
        ELOG_WARN("Discarding the %s message as the length %d exceeds the allowed maximum size.",
            m_protocol == UDP ? "UDP" : "TCP", len);
        return;
    }

    unsigned char *p = reinterpret_cast<unsigned char *>(const_cast<char *>(buf));
    ELOG_DEBUG("sendData(%d): [%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x...%x,%x,%x,%x]", len, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[len-4], p[len-3], p[len-2], p[len-1]);


    TransportData data;
    *(reinterpret_cast<uint32_t*>(data.buffer)) = htonl(len);
    memcpy(data.buffer + 4, buf, len);
    data.length = len + 4;

    boost::lock_guard<boost::mutex> lock(m_sendQueueMutex);
    m_sendQueue.push(data);
    if (m_sendQueue.size() == 1) {
        doSend();
    }
}

void RawTransport::receiveData()
{
    switch (m_protocol) {
    case TCP:
        assert(m_tcpSocket);
        m_tcpSocket->async_read_some(boost::asio::buffer(m_readHeader, 4),
            boost::bind(&RawTransport::readHandler, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        break;
    case UDP:
        assert(m_udpSocket);
        if (m_connectedUdp) {
            m_udpSocket->async_receive(boost::asio::buffer(m_udpReceiveData.buffer, TRANSPORT_BUFFER_SIZE),
                boost::bind(&RawTransport::readHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            m_udpSocket->async_receive_from(boost::asio::buffer(m_udpReceiveData.buffer, TRANSPORT_BUFFER_SIZE),
                m_udpRemoteEndpoint,
                boost::bind(&RawTransport::readHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        break;
    default:
        break;
    }
}

}
/* namespace woogeen_base */
