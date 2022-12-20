// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var path = require('path');
var EventEmitter = require('events').EventEmitter;

var WrtcConnection = require('./wrtcConnection');
var Connections = require('./connections');
var logger = require('../logger').logger;
var {InternalConnectionRouter} = require('./internalConnectionRouter');

// Logger
var log = logger.getLogger('WebrtcNode');

var addon = require('../rtcConn/build/Release/rtcConn.node');
var videoSwitch = require('../videoSwitch/build/Release/videoSwitch.node').VideoSwitch;

var threadPool = new addon.ThreadPool(global.config.webrtc.num_workers || 24);
threadPool.start();

// ThreadPool for libnice connection's main loop
var ioThreadPool = new addon.IOThreadPool(global.config.webrtc.io_workers || 8);
ioThreadPool.start();

const MediaFrameMulticaster = require(
    '../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');
// Setup GRPC server
var createGrpcInterface = require('./grpcAdapter').createGrpcInterface;
var enableGRPC = global.config.agent.enable_grpc || false;

module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    var that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    };
    var connections = new Connections;

    var router = new InternalConnectionRouter(global.config.internal);

    // Map { transportId => WrtcConnection }
    var peerConnections = new Map();
    // Map { publicTrackId => WrtcTrack }
    var mediaTracks = new Map();
    // Map { transportId => Map { trackId => publicTrackId } }
    var mappingPublicId = new Map();
    // Map { operationId => transportId }
    var mappingTransports = new Map();
    // Map { switchId => videoSwitch }
    var switches = new Map();
    var streamingEmitter = new EventEmitter();
    
    var notifyTransportStatus = function (controller, transportId, status) {
        rpcClient.remoteCast(controller, 'onTransportProgress', [transportId, status]);
        // Emit GRPC notifications
        const notification = {
            name: 'onTransportProgress',
            data: {transportId, status},
        };
        streamingEmitter.emit('notification', notification);
    };

    var notifyMediaUpdate = function (controller, publicTrackId, direction, mediaUpdate) {
        rpcClient.remoteCast(controller, 'onMediaUpdate', [publicTrackId, direction, mediaUpdate]);
        // Emit GRPC notifications
        const data = mediaUpdate;
        data.trackId = publicTrackId;
        const notification = {
            name: 'onMediaUpdate',
            data,
        };
        streamingEmitter.emit('notification', notification);
    };

    /* updateInfo = {
     *  event: ('track-added' | 'track-removed' | 'track-updated'),
     *  trackId, mediaType, mediaFormat, active }
     */
    var notifyTrackUpdate = function (controller, transportId, updateInfo) {
        rpcClient.remoteCast(controller, 'onTrackUpdate', [transportId, updateInfo]);
        // Emit GRPC notifications
        if (updateInfo.type === 'track-added') {
            updateInfo[updateInfo.mediaType] = updateInfo.mediaFormat;
        }
        updateInfo.transportId = transportId;
        const notification = {
            name: 'onTrackUpdate',
            data: updateInfo,
        };
        streamingEmitter.emit('notification', notification);
    };

    var handleTrackInfo = function (transportId, trackInfo, controller) {
        var publicTrackId;
        var updateInfo;
        if (trackInfo.type === 'track-added') {
            // Generate public track ID
            const track = trackInfo.track;
            publicTrackId = transportId + '-' + track.id;
            if (mediaTracks.has(publicTrackId)) {
                log.error('Conflict public track id:', publicTrackId, transportId, track.id);
                return;
            }
            mediaTracks.set(publicTrackId, track);
            mappingPublicId.get(transportId).set(track.id, publicTrackId);
            if (track.direction === 'in') {
                const trackSource = track.sender();
                router.addLocalSource(publicTrackId, 'webrtc', trackSource)
                .catch(e => log.warn('Unexpected error during track add:', e));
            } else {
                router.addLocalDestination(publicTrackId, 'webrtc', track)
                .catch(e => log.warn('Unexpected error during track add:', e));
            }

            // Bind media-update handler
            track.on('media-update', (jsonUpdate) => {
                log.debug('notifyMediaUpdate:', publicTrackId, jsonUpdate);
                notifyMediaUpdate(controller, publicTrackId, track.direction, JSON.parse(jsonUpdate));
            });
            // Notify controller
            const mediaType = track.format('audio') ? 'audio' : 'video';
            updateInfo = {
                type: 'track-added',
                trackId: publicTrackId,
                mediaType: track.format('audio') ? 'audio' : 'video',
                mediaFormat: track.format(mediaType),
                direction: track.direction,
                operationId: trackInfo.operationId,
                mid: trackInfo.mid,
                rid: trackInfo.rid,
                active: true,
            };
            log.debug('notifyTrackUpdate', controller, publicTrackId, updateInfo);
            notifyTrackUpdate(controller, transportId, updateInfo);

        } else if (trackInfo.type === 'track-removed') {
            publicTrackId = mappingPublicId.get(transportId).get(trackInfo.trackId);
            if (!mediaTracks.has(publicTrackId)) {
                log.error('Non-exist public track id:', publicTrackId, transportId, trackInfo.trackId);
                return;
            }
            log.debug('track removed:', publicTrackId);
            router.removeConnection(publicTrackId)
            .then(ok => {
                mediaTracks.get(publicTrackId).close();
                mediaTracks.delete(publicTrackId);
                mappingPublicId.get(transportId).delete(trackInfo.trackId);
            })
            .catch(e => log.warn('Unexpected error during track remove:', e));

            // Notify controller
            updateInfo = {
                type: 'track-removed',
                trackId: publicTrackId,
            };
            notifyTrackUpdate(controller, transportId, updateInfo);

        } else if (trackInfo.type === 'tracks-complete') {
            updateInfo = {
                type: 'tracks-complete',
                operationId: trackInfo.operationId
            };
            notifyTrackUpdate(controller, transportId, updateInfo);
        }
    };

    var createWebRTCConnection = function (transportId, controller, owner) {
        if (peerConnections.has(transportId)) {
            log.debug('PeerConnection already created:', transportId);
            return peerConnections.get(transportId);
        }
        var connection = new WrtcConnection({
            connectionId: transportId,
            threadPool: threadPool,
            ioThreadPool: ioThreadPool,
            network_interfaces: global.config.webrtc.network_interfaces,
            owner,
        }, function onTransportStatus(status) {
            notifyTransportStatus(controller, transportId, status);
        }, function onTrack(trackInfo) {
            handleTrackInfo(transportId, trackInfo, controller);
        });

        peerConnections.set(transportId, connection);
        mappingPublicId.set(transportId, new Map());
        connection.controller = controller;
        return connection;
    };

    var getWebRTCConnection = function (operationId) {
        var transportId = mappingTransports.get(operationId);
        if (peerConnections.has(transportId)) {
            return peerConnections.get(transportId);
        }
        return null;
    };

    var onSuccess = function (callback) {
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

    that.createTransport = function (transportId, controller, callback) {
        createWebRTCConnection(transportId, controller);
        callback('callback', 'ok');
    };

    that.destroyTransport = function (transportId, callback) {
        log.debug('destroyTransport, transportId:', transportId);
        if (!peerConnections.has(transportId)) {
            callback('callback', 'error', 'Transport does not exists: ' + transportId);
            return;
        }
        mappingPublicId.get(transportId).forEach((publicTrackId, trackId) => {
            router.removeConnection(publicTrackId)
            .catch(e => log.warn('Unexpected error during track destroy:', e));
            mediaTracks.get(publicTrackId).close();
            mediaTracks.delete(publicTrackId);
            // Notify controller
            const updateInfo = {
                type: 'track-removed',
                trackId: publicTrackId,
            };
            const controller = peerConnections.get(transportId).controller;
            notifyTrackUpdate(controller, publicTrackId, updateInfo);
        });
        mappingPublicId.delete(transportId);
        peerConnections.get(transportId).close();
        peerConnections.delete(transportId);
        callback('callback', 'ok');
    };

    that.getInternalAddress = function(callback) {
        log.debug('Internal PORT for webrtc:', router.internalPort);
        const ip = global.config.internal.ip_address;
        const port = router.internalPort;
        callback('callback', {ip, port});
    };

    /*
     * For operations on type webrtc, publicTrackId is connectionId.
     * For operations on type internal, operationId is connectionId.
     */
    // functions: publish, unpublish, subscribe, unsubscribe, linkup, cutoff
    // options = {
    //   transportId,
    //   tracks = [{mid, type, formatPreference, scalabilityMode}],
    //   controller, owner
    // }
    // formatPreference = {preferred: MediaFormat, optional: [MediaFormat]}
    that.publish = function (operationId, connectionType, options, callback) {
        log.debug('publish, operationId:', operationId, 'connectionType:', connectionType, 'options:', options);
        if (mappingTransports.has(operationId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+operationId});
        }

        var conn = null;
        if (connectionType === 'webrtc') {
            if (!options.transportId) {
                // Generate a transportId

            }
            conn = createWebRTCConnection(options.transportId, options.controller, options.owner);
            options.tracks.forEach(function trackOp(t) {
                conn.addTrackOperation(operationId, 'sendonly', t);
            });
            mappingTransports.set(operationId, options.transportId);
            callback('callback', 'ok');
        } else {
            log.error('Connection type invalid:' + connectionType);
        }

        if (!conn) {
            log.error('Create connection failed', operationId, connectionType);
            callback('callback', {type: 'failed', reason: 'Create Connection failed'});
        }
    };

    that.unpublish = function (operationId, callback) {
        log.debug('unpublish, operationId:', operationId);
        var conn = getWebRTCConnection(operationId);
        if (conn) {
            // For 'webrtc'
            conn.removeTrackOperation(operationId);
            callback('callback', 'ok');
        }
    };

    /*
     * For operations on type webrtc, publicTrackId is connectionId.
     * For operations on type internal, operationId is connectionId.
     */
    // functions: publish, unpublish, subscribe, unsubscribe, linkup, cutoff
    // options = {
    //   transportId,
    //   tracks = [{mid, type, formatPreference, scalabilityMode}],
    //   controller, owner, enableBWE
    // }
    // formatPreference = {preferred: MediaFormat, optional: [MediaFormat]}
    that.subscribe = function (operationId, connectionType, options, callback) {
        log.debug('subscribe, operationId:', operationId, 'connectionType:', connectionType, 'options:', options);
        if (mappingTransports.has(operationId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+operationId});
        }

        var conn = null;
        if (connectionType === 'webrtc') {
            if (!options.transportId) {
                // Generate a transportId

            }
            conn = createWebRTCConnection(options.transportId, options.controller, options.owner);
            options.tracks.forEach(function trackOp(t) {
                conn.addTrackOperation(operationId, 'recvonly', t);
            });
            mappingTransports.set(operationId, options.transportId);
            if (options.enableBWE) {
                conn.enableBWE = true;
            }
            callback('callback', 'ok');
        } else {
            log.error('Connection type invalid:' + connectionType);
        }

        if (!conn) {
            log.error('Create connection failed', operationId, connectionType);
            callback('callback', {type: 'failed', reason: 'Create Connection failed'});
        }
    };

    that.unsubscribe = function (operationId, callback) {
        log.debug('unsubscribe, operationId:', operationId);
        var conn = getWebRTCConnection(operationId);
        if (conn) {
            // For 'webrtc'
            conn.removeTrackOperation(operationId);
            callback('callback', 'ok');
        }
    };

    // streamInfo = {id: 'string', ip: 'string', port: 'number'}
    // from = {audio: streamInfo, video: streamInfo, data: streamInfo}
    that.linkup = function (connectionId, from, callback) {
        log.debug('linkup, connectionId:', connectionId, 'from:', from);
        router.linkup(connectionId, from).then(onSuccess(callback), onError(callback));
    };

    that.cutoff = function (connectionId, callback) {
        log.debug('cutoff, connectionId:', connectionId);
        router.cutoff(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.onTransportSignaling = function (connectionId, msg, callback) {
        log.debug('onTransportSignaling, connection id:', connectionId, 'msg:', msg);
        var conn = getWebRTCConnection(connectionId);
        if (conn) {
            conn.onSignalling(msg, connectionId);
            callback('callback', 'ok');
        } else {
          callback('callback', 'error', 'Connection does NOT exist:' + connectionId);
        }
    };

    that.mediaOnOff = function (connectionId, tracks, direction, action, callback) {
        log.debug('mediaOnOff, connection id:', connectionId, 'tracks:', tracks, 'direction:', direction, 'action:', action);
        var conn = getWebRTCConnection(connectionId);
        var promises;
        if (conn) {
            promises = tracks.map(trackId => new Promise((resolve, reject) => {
                if (mediaTracks.has(trackId)) {
                    log.debug('got on off track:', trackId);
                    mediaTracks.get(trackId).onTrackControl(
                        'av', direction, action, resolve, reject);
                } else {
                    resolve();
                }
            }));
            Promise.all(promises).then(() => {
                callback('callback', 'ok');
            }).catch(reason => {
                callback('callback', 'error', reason);
            });
        } else {
          log.info('WebRTC Connection does NOT exist:' + connectionId);
          callback('callback', 'error', 'Connection does NOT exist:' + connectionId);
        }
    };

    // Create streams from SVC stream with layer settings
    // layerInfo = {spatial: number|boolean, temporal: number|boolean}
    // Return [id] of created layer streams.
    that.createLayerStreams = function (streamId, layerInfo, callback) {
        log.debug('createLayerStreams', streamId, layerInfo);
        var retIds = [];
        var layers = [];
        var track = null;
        var layerStream = null;
        if (mediaTracks.has(streamId)) {
            track = mediaTracks.get(streamId);
            log.debug('scalabilityMode:', track.scalabilityMode);
            if (layerInfo.spatial === true) {
                for (let i = 0; i < parseInt(track.scalabilityMode[1]); i++) {
                    layers.push({spatial: i, temporal: -1});
                }
            } else if (layerInfo.temporal === true) {
                for (let i = 0; i < parseInt(track.scalabilityMode[3]); i++) {
                    layers.push({spatial: -1, temporal: i});
                }
            } else {
                layers.push(layerInfo);
            }
            for (let info of layers) {
                layerStream = track.getOrCreateLayerStream(info, (stream) => {
                    router.removeConnection(stream.id)
                        .catch(e => log.warn('Layer stream destroy:', e));
                });
                if (layerStream) {
                    retIds.push(layerStream.id);
                    if (!router.hasConnection(layerStream.id)) {
                        router.addLocalSource(
                            layerStream.id, 'webrtc', layerStream.sender())
                            .catch(e => log.warn('Add layer stream:', e));
                    }
                }
            }
            callback('callback', retIds);
        } else {
            callback('callback', 'error', 'Stream does NOT exist:', streamId);
        }
    };

    const dispatchers = new Map();

    // Create quality switch for given source streams,
    // which can switch source based on bitrate setting.
    // return created switch ID
    that.createQualitySwitch = function (sourceStreams, callback) {
        log.debug('createQualitySwitch', JSON.stringify(sourceStreams));
        const switchId = Math.round(Math.random() * 1000000000000000000) + '';
        const switchSources = [];
        for (const source of sourceStreams) {
            if (!router.hasConnection(source.id)) {
                router.getOrCreateRemoteSource({
                    id: source.id,
                    ip: source.ip,
                    port: source.port,
                }, function onStats(stat) {
                    if (stat === 'disconnected') {
                        if (switches.get(switchId)) {
                            log.debug('Quality switch source disconnect:', source.id);
                            switches.get(switchId).close();
                            switches.delete(switchId);
                        }
                        router.destroyRemoteSource(source.id);
                    }
                });
            }
            const conn = router.getConnection(source.id);
            if (conn) {
                log.debug('Added switch source:', source.id);
                const dispatcher = new MediaFrameMulticaster();
                const dispatcherId = switchId + switchSources.length;
                dispatchers.set(dispatcherId, dispatcher);
                router.addLocalDestination(
                    dispatcherId, 'dispatcher', dispatcher);
                router.linkup(dispatcherId, {video: {id: source.id}})
                    .catch((e) => log.debug(e));
                switchSources.push(dispatcher.source());
            }
        }
        const dispatcher = new MediaFrameMulticaster();
        dispatchers.set(switchId, dispatcher);
        switchSources.push(dispatcher.source());
        const vswitch = new videoSwitch(...switchSources);
        switches.set(switchId, vswitch);
        router.addLocalSource(switchId, 'webrtc', vswitch)
            .catch((e) => log.warn('Adding switch:', e));
        callback('callback', {id: switchId});
    };

    that.setVideoBitrate = function (connectionId, bitrate, callback) {
        log.debug('setVideoBitrate no longer supported');
    };

    that.close = function() {
        log.debug('close called');
        router.clear();
        peerConnections.forEach(pc => {
            pc.close();
        });
    };

    that.onFaultDetected = function (message) {
        connections.onFaultDetected(message);
    };

    if (enableGRPC) {
        // Export GRPC interface.
        return createGrpcInterface(that, streamingEmitter);
    }
    return that;
};
