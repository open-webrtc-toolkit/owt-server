// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "TransportClient.h"


namespace owt_base {

using boost::asio::ip::tcp;

DEFINE_LOGGER(TransportClient, "owt.TransportClient");

TransportClient::TransportClient(Listener* listener)
    : m_service(getIOService())
    , m_socket(m_service->service())
    , m_listener(listener)
{}

TransportClient::~TransportClient()
{
    close();
}

void TransportClient::onData(uint32_t id, TransportData data)
{
    if (m_listener) {
        m_listener->onData(data.buffer.get(), data.length);
    }
}

void TransportClient::onClose(uint32_t id)
{
    if (m_listener) {
        m_listener->onDisconnected();
    }
}

void TransportClient::createConnection(const std::string& ip, uint32_t port)
{
    if (m_session) {
        return;
    }
    tcp::resolver resolver(m_service->service());
    tcp::resolver::query query(ip.c_str(), boost::to_string(port).c_str());
    tcp::resolver::iterator iterator = resolver.resolve(query);
    // TODO: Accept IPv6.
    m_socket.open(tcp::v4());
    m_socket.async_connect(*iterator,
        boost::bind(&TransportClient::connectHandler, this,
            boost::asio::placeholders::error));
}

void TransportClient::connectHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        m_session = std::make_shared<TransportSession>(
            0,
            m_service,
            std::move(m_socket),
            this);
        if (m_listener) {
            m_listener->onConnected();
        }
    } else {
        ELOG_WARN("Connect error: %s", ec.message().c_str());
    }
}

void TransportClient::sendData(const char* data, int len)
{
    TransportData tData{data, (uint32_t)len};
    m_session->sendData(tData);
}

void TransportClient::sendData(const char* header, int headerLength, const char* payload, int payloadLength)
{
    TransportData data;
    data.buffer.reset(new char[headerLength + payloadLength]);
    memcpy(data.buffer.get(), header, headerLength);
    memcpy(data.buffer.get() + headerLength, payload, payloadLength);
    data.length = headerLength + payloadLength;
    m_session->sendData(data);
}

void TransportClient::close()
{
    ELOG_DEBUG("Closing...");
    m_listener = nullptr;
    boost::system::error_code ec;
    if (m_socket.is_open()) {
        m_socket.cancel();
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close();
    }
    ELOG_DEBUG("Closed");
}

}
/* namespace owt_base */
