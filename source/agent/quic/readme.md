# Open WebRTC Toolkit QUIC Agent

OWT QUIC agent allows client to connect server with `QuicTransport` of [WebTransport](https://wicg.github.io/web-transport/).

## Feature of QUIC agent

QUIC agent basically provides the network connections for client to send data to and receive data from conference server. It supports following features right now:

- Allow `QuicTransport` connections from client.
- Associate a `QuicStream` with a data stream in conference.

In the future, all data needs to be sent to or received from client will be able to go through QUIC agent. It includes but not limited to signaling messages, media data.

## Coding Style

Existing code in conference server seems to use mixed coding styles, QUIC agent follows WebKit style for C++ code because the `.clang-format` file in the root directory instructs us to do so, and Chromium style for JavaScript code. Global namespace is used for C++ code because most other C++ classes in conference server use it as well. Please consider to define a namespace for conference server.

## Issues

The development of QUIC agent is actively working in progress. Please file a bug on [GitHub](https://github.com/open-webrtc-toolkit/owt-server/issues) if something doesn't work as expected.