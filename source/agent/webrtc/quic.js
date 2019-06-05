// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
const path = require('path');
const Connections = require('./connections');
const InternalConnectionFactory = require('./InternalConnectionFactory');
const logger = require('../logger').logger;
const QuicConnection = require('./quicConnection');
const log = logger.getLogger('WebrtcQuicNode');
const addon = require('../quic/build/Release/webrtc-quic');

log.info('WebRTC QUIC node.')

const threadPool = new addon.ThreadPool(global.config.webrtc.num_workers || 24);
threadPool.start();

// We don't use Nicer connection now
const ioThreadPool = new addon.IOThreadPool(global.config.webrtc.io_workers || 1);

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
    const pubSubMap = new Map();  // Key is publication, value is an array of subscription array

    var notifyStatus = (controller, sessionId, direction, status) => {
        rpcClient.remoteCast(controller, 'onSessionProgress', [sessionId, direction, status]);
    };

    const createQuicConnection = function (connectionId, direction, options) {
        var connection = new QuicConnection(connectionId, function (status) {
            notifyStatus(options.controller, connectionId, direction, status);
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
        callback('callback', {ip: that.clusterIP, port: portInfo});
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
            case 'quic-p2p':
                log.debug("New QUIC connection.");
                conn = createQuicConnection(connectionId, 'in', options);
                pubSubMap.set(connectionId, []);
                // Forward data from publication to subscriptions.
                /*
                conn.ondata=(data)=>{
                    log.debug('conn.ondata');
                    if(pubSubMap.has(connectionId)){
                        log.debug('pubSubMap has connectionID.');
                        for(const sub of pubSubMap.get(connectionId)){
                            log.debug('Writing data.'+data);
                            sub.write(data);
                        }
                    }
                }*/
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
            case 'webrtc':
            case 'quic-p2p':
                log.debug("New QUIC connection.");
                conn = createQuicConnection(connectionId, 'out', options);
                if(!pubSubMap.has(options.data.from)){
                    return callback('callback', {type: 'failed', reason: 'Invalid data source.'});
                }
                log.debug('Add subscription to '+options.data.from);
                pubSubMap.get(options.data.from).push(conn);
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
        return;
        // Rename |videoFrom| to |dataFrom| for data stream. |audioFrom| is always undefined here.
        // Consider to add |dataFrom|.
        log.debug('linkup, connectionId:', connectionId, 'dataFrom:', dataFrom);
        const publicationConn=connections.getConnection(dataFrom);
        const subscriptionConn=connections.getConnection(connectionId);
        if(!publicationConn||!subscriptionConn){
            log.error('Invalid subscription request.');
            return;
        }
        if (!pubSubMap.has(publicationConn)) {
            pubSubMap.set(publicationConn, []);
        }
        pubSubMap.get(publicationConn).push(subscriptionConn);
    };

    that.cutoff = function (connectionId, callback) {
        log.debug('cutoff, connectionId:', connectionId);
        connections.cutoffConnection(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.onSessionSignaling = function (connectionId, msg, callback) {
        log.debug('onSessionSignaling, connection id:', connectionId, 'msg:', msg);
        var conn = connections.getConnection(connectionId);
        if (conn) {
            if (conn.type === 'quic-p2p' || conn.type === 'webrtc') { //NOTE: Only webrtc connection supports signaling.
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
            if (conn.type === 'quic-p2p') {//NOTE: Only webrtc connection supports media-on-off
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
