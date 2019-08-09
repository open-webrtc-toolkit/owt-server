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

var addon = require('../webrtcLib/build/Release/webrtc');

var threadPool = new addon.ThreadPool(global.config.webrtc.num_workers || 24);
threadPool.start();

// We don't use Nicer connection now
var ioThreadPool = new addon.IOThreadPool(global.config.webrtc.io_workers || 1);

if (global.config.webrtc.use_nicer) {
  log.info('Starting ioThreadPool');
  ioThreadPool.start();
}

module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    var that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    };
    var connections = new Connections;
    var internalConnFactory = new InternalConnectionFactory;

    // connectionId => { rid: simId }
    var simConnectionMap = new Map();
    // simId => {connctionId, rid}
    var simRidMap = new Map();

    // simId is a constructed streamId to identify simulcast stream in controller
    var addSimulcast = (connectionId, rid, controller) => {
        var simId, simIds;
        if (!simConnectionMap.has(connectionId)) {
            simConnectionMap.set(connectionId, {});
        }
        simIds = simConnectionMap.get(connectionId);
        if (!simIds[rid]) {
            // generate a streamId for alternative simulcast stream
            simId = connectionId + '-' + rid;
            simIds[rid] = simId;
            simConnectionMap.set(connectionId, simIds);
            simRidMap.set(simId, {connectionId, rid});

            if (connections.getConnection(connectionId)) {
                const conn = connections.getConnection(connectionId).connection;
                const alternative = conn.getAlternative(rid);
                connections.addConnection(simId, 'webrtc', controller, alternative, 'in');
            }
        }
        log.debug('add sim Id:', simIds[rid]);
        return simIds[rid];
    };

    var notifyStatus = (controller, sessionId, direction, status) => {
        rpcClient.remoteCast(controller, 'onSessionProgress', [sessionId, direction, status]);
    };

    var notifyMediaUpdate = (controller, sessionId, direction, mediaUpdate) => {
        if (typeof mediaUpdate.rid === 'string') {
            mediaUpdate.simId = addSimulcast(sessionId, mediaUpdate.rid, controller);
        }
        rpcClient.remoteCast(controller, 'onMediaUpdate', [sessionId, direction, mediaUpdate]);
    };

    var createWebRTCConnection = function (connectionId, direction, options, callback) {
        var connection = new WrtcConnection({
            connectionId: connectionId,
            threadPool: threadPool,
            ioThreadPool: ioThreadPool,
            direction: direction,
            media: options.media,
            formatPreference: options.formatPreference,
            network_interfaces: global.config.webrtc.network_interfaces
        }, function (status) {
            notifyStatus(options.controller, connectionId, direction, status);
        }, function (mediaUpdate) {
            notifyMediaUpdate(options.controller, connectionId, direction, JSON.parse(mediaUpdate));
        });

        return connection;
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

    // functions: publish, unpublish, subscribe, unsubscribe, linkup, cutoff
    that.publish = function (connectionId, connectionType, options, callback) {
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
            case 'webrtc':
                conn = createWebRTCConnection(connectionId, 'in', options, callback);
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
        if (simConnectionMap.has(connectionId)) {
            const simIds = simConnectionMap.get(connectionId);
            for (const rid in simIds) {
                connections.removeConnection(simIds[rid])
                    .catch((reason) => log.warn('remove simulcast:', reason));
            }
        }
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
        log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
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
            case 'webrtc':
                conn = createWebRTCConnection(connectionId, 'out', options, callback);
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

    that.linkup = function (connectionId, audioFrom, videoFrom, callback) {
        log.debug('linkup, connectionId:', connectionId, 'audioFrom:', audioFrom, 'videoFrom:', videoFrom);
        connections.linkupConnection(connectionId, audioFrom, videoFrom).then(onSuccess(callback), onError(callback));
    };

    that.cutoff = function (connectionId, callback) {
        log.debug('cutoff, connectionId:', connectionId);
        connections.cutoffConnection(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.onSessionSignaling = function (connectionId, msg, callback) {
        log.debug('onSessionSignaling, connection id:', connectionId, 'msg:', msg);
        var conn = connections.getConnection(connectionId);
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

    that.mediaOnOff = function (connectionId, track, direction, action, callback) {
        log.debug('mediaOnOff, connection id:', connectionId, 'track:', track, 'direction:', direction, 'action:', action);
        var conn = connections.getConnection(connectionId);
        if (conn) {
            if (conn.type === 'webrtc') {//NOTE: Only webrtc connection supports media-on-off
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
    };

    that.onFaultDetected = function (message) {
        connections.onFaultDetected(message);
    };

    return that;
};
