/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A plugin instance (PluginInterface) is created when the QUIC agent is
 * started. A data processor (ProcessorInterface) is created when a new pipeline
 * (publication - subscription pair) is created.
 *
 * See doc/servermd/QuicAgentPluginGuide.md for detailed information.
 */

#ifndef QUIC_QUICAGENTPLUGININTERFACE_H_
#define QUIC_QUICAGENTPLUGININTERFACE_H_

#include <cstdint>

// Most classes of addons don't have a namespace. Because developers may need to include this file when develop plugins, we add a namespace for it to avoid potential conflicts.
namespace owt {
namespace quic_agent_plugin {
    // Connection types for source and sink.
    enum class ConnectionType {
        kUnknown = 0,
        // A WebTransport connection usually means a connection between client and server.
        kWebTransport,
        // An internal connection is a connection between different agents.
        kInternalConnection,
    };

    // This struct represents a data flow's information.
    // See https://github.com/open-webrtc-toolkit/owt-server/blob/master/doc/Client-Portal%20Protocol.md for definitions of publication and subscription.
    struct ConnectionInfo {
        const ConnectionType type;
        // Publication ID or subscription ID.
        const char* id;
    };

    // A frame received of sent by QUIC agent. It's not a QUIC stream frame defined in QUIC spec.
    struct Frame {
        // Create a frame.
        static Frame* Create();
        // Delete the frame.
        void Dispose();
        // Payload of the frame.
        uint8_t* payload;
        // Payload's length.
        uint32_t length;

    private:
        Frame();
        virtual ~Frame();
    };

    // Interface for data processors.
    class ProcessorInterface {
    public:
        virtual ~ProcessorInterface() = default;
        // Invoked when a new subscription is requested for the publication associated with this processor.
        virtual void AddSink(const FrameSink& sink);
        // Invoked when a new frame is available. Ownership of `frame` is moved to processor. `frame` will not be sent back to the pipeline. If you need the frame to be delivered to the next node, please call `FrameSink`'s `deliverFrame` method explicitly.
        virtual void OnFrame(Frame* frame);
        // Invoked when a new feedback is available. Ownership of `feedback` is moved to processor. `feedback` will not be sent back to the pipeline. If you need the frame to be delivered to the next node, please call `FrameSource`'s `deliverFeedback` method explicitly.
        virtual void OnFeedback(Frame* feedback);
    };

    class FrameSource {
    public:
        virtual ~FrameSource() = default;
        // Call this function when a new feedback is ready to be delivered to the next node. Ownership of `feedback` is moved to the next node.
        virtual void DeliverFeedback(Frame* feedback);
        // Get publication infomation.
        virtual const ConnectionInfo& ConnectionInfo() const;
    };

    class FrameSink {
    public:
        virtual ~FrameSink() = default;
        // Call this function when a new frame is ready to be delivered to the next node. Ownership of `frame` is moved to the next node.
        virtual void DeliverFrame(Frame* frame);
        // Get subscription infomation.
        virtual const ConnectionInfo& ConnectionInfo() const;
    };

    // The interface for QUIC agent plugins. It allows developers to process data received or sent by QUIC agent.
    class PluginInterface {
    public:
        virtual ~PluginInterface() = default;
        // Create a data processor for a newly created data source. This method will be invoked when a new data source is created in QUIC agent. Returns nullptr if you don't need a processor for this source.
        virtual ProcessorInterface* CreateDataProcessor(const FrameSource& source);
        // Other methods for loading a shared library.
    };
}
}

#endif