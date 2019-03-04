// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <stdarg.h>
#include <stdio.h>

#include "SctpTransport.h"

namespace woogeen_base {

DEFINE_LOGGER(SctpTransport, "woogeen.SctpTransport");

namespace {

enum PreservedErrno {
    SCTP_EINPROGRESS = EINPROGRESS,
    SCTP_EWOULDBLOCK = EWOULDBLOCK
};

// Initialize with a large send space size currently
const int MAX_MSGSIZE = 1024 * 1024;

int usrsctp_ref_count = 0;
boost::mutex usrsctp_ref_mutex;

void debugSctpPrintf(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}

void initUsrSctp() {
    //printf("### initUsrSctp\n");
    usrsctp_init(0, &SctpTransport::onSctpOutboundPacket, &debugSctpPrintf);

    usrsctp_sysctl_set_sctp_sendspace((uint32_t) MAX_MSGSIZE);
    usrsctp_sysctl_set_sctp_recvspace((uint32_t) MAX_MSGSIZE);

    // uint32_t delay_time = 100;
    // usrsctp_sysctl_set_sctp_delayed_sack_time_default(delay_time);

    // uint32_t enable_immediate_sack = 1;
    // usrsctp_sysctl_set_sctp_enable_sack_immediately(enable_immediate_sack);
    // uint32_t on = 1;
    // usrsctp_sysctl_set_sctp_nr_sack_on_off(on);

    // send_size = usrsctp_sysctl_get_sctp_sendspace();
    // if (send_size < MAX_MSGSIZE) {
    //     //printf("Got smaller send size than expected: %d\n", send_size);
    // }
}

void uninitUsrSctp() {
    //printf("### uninitUsrSctp\n");
    for (size_t i = 0; i < 300; ++i) {
        if (usrsctp_finish() == 0) {
            //printf("### usrsctp finish\n");
            return;
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
    }
}

void incrementUsrSctpCount() {
    boost::lock_guard<boost::mutex> lock(usrsctp_ref_mutex);
    //printf("### increment %d\n", usrsctp_ref_count);
    if (!usrsctp_ref_count) {
        initUsrSctp();
    }
    usrsctp_ref_count++;
}

void decrementUsrSctpCount() {
    boost::lock_guard<boost::mutex> lock(usrsctp_ref_mutex);
    //printf("### decrement %d\n", usrsctp_ref_count);
    usrsctp_ref_count--;
    if (!usrsctp_ref_count) {
        uninitUsrSctp();
    }
}

} // End of namespace

SctpTransport::SctpTransport(RawTransportListener* listener, size_t initialBufferSize, bool tag)
    : m_isClosing(false)
    , m_localUdpPort(0)
    , m_remoteUdpPort(0)
    , m_localSctpPort(0)
    , m_remoteSctpPort(0)
    , m_tag(tag)
    , m_ready(false)
    , m_bufferSize(initialBufferSize)
    , m_fragBufferSize(initialBufferSize)
    , m_receivedBytes(0)
    , m_currentTsn(0)
    , m_sctpSocket(NULL)
    , m_sending(false)
    , m_listener(listener)
{
}

SctpTransport::~SctpTransport()
{
    ELOG_DEBUG("SctpTransport Destructor");
    m_isClosing = true;

    destroySctpSocket();

    m_ioService.stop();
    // We need to wait for the work thread to finish its job.
    m_workThread.join();

    m_work.reset();
    m_senderService.stop();
    m_senderThread.join();

    // Close the socket after it has no work left
    if (m_udpSocket && m_udpSocket->is_open()) {
        boost::system::error_code ec;
        m_udpSocket->shutdown(boost::asio::ip::udp::socket::shutdown_both, ec);
        m_udpSocket->close();
    }

    ELOG_DEBUG("SctpTransport Destructor END");
}

void SctpTransport::close()
{
    ELOG_DEBUG("Start Closing...");

    destroySctpSocket();

    m_ready = false;
    m_sending = false;
    m_isClosing = true;

    ELOG_DEBUG("Done Closing...");
}

void SctpTransport::handleNotification(union sctp_notification *notif, size_t n)
{
    if (notif->sn_header.sn_length != (uint32_t)n) {
        return;
    }
    switch (notif->sn_header.sn_type) {
    case SCTP_ASSOC_CHANGE:
        handleAssociationChangeEvent(&(notif->sn_assoc_change));
        break;
    case SCTP_PEER_ADDR_CHANGE:
        ELOG_DEBUG("SCTP_PEER_ADDR_CHANGE");
        //handlePeerAddressChangeEvent(&(notif->sn_paddr_change));
        break;
    case SCTP_REMOTE_ERROR:
        ELOG_WARN("SCTP_REMOTE_ERROR");
        break;
    case SCTP_SHUTDOWN_EVENT:
        ELOG_DEBUG("SCTP_SHUTDOWN_EVENT");
        break;
    case SCTP_ADAPTATION_INDICATION:
        ELOG_DEBUG("SCTP_ADAPTATION_INDICATION");
        break;
    case SCTP_PARTIAL_DELIVERY_EVENT:
        ELOG_DEBUG("SCTP_PARTIAL_DELIVERY_EVENT");
        break;
    case SCTP_AUTHENTICATION_EVENT:
        ELOG_DEBUG("SCTP_AUTHENTICATION_EVENT");
        break;
    case SCTP_SENDER_DRY_EVENT:
        //ELOG_DEBUG("SCTP_SENDER_DRY_EVENT");
        if (!m_sending) {
            m_sending = true;
            boost::lock_guard<boost::mutex> lock(m_sendBufferMutex);
            trySending();
        }
        break;
    case SCTP_NOTIFICATIONS_STOPPED_EVENT:
        ELOG_DEBUG("SCTP_NOTIFICATIONS_STOPPED_EVENT");
        break;
    case SCTP_SEND_FAILED_EVENT:
        ELOG_DEBUG("SCTP_SEND_FAILED_EVENT");
        //handleSendFailedEvent(&(notif->sn_send_failed_event));
        break;
    case SCTP_STREAM_RESET_EVENT:
        ELOG_DEBUG("SCTP_STREAM_RESET_EVENT");
        break;
    case SCTP_ASSOC_RESET_EVENT:
        ELOG_DEBUG("SCTP_ASSOC_RESET_EVENT");
        break;
    case SCTP_STREAM_CHANGE_EVENT:
        ELOG_DEBUG("SCTP_STREAM_CHANGE_EVENT");
        break;
    default:
        break;
    }
}

void SctpTransport::handleAssociationChangeEvent(struct sctp_assoc_change *sac)
{
    ELOG_DEBUG("Association change ");
    switch (sac->sac_state) {
    case SCTP_COMM_UP:
        ELOG_DEBUG("SCTP_COMM_UP");
        m_ready = true;
        if (!m_sending) {
            m_sending = true;
            boost::lock_guard<boost::mutex> lock(m_sendBufferMutex);
            trySending();
        }
        break;
    case SCTP_COMM_LOST:
        ELOG_DEBUG("SCTP_COMM_LOST");
        break;
    case SCTP_RESTART:
        ELOG_DEBUG("SCTP_RESTART");
        m_ready = true;
        if (!m_sending) {
            m_sending = true;
            boost::lock_guard<boost::mutex> lock(m_sendBufferMutex);
            trySending();
        }
        break;
    case SCTP_SHUTDOWN_COMP:
        ELOG_DEBUG("SCTP_SHUTDOWN_COMP");
        break;
    case SCTP_CANT_STR_ASSOC:
        ELOG_DEBUG("SCTP_CANT_STR_ASSOC");
        break;
    default:
        ELOG_INFO("sctp association change state: UNKNOWN");
        break;
    }

    if ((sac->sac_state == SCTP_CANT_STR_ASSOC) ||
        (sac->sac_state == SCTP_SHUTDOWN_COMP) ||
        (sac->sac_state == SCTP_COMM_LOST)) {
        ELOG_INFO("Disconnect due to notifications");
        m_ready = false;
    }
}

int SctpTransport::onSctpInboundPacket(struct socket *sock, union sctp_sockstore addr, void *data,
                                              size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info)
{
    // printf("Msg of length %d received via %p:%u on stream %d with SSN %u and TSN %u, PPID %u, context %u.\n",
    //    (int)datalen,
    //    addr.sconn.sconn_addr,
    //    ntohs(addr.sconn.sconn_port),
    //    rcv.rcv_sid,
    //    rcv.rcv_ssn,
    //    rcv.rcv_tsn,
    //    ntohl(rcv.rcv_ppid),
    //    rcv.rcv_context);

    if (data) {
        ELOG_DEBUG("SCTP inbound packet, length:%d", (int)datalen);
        SctpTransport* transport = static_cast<SctpTransport*>(ulp_info);

        if (flags & MSG_NOTIFICATION) {
            transport->handleNotification((union sctp_notification *)data, datalen);
        } else {
            if (transport->m_tag) {
                transport->processPacket((char*) data, datalen, rcv.rcv_tsn);
            } else {
                transport->m_listener->onTransportData((char*) data, datalen);
            }
        }

        free(data);
    } else {
        ELOG_DEBUG("SCTP inbound data null");
    }

    return 1;
}

int SctpTransport::onSctpOutboundPacket(void *addr, void *buf, size_t length, uint8_t tos, uint8_t set_df)
{
    ELOG_DEBUG("onSctpOutboundPacket, length:%zu, buf:%p", length, buf);

    // char *dump_buf;
    // if ((dump_buf = usrsctp_dumppacket(buf, length, SCTP_DUMP_OUTBOUND)) != NULL) {
    //     ELOG_DEBUG("out dump_buf %s\n", dump_buf);
    //     usrsctp_freedumpbuffer(dump_buf);
    // }

    SctpTransport* transport = static_cast<SctpTransport*>(addr);
    transport->postPacket((char*) buf, length);

    return 0;
}

bool SctpTransport::createSctpSocket() {
    incrementUsrSctpCount();

    m_sctpSocket = usrsctp_socket(
        AF_CONN, SOCK_STREAM, IPPROTO_SCTP, &SctpTransport::onSctpInboundPacket,
        NULL, 0, this);

    if (!m_sctpSocket) {
        ELOG_ERROR("SCTP create socket fail.");
        decrementUsrSctpCount();
        return false;
    }

    // Make the socket non-blocking. Connect, close, shutdown etc will not block
    // the thread waiting for the socket operation to complete.
    if (usrsctp_set_non_blocking(m_sctpSocket, 1) < 0) {
        ELOG_ERROR("SCTP set non-blocking fail.");
        return false;
    }

    // This ensures that the usrsctp close call deletes the association. This
    // prevents usrsctp from calling OnSctpOutboundPacket with references to
    // this class as the address.
    linger linger_opt;
    linger_opt.l_onoff = 1;
    linger_opt.l_linger = 0;
    if (usrsctp_setsockopt(m_sctpSocket, SOL_SOCKET, SO_LINGER, &linger_opt,
                           sizeof(linger_opt)) < 0) {
        ELOG_ERROR("SCTP set SO_LINGER fail.");
        return false;
    }

    // Nagle.
    uint32_t nodelay = 1;
    if (usrsctp_setsockopt(m_sctpSocket, IPPROTO_SCTP, SCTP_NODELAY, &nodelay,
                         sizeof(nodelay))) {
        ELOG_ERROR("SCTP set NO DELAY fail.");
        return false;
    }

    // Subscribe to SCTP event notifications.
    int event_types[] = {
        SCTP_ASSOC_CHANGE,
        SCTP_PEER_ADDR_CHANGE,
        SCTP_SEND_FAILED_EVENT,
        SCTP_SENDER_DRY_EVENT,
        SCTP_STREAM_RESET_EVENT
    };
    struct sctp_event event;
    memset(&event, 0, sizeof(event));
    event.se_assoc_id = SCTP_ALL_ASSOC;
    event.se_on = 1;
    for (size_t i = 0; i < (sizeof(event_types)/sizeof(int)); i++) {
        event.se_type = event_types[i];
        if (usrsctp_setsockopt(m_sctpSocket, IPPROTO_SCTP, SCTP_EVENT, &event,
                               sizeof(event)) < 0) {
          ELOG_ERROR("SCTP set SCTP_EVENT type: %d fail.", event.se_type);
          return false;
        }
    }

    // Register this class as an address for usrsctp. This is used by SCTP to
    // direct the packets received (by the created socket) to this class.
    usrsctp_register_address(this);

    // If need to debug sctp level, add flags when building
#ifdef SCTP_DEBUG
    usrsctp_sysctl_set_sctp_debug_on(SCTP_DEBUG_ALL);
#endif

    return true;
}

void SctpTransport::destroySctpSocket() {
    if (m_sctpSocket) {
        ELOG_DEBUG("Destroy sctp socket");
        // We assume that SO_LINGER option is set to close the association when
        // close is called. This means that any pending packets in usrsctp will be
        // discarded instead of being sent.
        usrsctp_close(m_sctpSocket);
        m_sctpSocket = NULL;
        usrsctp_deregister_address(this);
        decrementUsrSctpCount();
    }
}

bool SctpTransport::setupSctpPeer() {
    if (m_isClosing) {
        ELOG_WARN("setupSctpPeer ignored during closing");
        return false;
    }

    // Initialize udp socket for transport
    if (m_udpSocket) {
        ELOG_WARN("UDP transport existed, ignoring the setupSctpPeer");
        return false;
    }
    m_udpSocket.reset(new boost::asio::ip::udp::socket(m_ioService,
        boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)));
    m_localUdpPort = m_udpSocket->local_endpoint().port();

