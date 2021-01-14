// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "RawTransport.h"

#include <netinet/in.h>

namespace owt_base {

using boost::asio::ip::udp;
using boost::asio::ip::tcp;

DEFINE_TEMPLATE_LOGGER(template<Protocol prot>, RawTransport<prot>, "owt.RawTransport");

template<Protocol prot>
RawTransport<prot>::RawTransport(RawTransportListener* listener, size_t initialBufferSize, bool tag)
    : m_isClosing(false)
    , m_tag(tag)
    , m_bufferSize(initialBufferSize)
    , m_service(getIOService())
    , m_listener(listener)
    , m_receivedBytes(0)
{
}

template<Protocol prot>
RawTransport<prot>::~RawTransport()
{
    close();
}

template<Protocol prot>
void RawTransport<prot>::close()
{
    ELOG_DEBUG("Closing...");
    if (m_isClosing)
        return;

    m_isClosing = true;
    // Release the service
    m_service.reset();
    boost::system::error_code ec;
    switch (prot) {
    case TCP:
        if (m_socket.tcp.acceptor)
            m_socket.tcp.acceptor->close();
        if (m_socket.tcp.socket) {
            m_socket.tcp.socket->shutdown(tcp::socket::shutdown_both, ec);
            m_socket.tcp.socket->close();
        }
        break;
    case UDP:
        if (m_socket.udp.socket) {
            m_socket.udp.socket->shutdown(udp::socket::shutdown_both, ec);
            m_socket.udp.socket->close();
        }
        break;
    default:
        break;
    }
    ELOG_DEBUG("Closed");
}

template<Protocol prot>
void RawTransport<prot>::createConnection(const std::string& ip, uint32_t port)
{
    switch (prot) {
    case TCP: {
        if (m_socket.tcp.socket) {
            ELOG_WARN("TCP transport existed, ignoring the connection request for ip %s port %d\n", ip.c_str(), port);
        } else {
            m_socket.tcp.socket.reset(new tcp::socket(m_service->service()));
            tcp::resolver resolver(m_service->service());
            tcp::resolver::query query(ip.c_str(), boost::to_string(port).c_str());
            tcp::resolver::iterator iterator = resolver.resolve(query);
            // TODO: Accept IPv6.
            m_socket.tcp.socket->open(boost::asio::ip::tcp::v4());
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
            m_socket.udp.socket.reset(new udp::socket(m_service->service()));
            udp::resolver resolver(m_service->service());
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
}

template<Protocol prot>
void RawTransport<prot>::connectHandler(const boost::system::error_code& ec)
{
    if (m_isClosing)
        return;

    if (!ec) {
        switch (prot) {
        case TCP:
            ELOG_DEBUG("Connect ok, local TCP port: %d", m_socket.tcp.socket->local_endpoint().port());
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
        receiveData();
    } else {
        ELOG_ERROR("Error establishing the %s connection: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        m_listener->onTransportError();
    }
}

template<Protocol prot>
void RawTransport<prot>::acceptHandler(const boost::system::error_code& ec)
{
    if (m_isClosing)
        return;

    if (!ec) {
        switch (prot) {
        case TCP:
            ELOG_DEBUG("Accept ok, local TCP listening port: %d", m_socket.tcp.socket->local_endpoint().port());
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
        receiveData();
    } else {
        ELOG_ERROR("Error accepting the %s connection: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
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
            m_socket.tcp.socket.reset(new tcp::socket(m_service->service()));
            m_socket.tcp.acceptor.reset(new tcp::acceptor(m_service->service(), tcp::endpoint(tcp::v4(), port)));
            m_socket.tcp.acceptor->async_accept(*(m_socket.tcp.socket.get()),
                boost::bind(&RawTransport::acceptHandler, this,
                    boost::asio::placeholders::error));
            ELOG_DEBUG("TCP transport listening on %s:%d", m_socket.tcp.acceptor->local_endpoint().address().to_string().c_str(), m_socket.tcp.acceptor->local_endpoint().port());
        }
        break;
    }
    case UDP: {
        if (m_socket.udp.socket) {
            ELOG_WARN("UDP transport existed, ignoring the listening request for port %d\n", port);
        } else {
            m_socket.udp.socket.reset(new udp::socket(m_service->service(), udp::endpoint(udp::v4(), port)));
            receiveData();
        }
        break;
    }
    default:
        break;
    }
}

template<Protocol prot>
void RawTransport<prot>::listenTo(uint32_t minPort, uint32_t maxPort)
{
    switch (prot) {
    case TCP: {
        if (m_socket.tcp.socket) {
            ELOG_WARN("TCP transport existed, ignoring the listening request for minPort %d, maxPort %d\n", minPort, maxPort);
        } else {
            m_socket.tcp.socket.reset(new tcp::socket(m_service->service()));

            // find port in range
            uint32_t portRange = maxPort - minPort + 1;
            uint32_t port = rand() % portRange + minPort;
            boost::system::error_code ec;

            for (uint32_t i = 0; i < portRange; i++) {
                m_socket.tcp.acceptor.reset(new tcp::acceptor(m_service->service()));
                m_socket.tcp.acceptor->open(tcp::v4());
                m_socket.tcp.acceptor->bind(tcp::endpoint(tcp::v4(), port), ec);

                if (ec.value() != boost::system::errc::address_in_use) {
                    break;
                }
                if (m_socket.tcp.acceptor->is_open()) {
                    m_socket.tcp.acceptor->close();
                }

                port++;
                if (port > maxPort) {
                    port -= portRange;
                }
            }

            if (ec) {
                ELOG_WARN("TCP transport listen in port range bind error: %s", ec.message().c_str());
                break;
            }

            m_socket.tcp.acceptor->listen(boost::asio::socket_base::max_connections, ec);
            if (!ec) {
                ELOG_DEBUG("TCP transport listening on %s:%d(range:%d ~ %d)", m_socket.tcp.acceptor->local_endpoint().address().to_string().c_str(), m_socket.tcp.acceptor->local_endpoint().port(), minPort, maxPort);
                m_socket.tcp.acceptor->async_accept(*(m_socket.tcp.socket.get()),
                boost::bind(&RawTransport::acceptHandler, this,
                    boost::asio::placeholders::error));
            } else {
                ELOG_ERROR("Error(%s) in listening on port range %d ~ %d, last try on %d", ec.message().c_str(), minPort, maxPort, m_socket.tcp.acceptor->local_endpoint().port());
            }
        }
        break;
    }
    case UDP: {
        if (m_socket.udp.socket) {
            ELOG_WARN("UDP transport existed, ignoring the listening request for minPort %d, maxPort %d\n", minPort, maxPort);
        } else {
            ELOG_WARN("UDP transport does not support listening in specific range.");
            m_socket.udp.socket.reset(new udp::socket(m_service->service(), udp::endpoint(udp::v4(), 0)));
            receiveData();
        }
        break;
    }
    default:
        break;
    }
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

static const int BUFFER_ALIGNMENT = 16;
static const double BUFFER_EXPANSION_MULTIPLIER = 1.3;

template<Protocol prot>
void RawTransport<prot>::readHandler(const boost::system::error_code& ec, std::size_t bytes)
{
    if (m_isClosing)
        return;

    if (!ec || ec == boost::asio::error::message_size) {
        if (!m_tag) {
            m_listener->onTransportData(m_receiveData.buffer.get(), bytes);
            receiveData();
            return;
        }

        uint32_t payloadlen = 0;

        switch (prot) {
        case TCP:
            assert(m_socket.tcp.socket);

            m_receivedBytes += bytes;
            if (4 > m_receivedBytes) {
                ELOG_DEBUG("Incomplete header, continue receiving %u bytes", 4 - m_receivedBytes);
                m_socket.tcp.socket->async_read_some(boost::asio::buffer(m_readHeader + m_receivedBytes, 4 - m_receivedBytes),
                        boost::bind(&RawTransport::readHandler, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
            } else {
                payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_readHeader)));
                if (payloadlen > m_bufferSize) {
                    m_bufferSize = ((payloadlen * BUFFER_EXPANSION_MULTIPLIER + BUFFER_ALIGNMENT - 1) / BUFFER_ALIGNMENT) * BUFFER_ALIGNMENT;
                    ELOG_DEBUG("Increasing the buffer size: %zu", m_bufferSize);
                    m_receiveData.buffer.reset(new char[m_bufferSize]);
                }
                ELOG_DEBUG("readHandler(%zu):[%x,%x,%x,%x], payloadlen:%u", bytes, m_readHeader[0], m_readHeader[1], (unsigned char)m_readHeader[2], (unsigned char)m_readHeader[3], payloadlen);

                m_receivedBytes = 0;
                m_socket.tcp.socket->async_read_some(boost::asio::buffer(m_receiveData.buffer.get(), payloadlen),
                    boost::bind(&RawTransport::readPacketHandler, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
            break;
        case UDP:
            assert(m_socket.udp.socket);

            payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_receiveData.buffer.get())));
            if (bytes != payloadlen + 4) {
                // FIXME: Make UDP work with large packets.
                ELOG_WARN("Packet incomplete. with payloadlen:%u, bytes:%zu", payloadlen, bytes);
            } else {
                unsigned char *p = reinterpret_cast<unsigned char*>(&(m_receiveData.buffer.get())[4]);
                ELOG_DEBUG("readHandler(%zu): [%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x...%x,%x,%x,%x]", bytes, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[payloadlen-4], p[payloadlen-3], p[payloadlen-2], p[payloadlen-1]);
                m_listener->onTransportData(m_receiveData.buffer.get() + 4, payloadlen);
            }

            receiveData();
            break;
        default:
            break;
        }
    } else {
        ELOG_DEBUG("Error receiving %s data: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
        // Notify the listener about the socket error if the listener is not closing me.
        m_listener->onTransportError();
    }
}

template<Protocol prot>
void RawTransport<prot>::readPacketHandler(const boost::system::error_code& ec, std::size_t bytes)
{
    if (m_isClosing)
        return;

    ELOG_DEBUG("Port#%d recieved data(%zu)", m_socket.tcp.acceptor->local_endpoint().port(), bytes);
    uint32_t expectedLen = ntohl(*(reinterpret_cast<uint32_t*>(m_readHeader)));
    if (!ec || ec == boost::asio::error::message_size) {
        switch (prot) {
        case TCP:
            m_receivedBytes += bytes;
            if (expectedLen > m_receivedBytes) {
                ELOG_DEBUG("Expect to receive %u bytes, but actually received %zu bytes.", expectedLen, bytes);
                ELOG_DEBUG("Continue receiving %u bytes.", expectedLen - m_receivedBytes);
                m_socket.tcp.socket->async_read_some(boost::asio::buffer(m_receiveData.buffer.get() + m_receivedBytes, expectedLen - m_receivedBytes),
                        boost::bind(&RawTransport::readPacketHandler, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
            } else {
                m_receivedBytes = 0;
                m_listener->onTransportData(m_receiveData.buffer.get(), expectedLen);
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
        m_listener->onTransportError();
    }
}

template<Protocol prot>
void RawTransport<prot>::doSend()
{
    if (m_isClosing)
        return;

    TransportData& data = m_sendQueue.front();

    switch (prot) {
    case TCP:
        ELOG_DEBUG("Port#%d to send(%d)", m_socket.tcp.socket->local_endpoint().port(), data.length);
        assert(m_socket.tcp.socket);
        boost::asio::async_write(*(m_socket.tcp.socket), boost::asio::buffer(data.buffer.get(), data.length),
            boost::bind(&RawTransport::writeHandler, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        break;
    case UDP:
        assert(m_socket.udp.socket);
        if (!m_socket.udp.connected) {
            boost::system::error_code ignored_error;
            m_socket.udp.socket->async_send(boost::asio::buffer(data.buffer.get(), data.length),
                boost::bind(&RawTransport::writeHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            boost::system::error_code ignored_error;
            m_socket.udp.socket->async_send_to(boost::asio::buffer(data.buffer.get(), data.length),
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
    if (m_isClosing)
        return;

    if (ec) {
        ELOG_ERROR("%s wrote data error: %s", prot == UDP ? "UDP" : "TCP", ec.message().c_str());
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
    TransportData data;
    if (m_tag) {
        data.buffer.reset(new char[len + 4]);
        *(reinterpret_cast<uint32_t*>(data.buffer.get())) = htonl(len);
        memcpy(data.buffer.get() + 4, buf, len);
        data.length = len + 4;
    } else {
        data.buffer.reset(new char[len]);
        memcpy(data.buffer.get(), buf, len);
        data.length = len;
    }

    boost::lock_guard<boost::mutex> lock(m_sendQueueMutex);
    m_sendQueue.push(data);
    if (m_sendQueue.size() == 1)
        doSend();
}

template<Protocol prot>
void RawTransport<prot>::sendData(const char* header, int headerLength, const char* payload, int payloadLength)
{
    TransportData data;
    if (m_tag) {
        data.buffer.reset(new char[headerLength + payloadLength + 4]);
        *(reinterpret_cast<uint32_t*>(data.buffer.get())) = htonl(headerLength + payloadLength);
        memcpy(data.buffer.get() + 4, header, headerLength);
        memcpy(data.buffer.get() + 4 + headerLength, payload, payloadLength);
        data.length = headerLength + payloadLength + 4;
    } else {
        data.buffer.reset(new char[headerLength + payloadLength]);
        memcpy(data.buffer.get(), header, headerLength);
        memcpy(data.buffer.get() + headerLength, payload, payloadLength);
        data.length = headerLength + payloadLength;
    }

    boost::lock_guard<boost::mutex> lock(m_sendQueueMutex);
    m_sendQueue.push(data);
    if (m_sendQueue.size() == 1)
        doSend();
}

template<Protocol prot>
void RawTransport<prot>::receiveData()
{
    if (!m_receiveData.buffer)
        m_receiveData.buffer.reset(new char[m_bufferSize]);

    switch (prot) {
    case TCP:
        assert(m_socket.tcp.socket);
        if (m_tag) {
            m_socket.tcp.socket->async_read_some(boost::asio::buffer(m_readHeader, 4),
                boost::bind(&RawTransport::readHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            m_socket.tcp.socket->async_read_some(boost::asio::buffer(m_receiveData.buffer.get(), m_bufferSize),
                boost::bind(&RawTransport::readHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        break;
    case UDP:
        assert(m_socket.udp.socket);
        if (!m_socket.udp.connected) {
            m_socket.udp.socket->async_receive(boost::asio::buffer(m_receiveData.buffer.get(), m_bufferSize),
                boost::bind(&RawTransport::readHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            m_socket.udp.socket->async_receive_from(boost::asio::buffer(m_receiveData.buffer.get(), m_bufferSize),
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
/* namespace owt_base */
