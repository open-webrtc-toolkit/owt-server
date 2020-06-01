// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var path = require('path');

var WrtcConnection = require('./wrtcConnection');
var Connections = require('./connections');
var InternalConnectionFactory = require('./InternalConnectionFactory');
var logger = require('../logger').logger;

// Logger
var log = logger.getLogger('WebrtcNode');

var addon = require('../rtcConn/build/Release/rtcConn.node');

var threadPool = new addon.ThreadPool(global.config.webrtc.num_workers || 24);
threadPool.start();

// ThreadPool for libnice connection's main loop
var ioThreadPool = new addon.IOThreadPool(global.config.webrtc.io_workers || 8);
ioThreadPool.start();

module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    var that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    };
    var connections = new Connections;
    var internalConnFactory = new InternalConnectionFactory;

    // Map { transportId => WrtcConnection }
    var peerConnections = new Map();
    // Map { publicTrackId => WrtcTrack }
    var mediaTracks = new Map();
    // Map { transportId => Map { trackId => publicTrackId } }
    var mappingPublicId = new Map();
    // Map { operationId => transportId }
    var mappingTransports = new Map();
    
    var notifyTransportStatus = function (controller, transportId, status) {
        rpcClient.remoteCast(controller, 'onTransportProgress', [transportId, status]);
    };

    var notifyMediaUpdate = function (controller, publicTrackId, direction, mediaUpdate) {
        rpcClient.remoteCast(controller, 'onMediaUpdate', [publicTrackId, direction, mediaUpdate]);
    };

    /* updateInfo = {
     *  event: ('track-added' | 'track-removed' | 'track-updated'),
     *  trackId, mediaType, mediaFormat, active }
     */
    var notifyTrackUpdate = function (controller, transportId, updateInfo) {
        rpcClient.remoteCast(controller, 'onTrackUpdate', [transportId, updateInfo]);
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
            connections.addConnection(publicTrackId, 'webrtc', controller, track, track.direction)
            .catch(e => log.warn('Unexpected error during track add:', e));

            // Bind media-update handler
            track.on('media-update', (jsonUpdate) => {
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
            log.warn('notifyTrackUpdate', controller, publicTrackId, updateInfo);
            notifyTrackUpdate(controller, transportId, updateInfo);

        } else if (trackInfo.type === 'track-removed') {
            publicTrackId = mappingPublicId.get(transportId).get(trackInfo.trackId);
            if (!mediaTracks.has(publicTrackId)) {
                log.error('Non-exist public track id:', publicTrackId, transportId, trackInfo.trackId);
                return;
            }
            connections.removeConnection(publicTrackId)
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

    var createWebRTCConnection = function (transportId, controller) {
        if (peerConnections.has(transportId)) {
            log.warn('PeerConnection already created:', transportId);
            return peerConnections.get(transportId);
        }
        var connection = new WrtcConnection({
            connectionId: transportId,
            threadPool: threadPool,
            ioThreadPool: ioThreadPool,
            network_interfaces: global.config.webrtc.network_interfaces
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
            connections.removeConnection(publicTrackId)
            .catch(e => log.warn('Unexpected error during track destroy:', e));
            mediaTracks.get(publicTrackId).close();
            mediaTracks.delete(publicTrackId);
            // Notify controller
            const updateInfo = {
                event: 'track-removed',
                trackId: trackId,
            };
            const controller = peerConnections.get(transportId).controller
            notifyTrackUpdate(controller, publicTrackId, updateInfo);
        });
        mappingPublicId.delete(transportId);
        peerConnections.get(transportId).close();
        peerConnections.delete(transportId);
        callback('callback', 'ok');
    };

    that.createInternalConnection = function (connectionId, direction, internalOpt, callback) {
        internalOpt.minport = global.config.internal.minport;
        internalOpt.maxport = global.config.internal.maxport;
        var portInfo = internalConnFactory.create(connectionId, direction, internalOpt);
        callback('callback', {ip: global.config.internal.ip_address, port: portInfo});
    };

    that.destroyInternalConnection = function (connectionId, direction, callback) {
        internalConnFactory.destroy(connectionId, direction);
        callback('callback', 'ok');
    };

    /*
     * For operations on type webrtc, publicTrackId is connectionId.
     * For operations on type internal, operationId is connectionId.
     */
    // functions: publish, unpublish, subscribe, unsubscribe, linkup, cutoff
    // options = { transportId, tracks = [{mid, type, formatPreference}], controller}
    that.publish = function (operationId, connectionType, options, callback) {
        log.debug('publish, operationId:', operationId, 'connectionType:', connectionType, 'options:', options);
        if (connections.getConnection(operationId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+operationId});
        }

        var conn = null;
        if (connectionType === 'internal') {
            conn = internalConnFactory.fetch(operationId, 'in');
            if (conn) {
                conn.connect(options);
                connections.addConnection(operationId, connectionType, options.controller, conn, 'in')
                .then(onSuccess(callback), onError(callback));
            }
        } else if (connectionType === 'webrtc') {
            if (!options.transportId) {
                // Generate a transportId

            }
            conn = createWebRTCConnection(options.transportId, options.controller);
            options.tracks.forEach(function trackOp(t) {
                conn.addTrackOperation(operationId, t.mid, t.type, 'sendonly', t.formatPreference);
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
        } else {
            conn = connections.getConnection(operationId);
            connections.removeConnection(operationId).then(function(ok) {
                if (conn && conn.type === 'internal') {
                    internalConnFactory.destroy(operationId, 'in');
                }
                callback('callback', 'ok');
            }, onError(callback));
        }
    };

    that.subscribe = function (operationId, connectionType, options, callback) {
        log.debug('subscribe, operationId:', operationId, 'connectionType:', connectionType, 'options:', options);
        if (connections.getConnection(operationId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+operationId});
        }

        var conn = null;
        switch (connectionType) {
            case 'internal':
                conn = internalConnFactory.fetch(connectionId, 'out');
                if (conn)
                    conn.connect(options);//FIXME: May FAIL here!!!!!
                break;
            case 'webrtc':
                conn = createWebRTCConnection(connectionId, 'out', options, callback);
                break;
            default:
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
        } else {
            conn = connections.getConnection(operationId);
            connections.removeConnection(operationId).then(function(ok) {
                if (conn && conn.type === 'internal') {
                    internalConnFactory.destroy(operationId, 'out');
                }
                callback('callback', 'ok');
            }, onError(callback));
        }
    };

    that.linkup = function (connectionId, audioFrom, videoFrom, dataFrom, callback) {
        log.debug('linkup, connectionId:', connectionId, 'audioFrom:', audioFrom, 'videoFrom:', videoFrom, 'callback:', callback);
        connections.linkupConnection(connectionId, audioFrom, videoFrom).then(onSuccess(callback), onError(callback));
    };

    that.cutoff = function (connectionId, callback) {
        log.debug('cutoff, connectionId:', connectionId);
        connections.cutoffConnection(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.onTransportSignaling = function (connectionId, msg, callback) {
        log.debug('onTransportSignaling, connection id:', connectionId, 'msg:', msg);
        var conn = getWebRTCConnection(connectionId);
        if (conn) {
            if (conn.type === 'webrtc') {//NOTE: Only webrtc connection supports signaling.
                conn.connection.onSignalling(msg);
                callback('callback', 'ok');
            } else {
                log.info('signaling on non-webrtc connection');
                callback('callback', 'error', 'signaling on non-webrtc connection');
            }
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
                    log.warn('got on off track:', trackId);
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

    that.setVideoBitrate = function (connectionId, bitrate, callback) {
        log.debug('setVideoBitrate, connection id:', connectionId, 'bitrate:', bitrate);
        var conn = connections.getConnection(connectionId);
        if (conn && conn.direction === 'in') {
            if (conn.type === 'webrtc') {//NOTE: Only webrtc connection supports setting video bitrate.
                conn.connection.setVideoBitrate(bitrate, function () {
                  callback('callback', 'ok');
                }, function (err_reason) {
                    log.info('set video bitrate failed:', err_reason);
                    callback('callback', 'error', err_reason);
                });
            } else {
                callback('callback', 'error', 'setVideoBitrate on non-webrtc connection');
            }
        } else {
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
            } else if (conn) {
                conn.connection.close();
            }
        }
        peerConnections.forEach(pc => {
            pc.close();
        });
    };

    that.onFaultDetected = function (message) {
        connections.onFaultDetected(message);
    };

    return that;
};