    ELOG_DEBUG("Udp bind local port:%u", m_localUdpPort);

    // Initialize sctp socket
    if (m_sctpSocket) {
        ELOG_WARN("SCTP socket existed, ignoring the setupSctpPeer");
        return false;
    }

    if (!createSctpSocket()) {
        ELOG_ERROR("create SCTP socket fail");
        destroySctpSocket();
        return false;
    }

    // Bind sctp port 0 to get an available port
    struct sockaddr_conn sconn;
    memset(&sconn, 0, sizeof(struct sockaddr_conn));
    sconn.sconn_family = AF_CONN;
    sconn.sconn_port = htons(0);
    sconn.sconn_addr = this;
    if (usrsctp_bind(m_sctpSocket, (struct sockaddr *)&sconn, sizeof(struct sockaddr_conn)) < 0) {
        ELOG_ERROR("SCTP usrsctp_bind error");
        return false;
    }

    struct sockaddr *addrs;
    int addrNum = usrsctp_getladdrs(m_sctpSocket, 0, &addrs);
    if (addrNum != 1) {
        ELOG_ERROR("SCTP usrsctp_getladdrs error");
        return false;
    }

    sconn = *((struct sockaddr_conn*) addrs);
    m_localSctpPort = ntohs(sconn.sconn_port);

