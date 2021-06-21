// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef InternalClient_h
#define InternalClient_h

#include "TransportClient.h"
#include <logger.h>
#include "MediaFramePipeline.h"

namespace owt_base {

/*
 * InternalClient
 */
class InternalClient : public FrameSource,
                       public TransportClient::Listener {
    DECLARE_LOGGER();
public:
    class Listener {
    public:
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;
    };
    InternalClient(const std::string& streamId,
                   const std::string& protocol,
                   Listener* listener);
    InternalClient(const std::string& streamId,
                   const std::string& protocol,
                   const std::string& ip,
                   unsigned int port,
                   Listener* listener);
    virtual ~InternalClient();

    void connect(const std::string& ip, unsigned int port);

    // Implements FrameSource
    void onFeedback(const FeedbackMsg&) override;

    // Implements TransportClient::Listener
    void onConnected() override;
    void onData(uint8_t* data, uint32_t len) override;
    void onDisconnected() override;

private:
    boost::shared_ptr<TransportClient> m_client;
    std::string m_streamId;
    bool m_ready;
    Listener* m_listener;
};

} /* namespace owt_base */
#endif /* InternalClient_h */
