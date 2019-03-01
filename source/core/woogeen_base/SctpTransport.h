// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SctpTransport_h
#define SctpTransport_h

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/atomic.hpp>
#include <logger.h>
#include <queue>
#include "RawTransport.h"
#include "usrsctp.h"

namespace woogeen_base {

// usrsctp max message size 256*1024
class SctpTransport {
    DECLARE_LOGGER();
public:
    SctpTransport(RawTransportListener* listener, size_t initialBufferSize = 65536, bool tag = true);
    ~SctpTransport();

    void sendData(const char*, int len);
    void sendData(const char* header, int headerLength, const char* payload, int payloadLength);

    // Sctp connection
    void open();
    void connect(const std::string &ip, uint32_t udpPort, uint32_t sctpPort);
    void close();

    unsigned short getLocalUdpPort() { return m_localUdpPort; }
    unsigned short getLocalSctpPort() { return m_localSctpPort; }

    static int onSctpInboundPacket(struct socket *sock, union sctp_sockstore addr, void *data,
                                   size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info);
    static int onSctpOutboundPacket(void *addr, void *buf, size_t length, uint8_t tos, uint8_t set_df);

private:
    bool createSctpSocket();
    void destroySctpSocket();

    void handleNotification(union sctp_notification *notif, size_t n);
    void handleAssociationChangeEvent(struct sctp_assoc_change *sac);

    bool setupSctpPeer();
    void startSctpConnection();

    void postPacket(const char* buf, int len);
    void doSend();
    void receiveData();
    void processPacket(const char* data, int len, uint32_t tsn);

    void trySending();

    bool m_isClosing;

    std::string m_remoteIp;
    uint16_t m_localUdpPort;
    uint16_t m_remoteUdpPort;
    uint16_t m_localSctpPort;
    uint16_t m_remoteSctpPort;

    bool m_tag;
    boost::atomic<bool> m_ready;

    // Transport data
    typedef struct {
        boost::shared_array<char> buffer;
        unsigned int length;
    } TransportData;

    // Receive data buffer
    TransportData m_receiveData;
    size_t m_bufferSize;

    // Fragments buffer
    TransportData m_fragments;
    uint32_t m_fragBufferSize;
    uint32_t m_receivedBytes;
    uint32_t m_currentTsn;

    // Send queue data for packet
    std::queue<TransportData> m_sendQueue;
    boost::mutex m_sendQueueMutex;

    boost::thread m_workThread;

    boost::asio::io_service m_ioService;
    boost::scoped_ptr<boost::asio::ip::udp::socket> m_udpSocket;
    struct socket* m_sctpSocket;

    // Send queue data for buffer
    boost::atomic<bool> m_sending;
    boost::thread m_senderThread;
    std::queue<TransportData> m_sendBuffer;
    boost::mutex m_sendBufferMutex;
    boost::asio::io_service m_senderService;
    boost::scoped_ptr<boost::asio::io_service::work> m_work;

    RawTransportListener* m_listener;
};

} /* namespace woogeen_base */
#endif /* RawTransport_h */