    usrsctp_freeladdrs(addrs);

    ELOG_DEBUG("SCTP bind local port:%u", m_localSctpPort);

    return true;
}

void SctpTransport::startSctpConnection() {
    if (m_isClosing) {
        ELOG_WARN("startSctpConnection ignored during closing");
        return;
    }

    ELOG_DEBUG("Remote Ports:%u %u", m_remoteUdpPort, m_remoteSctpPort);

    // Note: conversion from int to uint16_t happens on assignment.
    sockaddr_conn sconn;
    memset(&sconn, 0, sizeof(struct sockaddr_conn));
    sconn.sconn_family = AF_CONN;
    sconn.sconn_port = htons(m_remoteSctpPort);
    sconn.sconn_addr = this;

    // Connect usrsctp on main thread
    int connect_result = usrsctp_connect(
        m_sctpSocket, reinterpret_cast<sockaddr *>(&sconn), sizeof(sconn));
    if (connect_result < 0 && errno != SCTP_EINPROGRESS) {
        ELOG_ERROR("SCTP connect ERROR");
        destroySctpSocket();
        return;
    }

    m_work.reset(new boost::asio::io_service::work(m_senderService));
    m_ready = true;
    m_sending = true;

    if (m_senderThread.get_id() == boost::thread::id()) // Not-A-Thread
        m_senderThread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_senderService));

    // Set the MTU and disable MTU discovery.
    // We can only do this after usrsctp_connect or it has no effect.
    /*
    sctp_paddrparams params = {{0}};
    memcpy(&params.spp_address, &remote_sconn, sizeof(remote_sconn));
    params.spp_flags = SPP_PMTUD_DISABLE;
    params.spp_pathmtu = kSctpMtu;
    if (usrsctp_setsockopt(m_sctpSocket, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &params,
                           sizeof(params))) {
        ELOG_ERROR("Failed to set SCTP_PEER_ADDR_PARAMS.");
    }
    */

    boost::asio::ip::udp::resolver resolver(m_ioService);
    boost::asio::ip::udp::resolver::query query(
        boost::asio::ip::udp::v4(), m_remoteIp.c_str(),
        boost::to_string(m_remoteUdpPort).c_str());
    boost::asio::ip::udp::resolver::iterator iterator = resolver.resolve(query);

    m_udpSocket->async_connect(*iterator,
        [this] (const boost::system::error_code& error) {
            // Start receving on udp port
            ELOG_DEBUG("Udp async connect callback");
            receiveData();
        });

    if (m_workThread.get_id() == boost::thread::id()) // Not-A-Thread
        m_workThread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService));
}

