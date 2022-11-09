// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "TransportClient.h"
#include <fstream>

namespace owt_base {

using boost::asio::ip::tcp;

DEFINE_LOGGER(TransportClient, "owt.TransportClient");

static constexpr const char kServerCrt[] = "cert/server.crt";
static constexpr const char kServerKey[] = "cert/server.key";

TransportClient::TransportClient(Listener* listener)
    : m_service(getIOService())
    , m_socket(m_service->service())
    , m_isSecure(false)
    , m_listener(listener)
{}

TransportClient::~TransportClient()
{
    close();
}

bool TransportClient::enableSecure()
{
    if (m_isSecure) {
        return true;
    }
    if (m_session) {
        ELOG_WARN("Failed to enable secure, client already start");
        return false;
    }
    if (!std::fstream{kServerCrt} ||
        !std::fstream{kServerKey}) {
        ELOG_WARN("Failed to enable secure, missing cert files");
        return false;
    }

    ELOG_DEBUG("Client enable secure");
    m_isSecure = true;

    m_sslContext.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
    m_sslContext->set_verify_mode(boost::asio::ssl::context::verify_peer);
    m_sslContext->use_certificate_file(kServerCrt, boost::asio::ssl::context::pem);
    m_sslContext->set_password_callback(boost::bind(&TransportSecret::getPassphrase));
    m_sslContext->use_private_key_file(kServerKey, boost::asio::ssl::context::pem);
    m_sslContext->load_verify_file(kServerCrt);

    return true;
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
    if (m_session || m_sslSocket) {
        return;
    }
    tcp::resolver resolver(m_service->service());
    tcp::resolver::query query(ip.c_str(), boost::to_string(port).c_str());
    tcp::resolver::iterator iterator = resolver.resolve(query);
    if (m_isSecure) {
        m_sslSocket.reset(new SSLSocket(m_service->service(), *m_sslContext));
        m_sslSocket->lowest_layer().open(tcp::v4());
        m_sslSocket->lowest_layer().async_connect(*iterator,
            boost::bind(&TransportClient::connectHandler, this,
                boost::asio::placeholders::error));
    } else {
        // TODO: Accept IPv6.
        m_socket.open(tcp::v4());
        m_socket.async_connect(*iterator,
            boost::bind(&TransportClient::connectHandler, this,
                boost::asio::placeholders::error));
    }
}

void TransportClient::connectHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        if (m_isSecure) {
            ELOG_DEBUG("Client start handshake");
            // Perform handkshake for secured session
            m_sslSocket->lowest_layer().set_option(tcp::no_delay(true));
            m_sslSocket->async_handshake(
                boost::asio::ssl::stream_base::client,
                boost::bind(&TransportClient::handshakeHandler, this,
                    boost::asio::placeholders::error));
        } else {
            m_socket.set_option(tcp::no_delay(true));
            m_session = std::make_shared<TransportSession>(
                0, m_service, std::move(m_socket), this);
            m_session->start();
            if (m_listener) {
                m_listener->onConnected();
            }
        }
    } else {
        ELOG_WARN("Connect error: %s", ec.message().c_str());
    }
}

void TransportClient::handshakeHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        ELOG_DEBUG("Handshake completed");
        m_session = std::make_shared<TransportSession>(
            0, m_service, m_sslSocket, this);
        m_session->start();
        m_sslSocket.reset();
        if (m_listener) {
            m_listener->onConnected();
        }
    } else {
        ELOG_WARN("Error during handshake: %s", ec.message().c_str());
    }
}

void TransportClient::sendData(const uint8_t* data, uint32_t len)
{
    TransportData tData{data, (uint32_t)len};
    m_session->sendData(tData);
}

void TransportClient::sendData(const uint8_t* header, uint32_t headerLength,
                               const uint8_t* payload, uint32_t payloadLength)
{
    TransportData data;
    data.buffer.reset(new uint8_t[headerLength + payloadLength]);
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
