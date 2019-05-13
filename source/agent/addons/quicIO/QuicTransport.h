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

/*
 * Wrapper class of TQuicServer
 *
 * Receives media from one
 */
class QuicIn : public owt_base::FrameSource, public net::RQuicListener {
public:
    QuicIn(const std::string& cert_file, const std::string& key_file);
    virtual ~QuicIn();

    unsigned int getListeningPort();

    // Implements FrameSource
    void onFeedback(const owt_base::FeedbackMsg&) override;

    // Implements RQuicListener.
    void onReady() override;
    void onData(uint32_t session_id, uint32_t stream_id, char* data, uint32_t len) override;
private:
    void dFrame(char* buf);

    typedef struct {
        boost::shared_array<char> buffer;
        int length;
    } TransportData;

    std::shared_ptr<net::RQuicServerInterface> server_;
    bool m_hasStream;
    size_t m_bufferSize;
    TransportData m_receiveData;
    uint32_t m_receivedBytes;
};

/*
 * Wrapper class of TQuicClient
 *
 * Sends media to server
 */
class QuicOut : public owt_base::FrameDestination, public net::RQuicListener {
public:
    QuicOut(const std::string& dest_ip, unsigned int dest_port);
    virtual ~QuicOut();

    // Implements FrameDestination
    void onFrame(const owt_base::Frame&) override;

    // Implements RQuicListener.
    void onReady() override;
    void onData(uint32_t session_id, uint32_t stream_id, char* data, uint32_t len) override;
private:

    typedef struct {
        boost::shared_array<char> buffer;
        int length;
    } TransportData;

    std::shared_ptr<net::RQuicClientInterface> client_;
};

#endif  // INTERNAL_QUIC_H_