void SctpTransport::postPacket(const char* buf, int len)
{
    TransportData data;
    data.buffer.reset(new char[len]);
    memcpy(data.buffer.get(), buf, len);
    data.length = len;

    // Make doSend all in workThread
    m_ioService.post([this, data]() {
        boost::lock_guard<boost::mutex> lock(m_sendQueueMutex);
        ELOG_DEBUG("m_sendQueue size: %zu", m_sendQueue.size());
        m_sendQueue.push(data);
        if (m_sendQueue.size() == 1)
            doSend();
    });
}

void SctpTransport::doSend()
{
    TransportData& data = m_sendQueue.front();

    assert(m_udpSocket);
    assert(m_remoteUdpPort);
    ELOG_DEBUG("Send to remote udp port %d->%d", m_localUdpPort, m_remoteUdpPort);

    m_udpSocket->async_send(boost::asio::buffer(data.buffer.get(), data.length),
        [this] (const boost::system::error_code& ec, std::size_t bytes) {
            if (ec) {
                ELOG_WARN("wrote data error: %s", ec.message().c_str());
                m_listener->onTransportError();
            }

            boost::lock_guard<boost::mutex> lock(m_sendQueueMutex);
            assert(m_sendQueue.size() > 0);
            m_sendQueue.pop();

            if (m_sendQueue.size() > 0)
                doSend();
        });
}

