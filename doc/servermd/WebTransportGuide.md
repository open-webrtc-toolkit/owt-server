# Open WebRTC Toolkit WebTransport Support

## Overview

Open WebRTC Toolkit (OWT) added [WebTransport](https://w3c.github.io/webtransport/) over [QUIC](https://tools.ietf.org/html/draft-ietf-quic-transport-32) support since 5.0. At this moment, it only supports simple data forwarding.

WebTransport over other protocols are not support.

## Streams

Similar to media streams, a stream for arbitrary data also supports `publish` and `subscribe`. A new property `data` is added to indicate if a stream is for arbitrary data. Media features like mixing and transcoding are not supported. If you develop client applications without OWT Client SDKs, please refer to [PR 125](https://github.com/open-webrtc-toolkit/owt-server/pull/125) for protocol changes and payload format. OWT JavaScript SDK and C++ SDK support streams for arbitrary data, please see API references of these two SDKs for newly added APIs. Basically, client side creates data streams, then `publish` and `subscribe` just as media streams. `streamadded` event also fires for data streams.

## QUIC agent

QUIC agent handles WebTransport connections from clients. It depends on [OWT QUIC SDK](https://github.com/open-webrtc-toolkit/owt-deps-quic). By default, it listens on 7700 port. Only one instance is running on a single machine or a docker environment.

## Roadmap

We are planning to support follow features, but we don't have an ETA so far.

- Signaling over QUIC. Instead of creating a WebSocket connection between a client and portal, we could use a WebTransport connection for both signaling and other data.
- Media over QUIC. Sending and receiving audio and video data over QUIC instead of WebRTC.
- Datagram support. Datagram support with FEC and NACK may help us to reduce the latency when sending media over QUIC.