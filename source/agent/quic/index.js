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
    const frameDestinationMap = new Map();  // Key is subscription ID, value is
                                            // WebTransportFrameDestination.
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
      // Event name follows WebRTC agent.
      // A new status "rtp" is added for RTP configuration. It would be good if
      // this configuration is included in the response of a subscribe request.
      // However, it may change a lot to achieve it, so we extend
      // onSessionProgress for it. In the long term, WebTransport connections
      // will have its own mechanism to accept publish or subscribe requests,
      // possibly via REST, which doesn't depend on Socket.IO connection. At
      // that time, RTP configuration can be included in response.
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
            frameSourceMap.get(stream.contentSessionId).addStreamInput(stream);
          }
        } else {
          log.warn(
              'Cannot find a pipeline for QUIC stream. Content session ID: ' +
              stream.contentSessionId);
          stream.close();
          return;
        }
      });
      quicTransportServer.on('trackid', (stream) => {
        // TODO: Defined track ID and get track kind based on ID. Currently it
        // is hard coded.
        if (stream.trackId === audioTrackId) {
          stream.trackKind = 'audio';
        } else if (stream.trackId === videoTrackId) {
          stream.trackKind = 'video';
        } else {
          log.warn('Unexpected track ID: ' + stream.trackId);
          return;
        }
        if (frameSourceMap.has(stream.contentSessionId)) {
          frameSourceMap.get(stream.contentSessionId).addStreamInput(stream);
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
        return;
      }
      const frameSource = new addon.WebTransportFrameSource(streamId);
      frameSourceMap.set(streamId, frameSource);
      return frameSource;
    };

    const createFrameDestination = (subscriptionId, options, isDatagram) => {
      if (frameDestinationMap.has(subscriptionId)) {
        log.warn('Frame destination for ' + subscriptionId + ' exists.');
        return;
      }
      const frameDestination =
          new addon.WebTransportFrameDestination(subscriptionId, isDatagram);
      frameDestinationMap.set(subscriptionId, frameDestination);
      return frameDestination;
    };

    const createStreamPipeline = function(
        streamId, direction, options) {
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
        log.warn('Pipeline for ' + streamId + ' exists.');
        return;
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
    that.publish = async function(connectionId, connectionType, options, callback) {
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
        try {
            await router.addLocalSource(connectionId, connectionType, conn);
        } catch (e) {
            onError(callback)();
            return;
        }
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
              if (typeof conn.close === 'function') {
                conn.close();
              }
            }
            callback('callback', 'ok');
        }, onError(callback));
        if (publicationOptions.has(connectionId)) {
          publicationOptions.delete(connectionId);
        }
    };

    that.subscribe = async function(connectionId, connectionType, options, callback) {
      if (connectionType == 'quic') {
        if (options.transport && options.transport.type) {
          connectionType = options.transport.type;
        }
      }
      log.debug(
          'subscribe, connectionId:', connectionId,
          'connectionType:', connectionType, 'options:', options);
      if (router.getConnection(connectionId)) {
        return callback('callback', {
          type: 'failed',
          reason: 'Connection already exists:' + connectionId
        });
      }

      let conn = null;
      switch (connectionType) {
        case 'quic':
          if (options.tracks && options.tracks.length) {  // Media.
            // Sending media to client over reliable stream or unreliable
            // datagram.
            let isDatagrame = true;
            if (global.config.quic.mediaOutMode === 'stream') {
              isDatagrame = false;
            }
            conn = createFrameDestination(connectionId, options, isDatagrame);
            const webTransportConnection =
                quicTransportServer.getConnection(options.transport.id);
            if (isDatagrame) {
              webTransportConnection.onclose = () => {
                conn.removeDatagramOutput(webTransportConnection);
              };
              conn.addDatagramOutput(webTransportConnection);
            } else {
              for (const track of options.tracks) {
                const trackId =
                    track.type === 'audio' ? audioTrackId : videoTrackId;
                const stream = quicTransportServer.createSendStream(
                    options.transport.id, connectionId, trackId);
                stream.onclose = () => {
                  conn.removeStreamOutput(connectionId);
                };
                conn.addStreamOutput(trackId, stream);
              }
            }
          } else {
            // TODO: Remove StreamPipeline, move to WebTransportFrameDestination.
            conn = createStreamPipeline(connectionId, 'out', options, callback);
            const stream = quicTransportServer.createSendStream(
                options.transport.id, connectionId);
            conn.quicStream(stream);
          }
          if (!conn) {
            return;
          }
          // RTP info only available when output is datagram. It's undefined
          // when output is stream. A client considers it as reliable stream
          // when it's undefined.
          notifyStatus(
              options.controller, connectionId, 'out',
              {type: 'rtp', rtp: conn.rtpConfig, datagram: false});
          break;
        default:
          log.error('Connection type invalid:' + connectionType);
      }
      if (!conn) {
        log.error('Create connection failed', connectionId, connectionType);
        return callback(
            'callback', {type: 'failed', reason: 'Create Connection failed'});
      }

      try {
        await router.addLocalDestination(connectionId, connectionType, conn);
      } catch (e) {
        onError(callback)();
      }
      onSuccess(callback)();
      notifyStatus(options.controller, connectionId, 'out', {
        type: 'ready',
        audio: undefined,
        video: undefined,
        data: true,
        simulcast: false
      });
    };

    that.unsubscribe = function (connectionId, callback) {
        log.debug('unsubscribe, connectionId:', connectionId);
        var conn = router.getConnection(connectionId);
        router.removeConnection(connectionId).then(function(ok) {
            if (conn) {
              if (typeof conn.close === 'function') {
                conn.close();
              }
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