void SctpTransport::receiveData()
{
    // The receiveData is only called in workThread
    if (!m_receiveData.buffer)
        m_receiveData.buffer.reset(new char[m_bufferSize]);

    assert(m_udpSocket);
    assert(m_remoteUdpPort);

    ELOG_DEBUG("!!! %d start udp receive data from:%d", m_localUdpPort, m_remoteUdpPort);

    m_udpSocket->async_receive(boost::asio::buffer(m_receiveData.buffer.get(), m_bufferSize),
        [this] (const boost::system::error_code& ec, std::size_t bytes) {
            if (ec) {
                ELOG_WARN("udp async receive error:%s", ec.message().c_str());
                m_listener->onTransportError();
            }

            // char *dump_buf;

            // if ((dump_buf = usrsctp_dumppacket(m_receiveData.buffer.get(), (size_t)bytes, SCTP_DUMP_INBOUND)) != NULL) {
            //     ELOG_DEBUG("in dump_buf %s\n", dump_buf);
            //     usrsctp_freedumpbuffer(dump_buf);
            // }

            // Pass received udp packets back to usrsctp
            usrsctp_conninput(this, m_receiveData.buffer.get(), (size_t)bytes, 0);

            // Continue to receive
            receiveData();
        });
}

void SctpTransport::processPacket(const char* data, int len, uint32_t tsn)
{
    // Called in usrsctp's receive callback thread
    if (!m_fragments.buffer) {
        m_fragments.buffer.reset(new char[m_fragBufferSize]);
    }

    const int INT_SIZE = sizeof(uint32_t);
    if (len < INT_SIZE) {
        ELOG_ERROR("Packet with length less than %d is incorrect, drop it, length:%d", INT_SIZE, len);
        return;
    }

    if (m_currentTsn != tsn) {
        if (m_receivedBytes) {
            ELOG_WARN("SCTP received message not complete.");
            // Deliver last message
            m_listener->onTransportData(m_fragments.buffer.get(), m_receivedBytes);
            m_receivedBytes = 0;
        }
        m_currentTsn = tsn;
    }

    if (!m_receivedBytes) {
        // Read header & reset buffer if needed
        uint32_t msglen;
        memcpy(&msglen, data, INT_SIZE);
        msglen = ntohl(msglen);

        if (msglen > m_fragBufferSize) {
            while (msglen > m_fragBufferSize) {
                m_fragBufferSize *= 2;
                if (m_fragBufferSize > MAX_MSGSIZE) {
                    ELOG_WARN("Assembled fragbuffersize %u becomes large than MAX_MSGSIZE", m_fragBufferSize);
                    break;
                }
            }
            ELOG_WARN("Increase the received buffer size %u", m_fragBufferSize);
            m_fragments.buffer.reset(new char[m_fragBufferSize]);
        }
        m_fragments.length = msglen;

        uint32_t bodylen = len - INT_SIZE;
        if (m_fragments.length < bodylen) {
            ELOG_WARN("SCTP packet msglen too small, not correct.");
            return;
        }

        // Save to fragments
        memcpy(m_fragments.buffer.get(), data + INT_SIZE, bodylen);
        m_receivedBytes = bodylen;
    } else {
        if (m_fragments.length < m_receivedBytes + len) {
            ELOG_WARN("SCTP packet msglen too small, not correct.");
            return;
        }

        // Append to current buffer
        memcpy(m_fragments.buffer.get() + m_receivedBytes, data, len);
        m_receivedBytes += len;
    }

    if (m_fragments.length == m_receivedBytes) {
        // Received message successfully

        // We cannot change m_fragments.buffer immediately after the onTransportData returns
        // Because the listener may used this buffer directly
        m_listener->onTransportData(m_fragments.buffer.get(), m_fragments.length);
        m_receivedBytes = 0;
    }

}

