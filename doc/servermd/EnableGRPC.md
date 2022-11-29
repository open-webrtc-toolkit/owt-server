##### Table of Contents
* 1.[Introduction](#introduction)
* 2.[Enable gRPC for internal RPC](#dependencies)
  * 2.1[Configuration for gRPC](#dependencies1)
  * 2.2[Enable TLS for gRPC](#dependencies2)

# 1 Introduction

By default, OWT[Open WebRTC Toolkit](https://github.com/open-webrtc-toolkit/owt-server) builds internal RPC utility based on message queue middleware(RabbitMQ). In this document we introduce the gRPC alternative for RabbitMQ, and a step by step guide to setup gRPC as OWT server's RPC framework.

# 2 Enable gRPC for internal RPC

In OWT server package, you can turn on gRPC option by updating configuration files. This section in troduces the configuration settings for internal gRPC.

# 2.1 Configuration for gRPC

To enable basic gRPC functionality for OWT server, edit following configuration files in server package:

    cluster_manager/cluster_manager.toml
    management_api/management_api
    portal/portal.toml
    webrtc_agent/agent.toml
    audio_agent/agent.toml
    video_agent/agent.toml
    conference_agent/agent.toml
    analytics_agent/agent.toml
    recording_agent/agent.toml
    streaming_agent/agent.toml
    quic_agent/agent.toml
    sip_agent/agent.toml
    sip_portal/sip_portal.toml

Set the item `enable_grpc` to `true` in these configuration files.

    enable_grpc = true

In `cluster_manager/cluster_manager.toml`, set `grpc_host` with `IP|hostname:port` of your own(when using TLS, only hostname is allowed).

    grpc_host = "localhost:10080"

In other modules' toml files, edit `grpc_host` to the value of `cluster_manager`.

    grpc_host = "localhost:10080"

If you want to enable HTTP proxy for gRPC, set environment variable `GRPC_ARG_HTTP_PROXY` to `1`, then system proxy will be used.

Restart OWT service after above configuration files being updated.

# 2.2 Enable TLS for gRPC

To enable TLS for gRPC of OWT server, you need prepare SSL certificates for internal gRPC services.
Both server certificate and client certificate are needed.

You could use following openssl commands to generate self-signed server and client certificates.

    # Generate CA key
    openssl genrsa -des3 -out ca.key 4096
    # Generate CA certificate
    openssl req -new -x509 -days 365 -key ca.key -out ca.crt -subj "/C=XX/ST=XX/L=XX/O=XX/OU=OWT/CN=ca"
    # Generate server key
    openssl genrsa -des3 -out server.key 4096
    # Generate server CSR
    openssl req -new -key server.key -out server.csr -subj "/C=XX/ST=XX/L=XX/O=XX/OU=OWT/CN=localhost"
    # Generate server certificate
    openssl x509 -req -days 365 -in server.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out server.crt
    # Generate client key
    openssl genrsa -des3 -out client.key 4096
    # Generate client CSR
    openssl req -new -key client.key -out client.csr -subj "/C=XX/ST=XX/L=XX/O=XX/OU=OWT/CN=localhost"
    # Generate client certificate
    openssl x509 -req -days 365 -in client.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out client.crt

During creating private keys, you need to enter passphrase for them. To save these passphrases in OWT's credential store, run `(cluster_manager|management_api|portal|webrtc_agent|{MODULE_NAME})/initauth.js --grpc` and save gRPC client/server key passphrase.

For each OWT server component, place root, server and client certificates in the same machine/instance. Make sure OWT processes have pemission to access those certificates.
Set following environment variables for SSL certificates before starting OWT server:

    OWT_GRPC_ROOT_CERT: The root certificate for server and client certificates.
    OWT_GRPC_SERVER_CERT: The server certificate.
    OWT_GRPC_SERVER_KEY: The encrypted server key.
    OWT_GRPC_CLIENT_CERT: The client certificate.
    OWT_GRPC_CLIENT_KEY: The encrypted client key.

Once you have updated these settings correctly, TLS for gRPC will be enabled  after restart OWT service.
