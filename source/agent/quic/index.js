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
// all agents. AFAIK, there is no documentation about these interfaces.

'use strict';
const Connections = require('./connections');
const InternalConnectionFactory = require('./InternalConnectionFactory');
const logger = require('../logger').logger;
const QuicTransportServer=require('./webtransport/quicTransportServer');
const QuicTransportStreamPipeline=require('./webtransport/quicTransportStreamPipeline');
const log = logger.getLogger('QuicNode');
const addon = require('./build/Release/quic');

log.info('QUIC transport node.')

module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    var that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    };
    var connections = new Connections;
    var internalConnFactory = new InternalConnectionFactory;
    const incomingStreamPipelines =
        new Map();  // Key is publication ID, value is stream pipeline.
    const outgoingStreamPipeline =
        new Map();  // Key is subscription ID, value is stream pipeline.

    var notifyStatus = (controller, sessionId, direction, status) => {
        rpcClient.remoteCast(controller, 'onSessionProgress', [sessionId, direction, status]);
    };

    const quicTransportServer = new QuicTransportServer();
    quicTransportServer.start();
    quicTransportServer.on('streamadded', (stream) => {
        const conn = connections.getConnection(stream.contentSessionId);
        if (conn) {
            // TODO: verify transport ID.
            conn.connection.quicStream(stream);
        } else {
          log.warn(
              'Cannot find a pipeline for QUIC stream. Content session ID: ' +
              stream.contentSessionId);
          stream.close();
        }
    });

    const createStreamPipeline =
        function(streamId, direction, options, callback) {
      // Client is expected to create a QuicTransport before sending publish or
      // subscription requests.
      const streamPipeline = new QuicTransportStreamPipeline(streamId, status=>{
        notifyStatus(options.controller, streamId, direction, status)
      });
      // If an stream pipeline already exists, just replace the old.
      let pipelineMap;
      if (direction === 'in') {
        pipelineMap = incomingStreamPipelines;
      } else {
        pipelineMap = outgoingStreamPipeline;
      }
      if (pipelineMap.has(streamId)) {
        return callback(
            'callback',
            {type: 'failed', reason: 'Pipeline for ' + streamId + ' exists.'});
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

    that.createInternalConnection = function (connectionId, direction, internalOpt, callback) {
        internalOpt.minport = global.config.internal.minport;
        internalOpt.maxport = global.config.internal.maxport;
        var portInfo = internalConnFactory.create(connectionId, direction, internalOpt);
        callback('callback', {ip: that.clusterIP, port: portInfo});
    };

    that.destroyInternalConnection = function (connectionId, direction, callback) {
        internalConnFactory.destroy(connectionId, direction);
        callback('callback', 'ok');
    };

    // functions: publish, unpublish, subscribe, unsubscribe, linkup, cutoff
    that.publish = function(connectionId, connectionType, options, callback) {
        log.debug('publish, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if (connections.getConnection(connectionId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
        }

        var conn = null;
        switch (connectionType) {
        case 'internal':
            conn = internalConnFactory.fetch(connectionId, 'in');
            if (conn)
                conn.connect(options);
            break;
        case 'quic':
            quicTransportServer.addTransportId(options.transport.id);
            conn = createStreamPipeline(connectionId, 'in', options, callback);
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
        connections.addConnection(connectionId, connectionType, options.controller, conn, 'in')
        .then(onSuccess(callback), onError(callback));
    };

    that.unpublish = function (connectionId, callback) {
        log.debug('unpublish, connectionId:', connectionId);
        var conn = connections.getConnection(connectionId);
        connections.removeConnection(connectionId).then(function(ok) {
            if (conn && conn.type === 'internal') {
                internalConnFactory.destroy(connectionId, 'in');
            } else if (conn) {
                conn.connection.close();
            }
            callback('callback', 'ok');
        }, onError(callback));
    };

    that.subscribe = function (connectionId, connectionType, options, callback) {
        if (connectionType == 'quic') {
            if (options.transport && options.transport.type) {
                connectionType = options.transport.type;
            }
        }
        log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if(!options.data){
            log.error('Subscription request does not include data field.');
        }
        if (connections.getConnection(connectionId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
        }

        var conn = null;
        switch (connectionType) {
            case 'internal':
                conn = internalConnFactory.fetch(connectionId, 'out');
                if (conn)
                    conn.connect(options);//FIXME: May FAIL here!!!!!
                break;
            case 'quic':
                quicTransportServer.addTransportId(options.transport.id);
                conn = createStreamPipeline(connectionId, 'out', options, callback);
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

        connections.addConnection(connectionId, connectionType, options.controller, conn, 'out')
        .then(onSuccess(callback), onError(callback));
    };

    that.unsubscribe = function (connectionId, callback) {
        log.debug('unsubscribe, connectionId:', connectionId);
        var conn = connections.getConnection(connectionId);
        connections.removeConnection(connectionId).then(function(ok) {
            if (conn && conn.type === 'internal') {
                internalConnFactory.destroy(connectionId, 'out');
            } else if (conn) {
                conn.connection.close();
            }
            callback('callback', 'ok');
        }, onError(callback));
    };

    that.linkup = function (connectionId, audioFrom, videoFrom, dataFrom, callback) {
        log.debug('linkup.');
        connections.linkupConnection(connectionId, audioFrom, videoFrom, dataFrom).then(onSuccess(callback), onError(callback));
    };

    that.cutoff = function (connectionId, callback) {
        log.debug('cutoff, connectionId:', connectionId);
        connections.cutoffConnection(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.mediaOnOff = function (connectionId, track, direction, action, callback) {
        log.debug('mediaOnOff, connection id:', connectionId, 'track:', track, 'direction:', direction, 'action:', action);
        var conn = connections.getConnection(connectionId);
        if (conn) {
            if (conn.type === 'quic') {//NOTE: Only webrtc connection supports media-on-off
                conn.connection.onTrackControl(track,
                                                direction,
                                                action,
                                                function () {
                                                    callback('callback', 'ok');
                                                }, function (error_reason) {
                                                    log.info('trac control failed:', error_reason);
                                                    callback('callback', 'error', error_reason);
                                                });
            } else {
                log.info('mediaOnOff on non-webrtc connection');
                callback('callback', 'error', 'mediaOnOff on non-webrtc connection');
            }
        } else {
          log.info('Connection does NOT exist:' + connectionId);
          callback('callback', 'error', 'Connection does NOT exist:' + connectionId);
        }
    };

    that.close = function() {
        log.debug('close called');
        var connIds = connections.getIds();
        for (let connectionId of connIds) {
            var conn = connections.getConnection(connectionId);
            connections.removeConnection(connectionId);
            if (conn && conn.type === 'internal') {
                internalConnFactory.destroy(connectionId, conn.direction);
            } else if (conn && conn.connection) {
                conn.connection.close();
            }
        }
    };

    that.onFaultDetected = function (message) {
        connections.onFaultDetected(message);
    };

    return that;
};
