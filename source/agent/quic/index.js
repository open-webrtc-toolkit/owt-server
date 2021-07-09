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
const logger = require('../logger').logger;
const QuicTransportServer = require('./webtransport/quicTransportServer');
const QuicTransportStreamPipeline =
    require('./webtransport/quicTransportStreamPipeline');
const log = logger.getLogger('QuicNode');
const addon = require('./build/Release/quic');
const cipher = require('../cipher');
const path = require('path');
const {InternalConnectionRouter} = require('./internalConnectionRouter');

log.info('QUIC transport node.')

module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    const rpcChannel = require('./rpcChannel')(rpcClient);
    const that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    };
    const router = new InternalConnectionRouter(global.config.internal);
    const incomingStreamPipelines =
        new Map();  // Key is publication ID, value is stream pipeline.
    const outgoingStreamPipelines =
        new Map();  // Key is subscription ID, value is stream pipeline.
    let quicTransportServer;
    const clusterName = global && global.config && global.config.cluster ?
        global.config.cluster.name :
        undefined;

    const getConferenceController = async (roomId) => {
      return await rpcChannel.makeRPC(clusterName, 'schedule', [
        'conference', roomId, 'preference' /*TODO: specify preference*/,
        30 * 1000
      ]);
    };

    const getRpcPortal = async (roomId, participantId) => {
      const controllerAgent = await getConferenceController(roomId);
      let controller = null;
      let portal = null;
      const retry = 5;
      const sleep = require('util').promisify(setTimeout);
      for (let i = 0; i < retry; i++) {
        try {
          controller = await rpcChannel.makeRPC(
              controllerAgent.id, 'getNode', [{room: roomId, task: roomId}]);
          if (controller) {
            break;
          }
        } catch (error) {
          sleep(1000);
        }
      }
      portal =
          await rpcChannel.makeRPC(controller, 'getPortal', [participantId])
      return portal;
    };

    const notifyStatus = (controller, sessionId, direction, status) => {
      rpcClient.remoteCast(
          controller, 'onSessionProgress', [sessionId, direction, status]);
    };

    const validateToken = async (token) => {
      if (!token || !token.roomId) {
        throw new TypeError('Invalid token.');
      }
      const portal = await getRpcPortal(token.roomId, token.participantId);
      return rpcChannel.makeRPC(
          portal, 'validateAndDeleteWebTransportToken', [token])
    };

    const keystore = path.resolve(path.dirname(global.config.quic.keystorePath), cipher.kstore);
    cipher.unlock(cipher.k, keystore, (error, password) => {
      if (error) {
        log.error('Failed to read certificate and key.');
        return;
      }
      log.info('path is '+path.resolve(global.config.quic.keystorePath));
      quicTransportServer = new QuicTransportServer(
          addon, global.config.quic.port, path.resolve(global.config.quic.keystorePath),
          password, validateToken);
      quicTransportServer.start();
      quicTransportServer.on('streamadded', (stream) => {
        log.debug(
            'A stream with session ID ' + stream.contentSessionId +
            ' is added.');
        let pipeline = null;
        if (outgoingStreamPipelines.has(stream.contentSessionId)) {
          pipeline = outgoingStreamPipelines.get(stream.contentSessionId);
        } else if (incomingStreamPipelines.has(stream.contentSessionId)) {
          pipeline = incomingStreamPipelines.get(stream.contentSessionId);
        } else {
          log.warn(
              'Cannot find a pipeline for QUIC stream. Content session ID: ' +
              stream.contentSessionId);
          stream.close();
          return;
        }
        pipeline.quicStream(stream);
      });
      quicTransportServer.on('connectionadded', (connection) => {
        log.debug(
            'A connection for transport ID ' + connection.transportId +
            ' is created.');
      });
    });

    const createStreamPipeline =
        function(streamId, direction, options, callback) {
      // Client is expected to create a QuicTransport before sending publish or
      // subscription requests.
      const streamPipeline = new QuicTransportStreamPipeline(streamId, status=>{
        notifyStatus(options.controller, streamId, direction, status)
      });
      // If a stream pipeline already exists, just replace the old.
      let pipelineMap;
      if (direction === 'in') {
        pipelineMap = incomingStreamPipelines;
      } else {
        pipelineMap = outgoingStreamPipelines;
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

    that.getInternalAddress = function(callback) {
        const ip = global.config.internal.ip_address;
        const port = router.internalPort;
        callback('callback', {ip, port});
    };

    // functions: publish, unpublish, subscribe, unsubscribe, linkup, cutoff
    that.publish = function(connectionId, connectionType, options, callback) {
        log.debug('publish, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if (router.getConnection(connectionId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
        }

        var conn = null;
        switch (connectionType) {
        case 'quic':
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
        conn.bindRouterAsSourceCallback = function(stream) {
          router.addLocalSource(connectionId, connectionType, stream);
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
        if (router.getConnection(connectionId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
        }

        var conn = null;
        switch (connectionType) {
            case 'quic':
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
    };

    that.linkup = function (connectionId, from, callback) {
        log.debug('linkup, connectionId:', connectionId, 'from:', from);
        router.linkup(connectionId, from).then(onSuccess(callback), onError(callback));
    };

    that.cutoff = function (connectionId, callback) {
        log.debug('cutoff, connectionId:', connectionId);
        router.cutoff(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.mediaOnOff = function (connectionId, track, direction, action, callback) {
        callback('callback', 'error', 'MediaOnOff is not supported by QUIC agent.');
    };

    that.close = function() {
        router.clear();
    };

    that.onFaultDetected = function (message) {
        router.onFaultDetected(message);
    };

    return that;
};
