// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef QUIC_TRANSPORT_H_
#define QUIC_TRANSPORT_H_

#include <string>
#include <memory>
#include "quic_raw_lib.h"
#include "MediaFramePipeline.h"

#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <EventRegistry.h>
#include <unordered_map>

/*
 * Wrapper class of TQuicServer
 *
 * Receives media from one
 */
class QuicIn : public net::RQuicListener, public EventRegistry {
public:
    QuicIn(unsigned int port, const std::string& cert_file, const std::string& key_file, EventRegistry *handle);
    virtual ~QuicIn();

    unsigned int getListeningPort();


    void send(uint32_t session_id, uint32_t stream_id, const std::string& data);

    void onReady(uint32_t session_id, uint32_t stream_id) override;
    // Implements RQuicListener.
    void onData(uint32_t session_id, uint32_t stream_id, char* data, uint32_t len) override;

    // EventRegistry
    bool notifyAsyncEvent(const std::string& event, const std::string& data) override;
    bool notifyAsyncEventInEmergency(const std::string& event, const std::string& data) override;
private:

    typedef struct {
        boost::shared_array<char> buffer;
        int length;
    } TransportData;

    std::shared_ptr<net::RQuicServerInterface> server_;
    std::unordered_map<std::string, bool> hasStream_;
    size_t m_bufferSize;
    TransportData m_receiveData;
    uint32_t m_receivedBytes;
    EventRegistry *m_asyncHandle;
};

/*
 * Wrapper class of TQuicClient
 *
 * Sends media to server
 */
class QuicOut : public net::RQuicListener, public EventRegistry {
public:
    QuicOut(const std::string& dest_ip, unsigned int dest_port, EventRegistry *handle);
    virtual ~QuicOut();

    void send(uint32_t session_id, uint32_t stream_id, const std::string& data);

    // Implements RQuicListener.
    void onData(uint32_t session_id, uint32_t stream_id, char* data, uint32_t len) override;
    void onReady(uint32_t session_id, uint32_t stream_id) override;

    // EventRegistry
    bool notifyAsyncEvent(const std::string& event, const std::string& data) override;
    bool notifyAsyncEventInEmergency(const std::string& event, const std::string& data) override;
private:

    typedef struct {
        boost::shared_array<char> buffer;
        int length;
    } TransportData;

    std::shared_ptr<net::RQuicClientInterface> client_;
    EventRegistry *m_asyncHandle;
    size_t m_bufferSize;
    TransportData m_receiveData;
    uint32_t m_receivedBytes;
};

#endif  // INTERNAL_QUIC_H_
