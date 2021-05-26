// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "TransportServer.h"
#include <fstream>

namespace owt_base {

using boost::asio::ip::tcp;

DEFINE_LOGGER(TransportServer, "owt.TransportServer");

static constexpr const char kServerCrt[] = "cert/server.crt";
static constexpr const char kServerKey[] = "cert/server.key";
static constexpr const char kDHParams[] = "cert/dh2048.pem";

TransportServer::TransportServer(
    TransportServer::Listener* listener)
    : m_nextSessionId(0)
    , m_service(new IOService())
    , m_isSecure(false)
    , m_socket(m_service->service())
    , m_acceptor(m_service->service())
    , m_isClosed(false)
    , m_listener(listener)
{}

TransportServer::~TransportServer()
{
    close();
    ELOG_DEBUG("Destructor end");
}



bool TransportServer::enableSecure()
{
    if (m_isSecure) {
        return true;
    }
    if (m_acceptor.is_open()) {
        ELOG_WARN("Failed to enable secure, server already start");
        return false;
    }
    if (!std::fstream{kServerCrt} ||
        !std::fstream{kServerKey}) {
        ELOG_WARN("Failed to enable secure, missing cert files");
        return false;
    }

    ELOG_DEBUG("Server enable secure");
    m_isSecure = true;

    m_sslContext.reset(new boost::asio::ssl::context(
        m_service->service(), boost::asio::ssl::context::sslv23));
    m_sslContext->set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::single_dh_use);
    m_sslContext->set_verify_mode(
        boost::asio::ssl::context::verify_peer
        | boost::asio::ssl::context::verify_fail_if_no_peer_cert);
    m_sslContext->set_password_callback(boost::bind(&TransportSecret::getPassphrase));

    m_sslContext->use_certificate_chain_file(kServerCrt);
    m_sslContext->use_private_key_file(kServerKey, boost::asio::ssl::context::pem);
    if (std::fstream{kDHParams}) {
        m_sslContext->use_tmp_dh_file(kDHParams);
    }
    m_sslContext->load_verify_file(kServerCrt);

    return true;
}

void TransportServer::onData(uint32_t id, TransportData data)
{
    if (m_listener) {
        m_listener->onSessionData(id, data.buffer.get(), data.length);
    }
}

void TransportServer::onClose(uint32_t id)
{
    onSessionRemoved(id);
}

void TransportServer::doAccept()
{
    if (!m_acceptor.is_open()) {
        return;
    }
    if (m_isSecure) {
        if (m_sslSocket) {
            ELOG_WARN("Previous ssl socket exist before accept");
        }
        m_sslSocket.reset(new SSLSocket(m_service->service(), *m_sslContext));
        m_acceptor.async_accept(m_sslSocket->lowest_layer(),
            boost::bind(&TransportServer::acceptHandler,
                this, boost::asio::placeholders::error));
    } else {
        m_acceptor.async_accept(m_socket,
            boost::bind(&TransportServer::acceptHandler,
                this, boost::asio::placeholders::error));
    }
}

void TransportServer::acceptHandler(const boost::system::error_code& ec)
{
    if (m_isClosed) {
        return;
    }
    if (!ec) {
        if (m_isSecure) {
            ELOG_DEBUG("Start handshake %d");
            m_sslSocket->lowest_layer().set_option(tcp::no_delay(true));
            m_sslSocket->async_handshake(
                boost::asio::ssl::stream_base::server,
                boost::bind(&TransportServer::handshakeHandler, this, m_sslSocket,
                    boost::asio::placeholders::error));
            m_sslSocket.reset();
        } else {
            m_socket.set_option(tcp::no_delay(true));
            int sessionId = m_nextSessionId++;
            m_sessions[sessionId] =
                std::make_shared<TransportSession>(
                    sessionId, m_service, std::move(m_socket), this);
            ELOG_DEBUG("Accept session %d", sessionId);
            if (m_listener) {
                m_listener->onSessionAdded(sessionId);
            }
        }
        doAccept();
    } else if (ec.value() != boost::system::errc::operation_canceled) {
        ELOG_WARN("Accept error: %s", ec.message().c_str());
        doAccept();
    }
}

void TransportServer::handshakeHandler(std::shared_ptr<SSLSocket> sock,
                                       const boost::system::error_code& ec)
{
    if (!ec) {
        int sessionId = m_nextSessionId++;
        m_sessions[sessionId] =
            std::make_shared<TransportSession>(
                sessionId, m_service, sock, this);
        ELOG_DEBUG("accept secure session %d", sessionId);
        if (m_listener) {
            m_listener->onSessionAdded(sessionId);
        }
    } else {
        ELOG_WARN("Error during handshake: %s", ec.message().c_str());
    }
}


