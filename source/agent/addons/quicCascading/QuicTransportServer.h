// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef QUIC_TRANSPORT_SERVER_H_
#define QUIC_TRANSPORT_SERVER_H_

#include <string>
#include <memory>
#include "quic_raw_lib.h"
#include "MediaFramePipeline.h"

#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/mutex.hpp>
#include <mutex>
#include <nan.h>
#include <unordered_map>
#include <queue>
#include <logger.h>

/*
 * Wrapper class of TQuicServer
 *
 * Receives media from one
 */
//class QuicTransportServer : public net::RQuicListener, public owt_base::FrameSource, public owt_base::FrameDestination, public NanFrameNode {
class QuicTransportServer : public net::RQuicListener,  public Nan::ObjectWrap {
    DECLARE_LOGGER();
public:
    QuicTransportServer(unsigned int port, const std::string& cert_file, const std::string& key_file);
    virtual ~QuicTransportServer();

    static NAN_MODULE_INIT(init);

    static NAN_METHOD(newInstance);
    /*static NAN_METHOD(addDestination);
    static NAN_METHOD(removeDestination);
    // addInputStream(stream, kind)
    // kind could be "data", "audio" or "video".
    static NAN_METHOD(addInputStream);
    static NAN_METHOD(stop);*/
    static NAN_METHOD(getListeningPort);
    static NAN_METHOD(send);
    static NAN_METHOD(onNewStreamEvent);
    static NAN_METHOD(onRoomEvent);

    static Nan::Persistent<v8::Function> s_constructor;

    static NAUV_WORK_CB(onNewStreamCallback);
    static NAUV_WORK_CB(onRoomCallback);


    /*// Overrides owt_base::FrameSource.
    void onFeedback(const owt_base::FeedbackMsg&) override;

    // Overrides owt_base::FrameDestination.
    void onFrame(const owt_base::Frame&) override;
    void onVideoSourceChanged() override;

    // Overrides NanFrameNode.
    owt_base::FrameSource* FrameSource() override { return this; }
    owt_base::FrameDestination* FrameDestination() override { return this; }*/

    void onReady(uint32_t session_id, uint32_t stream_id) override;
    // Implements RQuicListener.
    void onData(uint32_t session_id, uint32_t stream_id, char* data, uint32_t len) override;
private:

    typedef struct {
        boost::shared_array<char> buffer;
        int length;
    } TransportData;

    void sendData(uint32_t session_id, uint32_t stream_id, const std::string& data);

    unsigned int getServerPort();

    std::shared_ptr<net::RQuicServerInterface> m_quicServer;
    std::unordered_map<std::string, bool> hasStream_;
    size_t m_bufferSize;
    TransportData m_receiveData;
    uint32_t m_receivedBytes;
    uv_async_t m_asyncOnNewStream;
    uv_async_t m_asyncOnRoomEvents;
    bool has_stream_callback_;
    bool has_room_callback_;
    std::queue<std::string> stream_messages;
    std::queue<std::string> room_messages;
    Nan::Callback *stream_callback_;
    Nan::Callback *room_callback_;
    boost::mutex mutex;
};

#endif  // QUIC_TRANSPORT_SERVER_H_