void SctpTransport::open()
{
    ELOG_DEBUG("Sctp Open Start");
    if (setupSctpPeer()) {
        ELOG_DEBUG("Sctp Open Success");
    } else {
        ELOG_ERROR("Sctp Open Fail");
    }
}

void SctpTransport::connect(const std::string &ip, uint32_t udpPort, uint32_t sctpPort) {
    ELOG_DEBUG("Sctp Connect Start: %u, %u", udpPort, sctpPort);
    m_remoteIp = ip;
    m_remoteUdpPort = udpPort;
    m_remoteSctpPort = sctpPort;

    startSctpConnection();
}

void SctpTransport::sendData(const char* data, int len)
{
    sendData(NULL, 0, data, len);
}

void SctpTransport::sendData(const char* header, int headerLength, const char* data, int len)
{
    const int INT_SIZE = sizeof(uint32_t);

    if (!m_ready) {
        ELOG_DEBUG("SCTP not ready, send request ignored");
        return;
    }
    if (m_isClosing) {
        ELOG_WARN("SCTP closing, send request ignored");
        return;
    }

    if (headerLength + len > MAX_MSGSIZE) {
        // We don't fragment.
        ELOG_WARN("Try to send too large message exceed send buffer, ignore it.");
        return;
    }

    TransportData tData;
    if (m_tag) {
        tData.buffer.reset(new char[headerLength + len + INT_SIZE]);
        uint32_t msglen = htonl(headerLength + len);
        memcpy(tData.buffer.get(), &msglen, INT_SIZE);
        if (headerLength) {
            memcpy(tData.buffer.get() + INT_SIZE, header, headerLength);
        }
        memcpy(tData.buffer.get() + INT_SIZE + headerLength, data, len);
        tData.length = headerLength + len + INT_SIZE;
    } else {
        tData.buffer.reset(new char[headerLength + len]);
        if (headerLength) {
            memcpy(tData.buffer.get(), header, headerLength);
        }
        memcpy(tData.buffer.get() + headerLength, data, len);
        memcpy(tData.buffer.get(), data, len);
        tData.length = headerLength + len;
    }

    ELOG_DEBUG("SCTP send length: %d", tData.length);

    boost::lock_guard<boost::mutex> lock(m_sendBufferMutex);
    m_sendBuffer.push(tData);
    trySending();
}

