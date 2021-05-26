// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef InternalServer_h
#define InternalServer_h

#include "TransportServer.h"
#include <logger.h>
#include "MediaFramePipeline.h"

#include <set>
#include <unordered_map>

namespace owt_base {

class InternalServer : public TransportServer::Listener {
    DECLARE_LOGGER();
public:
    class Listener {
    public:
        virtual void onConnected(const std::string& id) = 0;
        virtual void onDisconnected(const std::string& id) = 0;
    };
    InternalServer(const std::string& protocol,
                   unsigned int minPort,
                   unsigned int maxPort,
                   Listener* listener);
    virtual ~InternalServer();

    bool addSource(const std::string& streamId, FrameSource* src);
    bool removeSource(const std::string& streamId);

    unsigned int getListeningPort();

    // Implements TransportServer::Listener
    void onSessionAdded(int id) override;
    void onSessionData(int id, char* data, int len) override;
    void onSessionRemoved(int id) override;

private:
    class InternalSession : public FrameDestination {
    public:
        InternalSession(int id, InternalServer* p)
            : m_id(id), m_parent(p) {}
        // Implements FrameDestination
        void onFrame(const Frame&) override;
        void onMetaData(const MetaData&) override;

        int id() { return m_id; }
        std::string streamId() { return m_streamId; }
        void setStreamId(const std::string& streamId)
        {
            m_streamId = streamId;
        }
    private:
        int m_id;
        std::string m_streamId;
        InternalServer* m_parent;
    };

    boost::shared_ptr<TransportServer> m_server;
    boost::mutex m_sessionMutex;
    std::unordered_map<std::string, FrameSource*> m_sourceMap;
    std::unordered_map<std::string, std::set<int>> m_sessionIdMap;
    std::unordered_map<int, boost::shared_ptr<InternalSession>> m_sessions;
    Listener* m_listener;
};

} /* namespace owt_base */

#endif /* InternalServer_h */
