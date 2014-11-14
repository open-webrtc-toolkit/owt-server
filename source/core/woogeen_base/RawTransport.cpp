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

RawTransport::RawTransport(RawTransportListener* listener) 
    : m_isClosing(false)
    , m_listener(listener)
{
}

RawTransport::~RawTransport()
{
    // We need to wait for the work thread to finish its job.
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

void RawTransport::createConnection(const std::string& ip, uint32_t port, Protocol prot)
{
    switch (prot) {
    case TCP: {
        if (m_tcpSocket) {
            ELOG_WARN("TCP transport existed, ignoring the connection request for ip %s port %d\n", ip.c_str(), port);
        } else {
            m_tcpSocket.reset(new tcp::socket(m_io_service));
            tcp::resolver resolver(m_io_service);
            tcp::resolver::query query(tcp::v4(), ip.c_str(), boost::to_string(port).c_str());
            tcp::resolver::iterator iterator = resolver.resolve(query);

            m_tcpSocket->async_connect(*iterator,
                boost::bind(&RawTransport::connectHandler, this, TCP,
                    boost::asio::placeholders::error));
        }
        break;
    }
    case UDP: {
        if (m_udpSocket) {
            ELOG_WARN("UDP transport existed, ignoring the connection request for ip %s port %d\n", ip.c_str(), port);
        } else {
            m_udpSocket.reset(new udp::socket(m_io_service));
            udp::resolver resolver(m_io_service);
            udp::resolver::query query(udp::v4(), ip.c_str(), boost::to_string(port).c_str());
            udp::resolver::iterator iterator = resolver.resolve(query);

            m_udpSocket->async_connect(*iterator,
                boost::bind(&RawTransport::connectHandler, this, UDP,
                    boost::asio::placeholders::error));
        }
        break;
    }
    default:
        break;
    }

    if (m_workThread.get_id() == boost::thread::id()) // Not-A-Thread
        m_workThread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_io_service));
}

void RawTransport::connectHandler(Protocol prot, const boost::system::error_code& ec)
{
    if (!ec) {
        switch (prot) {
        case TCP:
            ELOG_DEBUG("Local TCP port: %d", m_tcpSocket->local_endpoint().port());
            // Disable Nagle's algorithm in the underlying TCP stack.
            // FIXME: Re-enable it later if we prove that the remote endpoing can correctly handle TCP reassembly
            // because that should improve the transmission efficiency.
            m_tcpSocket->set_option(tcp::no_delay(true));
            break;
        case UDP:
            ELOG_DEBUG("Local UDP port: %d", m_udpSocket->local_endpoint().port());
            break;
        default:
            break;
        }

        m_listener->onTransportConnected(prot);
        if (!m_isClosing)
            receiveData(prot);
    } else {
        ELOG_DEBUG("Error establishing the %s connection: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError(prot);
    }
}

void RawTransport::readHandler(Protocol prot, const boost::system::error_code& ec, std::size_t bytes)
{
    if (!ec || ec == boost::asio::error::message_size) {
        switch (prot) {
        case TCP:
            m_listener->onTransportData(m_tcpReceiveData.buffer, bytes, TCP);
            break;
        case UDP:
            m_listener->onTransportData(m_udpReceiveData.buffer, bytes, UDP);
            break;
        default:
            break;
        }

        if (!m_isClosing)
            receiveData(prot);
    } else {
        ELOG_DEBUG("Error receiving %s data: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        if (!m_isClosing)
            m_listener->onTransportError(prot);
    }
}

void RawTransport::writeHandler(Protocol prot, const boost::system::error_code& ec, std::size_t bytes)
{
    if (ec) {
        ELOG_DEBUG("%s wrote data error: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
    }

    if (prot == TCP) {
        boost::lock_guard<boost::mutex> lock(m_tcpSendQueueMutex);
        assert(m_tcpSendQueue.size() > 0);
        m_tcpSendQueue.pop();

        if (m_tcpSendQueue.size() > 0) {
            // If there're pending send requests in the queue, we proactively
            // process them without waiting for another external request.
            TransportData& data = m_tcpSendQueue.front();
            boost::asio::async_write(*m_tcpSocket, boost::asio::buffer(data.buffer, data.length),
                boost::bind(&RawTransport::writeHandler, this, TCP,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
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

void RawTransport::sendData(const char* buf, int len, Protocol prot)
{
    if (len > TRANSPORT_BUFFER_SIZE) {
        ELOG_WARN("Discarding the %s message as the length %d exceeds the allowed maximum size.",
            prot == UDP ? "UDP" : "TCP", len);
        return;
    }

    switch (prot) {
    case TCP: {
        assert(m_tcpSocket);

        dumpTcpSSLv3Header(buf, len);
        // For data transported over the TCP protocol, we try to provide more
        // assurances to make the same data received by the destination.
        // So we use async_write to make sure every byte in the send buffer to
        // be sent out before the write handler is invoked, and use a queue to
        // make every message being processed one after another without
        // polluting each other.
        TransportData data;
        memcpy(data.buffer, buf, len);
        data.length = len;

        boost::lock_guard<boost::mutex> lock(m_tcpSendQueueMutex);
        m_tcpSendQueue.push(data);
        if (m_tcpSendQueue.size() == 1) {
            // We should hand over a new message to the TCP socket only if
            // the pending messages in the message queue have been processed.
            TransportData& data = m_tcpSendQueue.front();
            boost::asio::async_write(*m_tcpSocket, boost::asio::buffer(data.buffer, data.length),
                boost::bind(&RawTransport::writeHandler, this, TCP,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        break;
    }
    case UDP:
        assert(m_udpSocket);

        // We may revisit here later to see if we should adopt similar approach
        // with the TCP socket.

        // We need to make sure the buffer not being deleted before the write
        // handler is invoked, which is required by boost asio. So the safest
        // way is to copy it into a buffer owned by the transport.
        memcpy(m_udpSendData.buffer, buf, len);
        m_udpSocket->async_send(boost::asio::buffer(m_udpSendData.buffer, len),
            boost::bind(&RawTransport::writeHandler, this, UDP,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        break;
    default:
        break;
    }
}

void RawTransport::receiveData(Protocol prot)
{
    switch (prot) {
    case TCP:
        assert(m_tcpSocket);
        m_tcpSocket->async_read_some(boost::asio::buffer(m_tcpReceiveData.buffer, TRANSPORT_BUFFER_SIZE),
            boost::bind(&RawTransport::readHandler, this, TCP,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        break;
    case UDP:
        assert(m_udpSocket);
        m_udpSocket->async_receive(boost::asio::buffer(m_udpReceiveData.buffer, TRANSPORT_BUFFER_SIZE),
            boost::bind(&RawTransport::readHandler, this, UDP,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        break;
    default:
        break;
    }
}

}
/* namespace woogeen_base */