void SctpTransport::trySending() {
    // Need m_sendBufferMutex lock before calling
    if (!m_sending || !m_ready || m_isClosing)
        return;

    if (m_sendBuffer.empty())
        return;

    TransportData& tData = m_sendBuffer.front();

    // Make sending all in senderThread
    m_senderService.post([this, tData]() {
        // Send data using SCTP.
        struct sctp_sndinfo sndinfo;
        sndinfo.snd_sid = 1;
        sndinfo.snd_flags = 0;
        sndinfo.snd_ppid = htonl(233);
        sndinfo.snd_context = 0;
        sndinfo.snd_assoc_id = 0;

        int send_res = usrsctp_sendv(
            m_sctpSocket, tData.buffer.get(), static_cast<size_t>(tData.length), NULL, 0, &sndinfo,
            static_cast<socklen_t>(sizeof(struct sctp_sndinfo)), SCTP_SENDV_SNDINFO, 0);
        if (send_res < 0) {
            if (errno == SCTP_EWOULDBLOCK) {
                ELOG_WARN("usrsctp_sendv: EWOULDBLOCK returned");

                m_sending = false;

                // Double the send buffer size
                int sndbufsize = MAX_MSGSIZE;
                int intlen = sizeof(int);
                if (usrsctp_getsockopt(m_sctpSocket, SOL_SOCKET, SO_SNDBUF, &sndbufsize,
                                       (socklen_t *)&intlen) < 0) {
                    ELOG_INFO("usrsctp_getsockopt: Can not get SNDBUF");
                } else {
                    ELOG_DEBUG("Send buffer size origin: %d", sndbufsize);
                    if (sndbufsize < MAX_MSGSIZE * 16) {
                        sndbufsize *= 2;
                        if (usrsctp_setsockopt(m_sctpSocket, SOL_SOCKET, SO_SNDBUF, &sndbufsize,
                                               sizeof(int)) < 0) {
                            ELOG_WARN("SCTP set SO_SNDBUF fail.");
                        }
                    } else {
                        ELOG_WARN("Send buffer size already max.");
                    }
                    ELOG_DEBUG("Send buffer size after: %d", sndbufsize);
                }

            } else {
                ELOG_ERROR("usrsctp_sendv: %d", errno);
            }
        } else {
            boost::lock_guard<boost::mutex> lock(m_sendBufferMutex);
            assert(m_sendBuffer.size() > 0);
            m_sendBuffer.pop();

            if (m_sendBuffer.size() > 0)
                trySending();
        }
    });
}


}
/* namespace woogeen_base */
