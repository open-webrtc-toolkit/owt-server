// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// QUIC agent starts a QUIC transport server which listens incoming QUIC
// connections. After a new QUIC connection is established, it waits for a
// transport ID transmitted through signaling stream(content session ID is 0).
// Transport ID is issued by portal. QUIC agent gets authorized transport ID
// from |options| of publication and subscription.
// QuicStreamPipeline is 1:1 mapped to a publication or a subscription. But the
// QuicStream associated with QuicStreamPipeline could be a QuicStream instance
// or |undefined|, which means QuicStream is not ready at this moment. It also
// allows updating its associated QuicStream to a new one.
// publish, unpublish, subscribe, unsubscribe, linkup, cutoff are required by
// all agents. They are defined in base-agent.js.

'use strict';
var logger = require('../logger').logger;
var log = logger.getLogger('MediaBridge');
var addon = require('../quicCascading/build/Release/quicCascading.node');
const QuicTransportStreamPipeline =
    require('./quicTransportStreamPipeline');
const path = require('path');
const {InternalConnectionRouter} = require('./internalConnectionRouter');
const audioTrackId = '00000000000000000000000000000001';
const videoTrackId = '00000000000000000000000000000002';

log.info('QUIC transport node.')

module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    const rpcChannel = require('./rpcChannel')(rpcClient);
    const rpcReq = require('./rpcRequest')(rpcChannel);
    const that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    };
    const router = new InternalConnectionRouter(global.config.internal);
    const incomingStreamPipelines =
        new Map();  // Key is publication ID, value is stream pipeline.
    const outgoingStreamPipelines =
        new Map();  // Key is subscription ID, value is stream pipeline.
    const frameSourceMap =
        new Map();  // Key is publication ID, value is WebTransportFrameSource.
    const publicationOptions = new Map();
    const subscriptionOptions = new Map();
    let quicTransportServer;
    let controllers = {};
    let clusters = {};
    //Map OWT internal connections with quic streams
    let connections = {};
    let server;
    var cf = 'leaf_cert.pem';
    var kf = 'leaf_cert.pkcs8';
    const clusterName = global && global.config && global.config.cluster ?
        global.config.cluster.name :
        undefined;
    var port = global && global.config && global.config.bridge ? global.config.bridge.port : 8700;
    var bridge = global.config.bridge;

    var ip_address;
    (function getPublicIP() {
      var BINDED_INTERFACE_NAME = global.config.bridge.networkInterface;
      var interfaces = require('os').networkInterfaces(),
        addresses = [],
        k,
        k2,
        address;

      for (k in interfaces) {
        if (interfaces.hasOwnProperty(k)) {
          for (k2 in interfaces[k]) {
            if (interfaces[k].hasOwnProperty(k2)) {
              address = interfaces[k][k2];
              if (address.family === 'IPv4' && !address.internal) {
                if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
                  addresses.push(address.address);
                }
              }
              if (address.family === 'IPv6' && !address.internal) {
                if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
                  addresses.push('[' + address.address + ']');
                }
              }
            }
          }
        }
      }

      if (bridge.hostname === '' || bridge.hostname === undefined) {
        if (bridge.ip_address === '' || bridge.ip_address === undefined){
          ip_address = addresses[0];
        } else {
          ip_address = bridge.ip_address;
        }
      } else {
        ip_address = bridge.hostname;
      }

    })();

    const createFrameSource = (streamId, options, callback) => {
      if (frameSourceMap.has(streamId)) {
        callback('callback', {
          type: 'failed',
          reason: 'Frame source for ' + streamId + ' exists.'
        });
      }
      const frameSource = new addon.QuicTransportFrameSource(streamId);
      frameSourceMap.set(streamId, frameSource);
      return frameSource;
    };

    const createStreamPipeline =
        function(streamId, direction, options) {
      // Client is expected to create a QuicTransport before sending publish or
      // subscription requests.
      const streamPipeline = new QuicTransportStreamPipeline(streamId);
      // If a stream pipeline already exists, just replace the old.
      let pipelineMap;
      if (direction === 'in') {
        pipelineMap = incomingStreamPipelines;
      } else {
        pipelineMap = outgoingStreamPipelines;
      }
      if (pipelineMap.has(streamId)) {
        return ;
      }
      pipelineMap.set(streamId, streamPipeline);
      return streamPipeline;
    };

    var onSuccess = function(callback) {
      return function(result) {
        callback('callback', result);
      };
    };

    var onError = function (callback) {
        return function(reason) {
            if (typeof reason === 'string') {
                callback('callback', 'error', reason);
            } else {
                callback('callback', reason);
            }
        };
    };

    that.getInternalAddress = function(callback) {
        const ip = global.config.internal.ip_address;
        const port = router.internalPort;
        log.info("getInternalAddress ip:", ip, " port:", port);
        callback('callback', {ip, port});
    };

    that.getInfo = function(callback) {
        var info = {
            ip: ip_address,
            port: port
        };
        log.info("get bridge Address ip:", info.ip, " port:", info.port);
        callback('callback', info);
    };

    // functions: publish, unpublish, subscribe, unsubscribe, linkup, cutoff
    that.publish = function(connectionId, connectionType, options, callback) {
        log.debug('publish, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if (router.getConnection(connectionId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
        }

        var conn = null;
        switch (connectionType) {
        case 'mediabridge':
            if (options.originType === 'webrtc') {
                log.info("origin type is webrtc and pubArgs:", options.pubArgs, " and session info is:", clusters[options.cluster].session);
                var sessionid = clusters[options.cluster].session.getId();
                const pubs = options.pubArgs.map(pubArg => new Promise((resolve, reject) => {
                    var pubId = pubArg.id;
                    var connId = 'quic-' + sessionid + '-' + pubId;

                    log.info('connId is:', connId);
                    conn = createFrameSource(connId, 'in', options, callback);
                    if (!conn) {
                        return;
                    }

                    var quicStream = clusters[options.cluster].session.createBidirectionalStream();
                    var streamID = quicStream.getId();
                    quicStream.trackKind = pubArg.type;

                    connections[pubId] = quicStream;

                    quicStream.onStreamData((msg) => {
                      log.info("quic client stream get data:", msg);
                      var event = JSON.parse(msg);
                      if (event.type === 'ready') {
                        var data = {
                            type: 'track',
                            id: connId,
                            kind: pubArg.type
                        }
                        quicStream.send(JSON.stringify(data));

                        options.connectionId = connId;
                        delete pubArg.id;
                        options.media.tracks = [];
                        options.media.tracks.push(pubArg);
                        log.info("quic stream send subscribe options:", options);

                        var info = {
                          type: 'subscribe',
                          options: options
                        }

                        quicStream.send(JSON.stringify(info));
                      }
                    })
                    conn.addInputStream(quicStream);

                    router.addLocalSource(pubId, connectionType, conn);

                    var readyinfo = {
                      type: 'ready'
                    }
                    log.info("quic stream:", streamID, " send ready msg");
                    quicStream.send(JSON.stringify(readyinfo));
                }));
            } else {
                conn = createFrameSource(connectionId, 'in', options, callback);
                if (!conn) {
                    return;
                }

                var quicStream = clusters[options.cluster].session.createBidirectionalStream();
                var streamID = quicStream.getId();

                connections[connectionId] = quicStream;

                conn.addInputStream(quicStream);
                var data = {
                    type: 'track',
                    id: connectionId
                }
                quicStream.send(JSON.stringify(data));

                router.addLocalSource(connectionId, connectionType, conn);

                options.connectionId = connectionId;
                log.info("Create quic stream to receive subscribed stream from remote cluster, clusters:", clusters);

                var info = {
                  type: 'subscribe',
                  options: options
                }

                clusters[options.cluster].signalStream.send(JSON.stringify(info));

            }
            subscriptionOptions.set(connectionId, options);

            
            break;
        default:
            log.error('Connection type invalid:' + connectionType);
        }
        if (!conn) {
            log.error('Create connection failed', connectionId, connectionType);
            return callback('callback', {type: 'failed', reason: 'Create Connection failed'});
        }
        
        onSuccess(callback)();
    };

    that.unpublish = function (connectionId, callback) {
        log.debug('unpublish, connectionId:', connectionId);
        var conn = router.getConnection(connectionId);
        router.removeConnection(connectionId).then(function(ok) {
            if (conn) {
                conn.close();
            }
            callback('callback', 'ok');
        }, onError(callback));
        if (publicationOptions.has(connectionId)) {
          publicationOptions.delete(connectionId);
        }
    };

    that.subscribe = function (connectionId, connectionType, options, callback) {
        if (connectionType == 'mediabridge') {
            if (options.transport && options.transport.type) {
                connectionType = options.transport.type;
            }
        }
        log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if (router.getConnection(connectionId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
        }

        var conn = null;
        switch (connectionType) {
            case 'quic':
                conn = createStreamPipeline(connectionId, 'out', options);
                const stream = quicTransportServer.createSendStream(options.transport.id, connectionId);
                conn.quicStream(stream);
                if (!conn) {
                    return;
                }
                break;
            default:
                log.error('Connection type invalid:' + connectionType);
        }
        if (!conn) {
            log.error('Create connection failed', connectionId, connectionType);
            return callback('callback', {type: 'failed', reason: 'Create Connection failed'});
        }

        router.addLocalDestination(connectionId, connectionType, conn)
        .then(onSuccess(callback), onError(callback));
    };

    that.unsubscribe = function (connectionId, callback) {
        log.debug('unsubscribe, connectionId:', connectionId);
        var conn = router.getConnection(connectionId);
        router.removeConnection(connectionId).then(function(ok) {
            if (conn) {
                conn.close();
            }
            callback('callback', 'ok');
        }, onError(callback));
        var connid = connectionId.split('@')[0];
        log.info("unsubscribe sub connid:", connid, " connections:", connections);
        if (connections[connid]) {
            connections[connid].close();
            delete connections[connid];
        }

    };

    that.linkup = function (connectionId, from, callback) {
        log.debug('linkup, connectionId:', connectionId, 'from:', from);

        router.linkup(connectionId, from).then(onSuccess(callback), onError(callback));
    };

    that.cutoff = function (connectionId, callback) {
        log.debug('cutoff, connectionId:', connectionId);
        router.cutoff(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.close = function() {
        router.clear();
        server.stop();
        Object.keys(clusters).forEach(key => {
            clusters[key].stop();
        });;

    };

    that.onFaultDetected = function (message) {
        router.onFaultDetected(message);
    };

    const createQuicStream = (controller, clusterID, initialized, data) => {
      log.info("Create quic stream with controller:", controller, " clientID:", clientID, " and data:",data);
      var quicStream = clusters[clusterID].session.createBidirectionalStream();
      var streamID = quicStream.getId();

      if (!initialized) {
        //Client create initialized stream to exchange cluster info for this session
        var info = {
          type: 'ready'
        }
        quicStream.send(JSON.stringify(info));
        
        quicStream.onStreamData((msg) => {
          log.info("quic client stream get data:", msg);
          var event = JSON.parse(msg);
          if (event.type === 'ready') {
            var info = {
              type: 'cluster',
              cluster: data.selfCluster
            }
            quicStream.send(JSON.stringify(info));
          }
        })
      } else {
        var info = {
          type: 'subscribe',
          options: data
        }
        quicStream.send(JSON.stringify(info));
        
        quicStream.onStreamData((msg) => {
          log.info("quic client stream get data:", msg);
          var event = JSON.parse(msg);
          if (event.type === 'ready') {
            var info = {
              type: 'cluster',
              cluster: data.selfCluster
            }
            quicStream.send(JSON.stringify(info));
          } else if (event.type === 'publish') {

          }
        })
      }      
    }

    //Work as quic client to proactively establish quic connection with another OWT cluster
    that.startCascading = function (data, callback) {
        log.info("startEventCascading with data:", data);

        //A new conference request between uncascaded clusters
        if(!clusters[data.targetCluster]) {
          return rpcReq.getController(clusterName, data.room)
            .then(function(controller) {
              log.info("Client get controller:", controller);
              var client = new addon.QuicTransportClient(data.mediaIP, data.mediaPort);
              client.connected = false;
              client.connect();
              client.id = data.targetCluster;
              if (!clusters[data.targetCluster]) {
                clusters[data.targetCluster] = {};
              }
              clusters[data.targetCluster].session = client
              client.onConnection(() => {
                log.info("Quic client connected");
                client.connected = true;

                var quicStream = clusters[data.targetCluster].session.createBidirectionalStream();
                var streamID = quicStream.getId();
                clusters[data.targetCluster].signalStream = quicStream;

                log.info("Create quic stream with controller:", controller, " and data:",data, " streamID:", streamID);
                //Client create initialized stream to exchange cluster info for this session
                var info = {
                  type: 'ready'
                }
                quicStream.send(JSON.stringify(info));
                
                quicStream.onStreamData((msg) => {
                  log.info("quic client stream get data:", msg);
                  var event = JSON.parse(msg);
                  if (event.type === 'ready') {
                    var info = {
                      type: 'cluster',
                      room: data.room,
                      cluster: data.selfCluster
                    }
                    quicStream.send(JSON.stringify(info));
                  }
                })
                callback('callback', 'ok');
              });

              client.onNewStream((incomingStream) => {
                var streamId = incomingStream.getId();
                log.info("client get new stream id:", streamId);
                incomingStream.onStreamData((msg) => {
                  var info = JSON.parse(msg);
                  log.info("client get stream data:", info);
                  if (info.type === 'ready') {
                    log.info("Client stream id:", streamId, " get ready msg and send back ready msg");
                    var data = {
                      type: 'ready'
                    }
                    incomingStream.send(JSON.stringify(data));
                  } else if (info.type === 'track') {
                    log.info("Client stream id:", streamId, " get track msg with info:", info);
                    var conn = createStreamPipeline(info.id, 'out', info.options);
                    conn.quicStream(incomingStream);
                    if (!conn) {
                        return;
                    }
                    incomingStream.trackKind = info.kind;
                    var connid = 'quic-' + info.id;
                    router.addLocalDestination(info.id, 'mediabridge', conn);

                  } else if (info.type === 'subscribe') {
                    log.info("Client stream id:", streamId, " get subscribe msg with info:", info);
                    info.options.locality = {agent: parentRpcId, node: selfRpcId};
                    var connectionId = info.options.connectionId;
                    if (controllers[info.options.room]) {
                        rpcReq.subscribe(controllers[info.options.room], 'admin', connectionId, info.options);
                    } else {
                        rpcReq.getController(clusterName, info.options.room)
                        .then(function(controller) {
                        controllers[info.options.room] = controller;
                        log.info("client subscribe to controller:", controller, "connection id:", connectionId, " with info:", info.options.media.tracks);

                        rpcReq.subscribe(controller, 'admin', connectionId, info.options);
                    });
                    }
                  } else if (info.type === 'unsubscribe') {
                    //handle unsubscribe request
                  }
                });
              })

              client.onClosedStream((closedStreamId) => {
                log.info("client stream:", closedStreamId, " is closed");
              })
            });
        } else {
            log.debug('Cluster already cascaded');
            callback('callback', 'ok');
            return Promise.resolve('ok');
        }
    }

    const subscribeFromLocal = (controller, info) => {
        log.info("Subscribe to controller:", controller, " with info", info);
        info.type = 'mediabridge';
        info.locality = {agent: parentRpcId, node: selfRpcId};
        var connectionId = info.connectionId;
        rpcReq.getController(clusterName, info.room)
            .then(function(controller) {
            log.info("Subscribe connection id:", connectionId, " with info:", info.media.tracks);
            rpcReq.subscribe(controller, 'admin', connectionId, info);
        });
    }

    //work as quic server to wait for another OWT cluster to establish quic connection
    const start = function () {
      server = new addon.QuicTransportServer(port, cf, kf);

      server.start();
      server.onNewSession((session) => {
      session.connected = true;

      log.info("Server get new session:");
      session.onNewStream((quicStream) => {
        var streamId = quicStream.getId();
        log.info("Server get new stream id:", streamId);
        quicStream.onStreamData((msg) => {
          var info = JSON.parse(msg);
          log.info("Server get stream data:", info);
          if (info.type === 'cluster') {
            session.id = info.cluster;
            if (!clusters[info.cluster]) {
                clusters[info.cluster] = {};
            }
            clusters[info.cluster].session = session;
            clusters[info.cluster].signalStream = quicStream;
            rpcReq.getController(clusterName, info.room)
            .then(function(controller) {
               quicStream.controller = controller; 
            });
          } else if (info.type === 'ready') {
            log.info("Server stream id:", streamId, " get ready msg and send back ready msg");
            var data = {
              type: 'ready'
            }
            quicStream.send(JSON.stringify(data));
          } else if (info.type === 'track') {
            log.info("Server stream id:", streamId, " get track msg and track info:", info);
            var conn = createStreamPipeline(info.id, 'out', info.options);
            conn.quicStream(quicStream);
            if (!conn) {
                return;
            }
            quicStream.trackKind = info.kind;
            var connid = 'quic-' + info.id;
            router.addLocalDestination(info.id, 'mediabridge', conn);

          } else if (info.type === 'subscribe') {
            log.info("Server stream id:", streamId, " get subscribe msg with subscribe info:", info);
            info.options.locality = {agent: parentRpcId, node: selfRpcId};
            var connectionId = info.options.connectionId;
            if (controllers[info.options.room]) {
                rpcReq.subscribe(controllers[info.options.room], 'admin', connectionId, info.options); 
            } else {
                rpcReq.getController(clusterName, info.options.room)
                .then(function(controller) {
                    controllers[info.options.room] = controller;
                    log.info("Subscribe to controller:", controller, "connection id:", connectionId, " with info:", info.options.media.tracks);

                    return rpcReq.subscribe(controller, 'admin', connectionId, info.options);
                })
                .then(function(result) {
                    log.info("subscribe result is:", result);

                })
                .catch((e) => {
                    log.info("subscribe failed with error:", e);
                });
            }
          } else if (info.type === 'unsubscribe') {
            //handle unsusbcribe request
          }
        });
      })

      session.onClosedStream((closedStreamId) => {
        log.info("server stream:", closedStreamId, " is closed");
      })
    });
  }

  start();

    return that;
};
