# gRPC for Portal

Since socket.io cpp client is not actively maintained for a long time. We're planing to add [gRPC](https://grpc.io/) support for portal. Therefore, clients are able to send and receive messages from portal through gRPC streams. In the first phase, default signaling channels of iOS and C++ applications will be replaced by gRPC.

gRPC support is working in progress. It is not complete yet.

## Design

### Architecture

Portal will be modularized as portal, socket.io server and gRPC server, so it can handle both socket.io connections and gRPC connections. Connections between socket.io/gRPC server and portal backend are implemented by internalIO.

gRPC server provides RPC service so clients send RPCs to server and receive responses when calls are complete. [Socket.io signaling messages](https://github.com/open-webrtc-toolkit/owt-server/blob/master/doc/Client-Portal%20Protocol.md#3-socketio-signaling) will be converted to RPCs. A basic rule for conversion is `RequestName` is the name of a RPC, `RequestData` is the argument of a RPC, `ResponseData` is the reply of a RPC. A server stream will be created for notifications send from server to client. All messages defined above will have protobuf definitions.

### Authentication

As socket.io maintains a persistent connection between client and server, while RPC is a call from client to server, it cannot do authentication only once at the beginning of a connection. Thus, an access token will be used for each session. To minimize changes required, the access token will mostly reuse the reconnection ticket defined in previous version. Besides the name change, it still has expiration time. And it can be refreshed before expiration as it used to be. For RPCs, every call brings the access token to server for authentication. For socket.io connection, it can be used for reconnection.