void TransportServer::listenTo(uint32_t port)
{
    if (m_acceptor.is_open()) {
        return;
    }
    boost::system::error_code ec;
    tcp::endpoint endpoint(tcp::v4(), port);
    m_acceptor.open(endpoint.protocol());
    m_acceptor.bind(endpoint, ec);
    if (!ec) {
        m_acceptor.listen(boost::asio::socket_base::max_connections, ec);
        if (!ec) {
            doAccept();
        }
    } else {
        ELOG_WARN("TransportServer listenTo failed: %s", ec.message().c_str());
    }
}

void TransportServer::listenTo(uint32_t minPort, uint32_t maxPort)
{
    if (m_acceptor.is_open()) {
        return;
    }
    boost::system::error_code ec;

    // find port in range
    uint32_t portRange = maxPort - minPort + 1;
    uint32_t port = rand() % portRange + minPort;

    for (uint32_t i = 0; i < portRange; i++) {
        if (m_acceptor.is_open()) {
            m_acceptor.close();
        }
        m_acceptor.open(tcp::v4());
        m_acceptor.bind(tcp::endpoint(tcp::v4(), port), ec);

        if (ec != boost::system::errc::address_in_use) {
            break;
        }
        port++;
        if (port > maxPort) {
            port -= portRange;
        }
    }

    if (!ec) {
        m_acceptor.listen(boost::asio::socket_base::max_connections, ec);
        if (!ec) {
            ELOG_DEBUG("TCP transport listening on:%d(range:%d ~ %d)",
                m_acceptor.local_endpoint().port(), minPort, maxPort);
            doAccept();
        } else {
            ELOG_ERROR("Error(%s) in listening on port range %d ~ %d",
                ec.message().c_str(), minPort, maxPort);
        }
    } else {
        ELOG_WARN("TCP transport listen in port range bind error: %s", ec.message().c_str());
        if (m_acceptor.is_open()) {
            m_acceptor.close();
        }
    }
}

unsigned short TransportServer::getListeningPort()
{
    unsigned short port = 0;
    if (m_acceptor.is_open()) {
        port = m_acceptor.local_endpoint().port();
    }
    return port;
}

void TransportServer::onSessionRemoved(int id)
{
    if (m_sessions.count(id)) {
        m_sessions.erase(id);
        if (m_listener) {
            m_listener->onSessionRemoved(id);
        }
    }
}

void TransportServer::sendData(const char* data, int len)
{
    TransportData tData{data, len};
    for (auto it = m_sessions.begin(); it != m_sessions.end(); it++) {
        it->second->sendData(tData);
    }
}

void TransportServer::sendData(const char* header, int headerLength, const char* payload, int payloadLength)
{
    TransportData data;
    data.buffer.reset(new char[headerLength + payloadLength]);
    memcpy(data.buffer.get(), header, headerLength);
    memcpy(data.buffer.get() + headerLength, payload, payloadLength);
    data.length = headerLength + payloadLength;

    for (auto it = m_sessions.begin(); it != m_sessions.end(); it++) {
        it->second->sendData(data);
    }
}

void TransportServer::sendSessionData(int id, const char* data, int len)
{
    TransportData tData{data, len};
    if (m_sessions.count(id)) {
        m_sessions[id]->sendData(tData);
    }
}

void TransportServer::closeSession(int id)
{
    ELOG_DEBUG("close session: %d", id);
    if (m_sessions.count(id)) {
        m_sessions.erase(id);
    }
}


void TransportServer::close()
{
    if (!m_isClosed.exchange(true)) {
        ELOG_DEBUG("Closing... %p", this);
        m_listener = nullptr;
        boost::system::error_code ec;
        if (m_acceptor.is_open()) {
            m_acceptor.cancel();
            m_acceptor.close();
        }
        if (m_socket.is_open()) {
            m_socket.cancel();
            m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            m_socket.close();
        }
        if (m_sslSocket) {
            m_sslSocket->shutdown(ec);
            m_sslSocket->lowest_layer()
                .shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        }
        m_sessions.clear();
        m_service.reset();
        ELOG_DEBUG("Closed %p", this);
    }
}

}
/* namespace owt_base */
