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
const audioTrackId = '00000000000000000000000000000001';
const videoTrackId = '00000000000000000000000000000002';

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
    const frameSourceMap =
        new Map();  // Key is publication ID, value is WebTransportFrameSource.
    const publicationOptions = new Map();
    const subscriptionOptions = new Map();
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

    // for algorithms that generate new stream
  const generateStream = (controller, streamId, streamInfo) => {
    log.debug('generateStream:', controller, streamId, streamId);
    do_publish(controller, 'admin', streamId, streamInfo)
      .then(() => {
        log.debug('Generated stream:', streamId, 'publish success');
      })
      .catch((err) => {
        log.debug('Generated stream:', streamId, 'publish failed', err);
      });
  };

    const notifyStatus = (controller, sessionId, direction, status) => {
      rpcClient.remoteCast(
          controller, 'onSessionProgress', [sessionId, direction, status]);
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
        if (outgoingStreamPipelines.has(stream.contentSessionId)) {
          const pipeline = outgoingStreamPipelines.get(stream.contentSessionId);
          pipeline.quicStream(stream);
        } else if (frameSourceMap.has(stream.contentSessionId)) {
          if (!publicationOptions.has(stream.contentSessionId)) {
            log.warn(
                'Cannot find publication options for session ' +
                stream.contentSessionId);
            stream.close();
          }
          const options = publicationOptions.get(stream.contentSessionId);
          // Only publications for media have tracks.
          if (options.tracks && options.tracks.length) {
            stream.readTrackId();
          } else {
            stream.trackKind = 'data';
            frameSourceMap.get(stream.contentSessionId).addInputStream(stream);
          }
        } else {
          log.warn(
              'Cannot find a pipeline for QUIC stream. Content session ID: ' +
              stream.contentSessionId);
          stream.close();
          return;
        }
      });
      quicTransportServer.on('connectionadded', (connection) => {
        log.debug(
            'A connection for transport ID ' + connection.transportId +
            ' is created.');
      });
    });

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

    var do_publish = function(conference_ctl, user, stream_id, stream_info) {
      return new Promise(function(resolve, reject) {
          makeRPC(
              rpcClient,
              conference_ctl,
              'publish',
              [user, stream_id, stream_info],
              resolve,
              reject);
      });
    }

    var do_subscribe = function(conference_ctl, user, subscription_id, subInfo) {
      return new Promise(function(resolve, reject) {
          makeRPC(
              rpcClient,
              conference_ctl,
              'subscribe',
              [user, subscription_id, subInfo],
              resolve,
              reject);
      });
    }

    var do_unpublish = function(conference_ctl, user, stream_id) {
      return new Promise(function(resolve, reject) {
          makeRPC(
              rpcClient,
              conference_ctl,
              'unpublish',
              [user, stream_id],
              resolve,
              reject);
      });
    }

    var do_unsubscribe = function(conference_ctl, user, subscription_id) {
      return new Promise(function(resolve, reject) {
          makeRPC(
              rpcClient,
              conference_ctl,
              'unsubscribe',
              [user, subscription_id],
              resolve,
              reject);
      });
    }

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
            publicationOptions.set(connectionId, options);
            conn = createFrameSource(connectionId, 'in', options, callback);
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
        router.addLocalSource(connectionId, connectionType, conn);
        onSuccess(callback)();
        notifyStatus(options.controller, connectionId, 'in', {
          type: 'ready',
          audio: undefined,
          video: undefined,
          data: true,
          simulcast: false
        });
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

        if (server_controllers[options.controller]) {

        } else if(client_controllers[options.controller]) {

        } else {

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

    that.close = function() {
        router.clear();
    };

    that.onFaultDetected = function (message) {
        router.onFaultDetected(message);
    };

    that.startEventCascading = function (data) {
    var clientID = data.evIP.replace(/\./g, '-') + '-' + data.evPort;
    log.info("startEventCascading with data:", data, " clientID:", clientID);
    if(!clients[clientID]) {
      const controller = await getConferenceController(data.room);
      var client = new quicCas.out(data.evIP, data.evPort);
      clients[clientID] = true;
      client.addEventListener('newstream', function (msg) {
        var info = JSON.parse(msg);
        log.info("Get new stream:", info);
        client.controller = controller;
        var quicinfo = {session: info.sessionId, stream: info.streamId, client: client};
        if (!client_controllers[controller]) {
          client_controllers[controller] = {};
        }

        if (!client_controllers[controller][clientID]) {
          client_controllers[controller][clientID] = {};
        }
        client_controllers[controller][clientID] = quicinfo;
      });
    } else {
      log.debug('Cluster already cascaded');
      return Promise.resolve('ok');
    }
  }

    return that;
};
