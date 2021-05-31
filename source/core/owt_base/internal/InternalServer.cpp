// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalServer.h"
#include "RawTransport.h"

namespace owt_base {

DEFINE_LOGGER(InternalServer, "owt.InternalServer");

InternalServer::InternalServer(
    const std::string& protocol,
    unsigned int minPort,
    unsigned int maxPort,
    Listener* listener)
    : m_server(new TransportServer(this))
    , m_listener(listener)
{
    if (!TransportSecret::getPassphrase().empty()) {
        m_server->enableSecure();
    }
    m_server->listenTo(minPort, maxPort);
}

InternalServer::~InternalServer()
{
    m_server->close();
}

bool InternalServer::addSource(const std::string& streamId, FrameSource* src)
{
    ELOG_DEBUG("addSource %s, %p", streamId.c_str(), src);
    if (m_sourceMap.count(streamId)) {
        ELOG_WARN("Source for stream:%s already added", streamId.c_str());
        return false;
    }
    m_sourceMap[streamId] = src;
    return true;
}

bool InternalServer::removeSource(const std::string& streamId)
{
    if (!m_sourceMap.count(streamId)) {
        ELOG_WARN("Invalid source for stream:%s to remove", streamId.c_str());
        return false;
    }
    ELOG_DEBUG("removeSource %s", streamId.c_str());
    FrameSource* src = m_sourceMap[streamId];
    m_sourceMap.erase(streamId);
    assert(src);

    boost::mutex::scoped_lock lock(m_sessionMutex);
    for (int sId : m_sessionIdMap[streamId]) {
        if (m_sessions.count(sId)) {
            // Unlink source & destination
            src->removeAudioDestination(m_sessions[sId].get());
            src->removeVideoDestination(m_sessions[sId].get());
            src->removeDataDestination(m_sessions[sId].get());
            m_sessions.erase(sId);
        }
        m_server->closeSession(sId);
    }
    m_sessionIdMap.erase(streamId);
    return true;
}

unsigned int InternalServer::getListeningPort()
{
    return m_server->getListeningPort();
}

void InternalServer::onSessionAdded(int id)
{
    ELOG_DEBUG("onSessionAdded %d", id);
    boost::mutex::scoped_lock lock(m_sessionMutex);
    if (m_sessions.count(id)) {
        ELOG_WARN("Duplicate session added:%d", id);
    } else {
        m_sessions[id].reset(new InternalSession(id, this));
    }
}

void InternalServer::onSessionData(int id, uint8_t* data, uint32_t len)
{
    if (!m_sessions.count(id)) {
        ELOG_WARN("Unknown ID:%d for onSessionData", id);
        return;
    }
    if (len <= 0) {
        return;
    }
    if (data[0] == TDT_FEEDBACK_MSG) {
        auto session = m_sessions[id];
        if (session) {
            FeedbackMsg fbMsg = *(reinterpret_cast<FeedbackMsg*>(data + 1));
            if (fbMsg.cmd == INIT_STREAM_ID) {
                // Init stream ID
                std::string streamId(fbMsg.buffer.data, fbMsg.buffer.len);
                if (!session->streamId().empty()) {
                    ELOG_WARN("Multiple init stream fb, ignored");
                    streamId = session->streamId();
                } else if (m_sourceMap.count(streamId)) {
                    ELOG_WARN("Mapped StreamID :%s %p", streamId.c_str(), m_sourceMap[streamId]);

                    FrameSource* src = m_sourceMap[streamId];
                    if (src) {
                        // Unlink source & destination
                        src->addAudioDestination(session.get());
                        src->addVideoDestination(session.get());
                        src->addDataDestination(session.get());
                    }
                    session->setStreamId(streamId);
                    m_sessionIdMap[streamId].insert(id);
                    if (m_listener) {
                        m_listener->onConnected(streamId);
                    }
                } else {
                    ELOG_WARN("Unknown streamId:%s", streamId.c_str());
                }
            } else {
                std::string streamId = session->streamId();
                if (m_sourceMap.count(streamId)) {
                    FrameSource* src = m_sourceMap[streamId];
                    if (src) {
                        src->onFeedback(fbMsg);
                    }
                }
            }
        }
    } else {
        ELOG_WARN("Receive unexpected data from:%d", id);
    }
}

void InternalServer::onSessionRemoved(int id)
{
    boost::mutex::scoped_lock lock(m_sessionMutex);
    if (!m_sessions.count(id)) {
        ELOG_WARN("Non-exist session remove:%d", id);
    } else {
        auto session = m_sessions[id];
        std::string streamId = session->streamId();
        FrameSource* src = m_sourceMap[streamId];
        if (src) {
            // Unlink source & destination
            src->removeAudioDestination(session.get());
            src->removeVideoDestination(session.get());
            src->removeDataDestination(session.get());
        }
        m_sessions.erase(id);
        m_sessionIdMap[streamId].erase(id);
        if (m_listener) {
            m_listener->onDisconnected(streamId);
        }
    }
}

void InternalServer::InternalSession::onFrame(const Frame& frame)
{
    uint8_t sendBuffer[1 + sizeof(Frame) + frame.length];
    sendBuffer[0] = TDT_MEDIA_FRAME;
    memcpy(&sendBuffer[1],
           reinterpret_cast<uint8_t*>(const_cast<Frame*>(&frame)),
           sizeof(Frame));
    memcpy(&sendBuffer[1 + sizeof(Frame)], frame.payload, frame.length);

    m_parent->m_server->sendSessionData(m_id, sendBuffer, sizeof(sendBuffer));
}

void InternalServer::InternalSession::onMetaData(const MetaData& metadata)
{
    uint8_t sendBuffer[1 + sizeof(MetaData) + metadata.length];
    sendBuffer[0] = TDT_MEDIA_METADATA;
    memcpy(&sendBuffer[1],
           reinterpret_cast<uint8_t*>(const_cast<MetaData*>(&metadata)),
           sizeof(MetaData));
    memcpy(&sendBuffer[1 + sizeof(MetaData)], metadata.payload, metadata.length);

    m_parent->m_server->sendSessionData(m_id, sendBuffer, sizeof(sendBuffer));
}

} /* namespace owt_base */

