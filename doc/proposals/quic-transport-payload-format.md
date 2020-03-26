# QUIC Transport Payload and Message Format

This post defines the payload and message format for data transmitted over [QuicTransport](https://wicg.github.io/web-transport/#quic-transport).

## Streams

Both server and client can initialize a stream. When a stream is created, initial side sends a session ID, which is a 128 bit length message to the remote side. Session ID could be a publication ID or subscription ID as defined in [Client-Portal Protocol](https://github.com/open-webrtc-toolkit/owt-server/blob/master/doc/Client-Portal%20Protocol.md). As the session ID issued by server may less than 128 bit right now, fill it with 0 in most significant bits. Session ID 0 is reserved for signaling. When remote side receives the session ID, it should check whether session ID is valid. Terminate the stream if session ID is invalid, or send the same session ID to client if it is valid. Depends on the type of stream it created, one side or both sides are ready to send data.

## Datagram

Each package has a 128 bit header for session ID.

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   |                      Session Identifier                       |
   |                             ....                              |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Datagram Data (*)                      ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

It may increase about 2% network cost.


## Authentication

After creating a QuicTransport, a stream with session 0 should be created for authentication. If signaling is transmitted over QuicTransport, authentication follows the regular process defined by [Client-Portal Protocol](https://github.com/open-webrtc-toolkit/owt-server/blob/master/doc/Client-Portal%20Protocol.md). Otherwise, client sends a following message with UTF-8 encoding in JSON format to server.

```webidl
interface QuicTransportLoginInfo {
  attribute DOMString? token;  // Base64 encoded token.
};
```