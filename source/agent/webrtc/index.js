/*global require, module, GLOBAL*/
'use strict';

var WrtcConnection = require('./wrtcConnection');
var logger = require('./logger').logger;
var path = require('path');
var Connections = require('./connections');

var internalIO = require('./internalIO/build/Release/internalIO');
var InternalIn = internalIO.In;
var InternalOut = internalIO.Out;

// Logger
var log = logger.getLogger('AccessNode');
module.exports = function () {
    var that = {};
    var connections = new Connections;

    var createInternalIn = function (options) {
        var connection = new InternalIn(options.protocol);
        return connection;
    };

    var createInternalOut = function (options) {
        var connection = new InternalOut(options.protocol, options.dest_ip, options.dest_port);

        // Adapter for being consistent with webrtc connection
        connection.receiver = function(type) {
            return this;
        };
        return connection;
    };

    var createWebRTCConnection = function (direction, options, callback) {
        var connection = new WrtcConnection({
            direction: direction,
            audio: options.audio,
            video: options.video,
            private_ip_regexp: that.privateRegexp,
            public_ip: that.publicIP
        }, function (response) {
            callback('onStatus', response);
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

    // functions: publish, unpublish, subscribe, unsubscribe, linkup, cutoff
    that.publish = function (connectionId, connectionType, options, callback) {
        log.debug('publish, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        var conn = connections.getConnection(connectionId);
        if (conn) {
            // Connection already exist
            if (connectionType !== 'internal') {
                log.error('Connection already exists:'+connectionId);
                return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
            }
        } else {
            // Connection not exist
            if (connectionType === 'internal') {
                conn = createInternalIn(options);
            } else if (connectionType === 'webrtc') {
                conn = createWebRTCConnection('in', options, callback);
            } else {
                log.error('Connection type invalid:' + connectionType);
                callback('callback', {type: 'failed', reason: 'Connection type invalid:' + connectionType});
                return;
            }
        }

        connections.addConnection(connectionId, connectionType, conn, 'in')
        .then(function(result) {
            if (connectionType === 'internal') {
                callback('callback', {ip: that.clusterIP, port: conn.getListeningPort()})
            } else {
                callback('callback', 'ok');
            }
        }, onError(callback));
    };

    that.unpublish = function (connectionId, callback) {
        log.debug('unpublish, connectionId:', connectionId);
        connections.removeConnection(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.subscribe = function (connectionId, connectionType, options, callback) {
        log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        var conn = connections.getConnection(connectionId);
        if (conn) {
            // Connection already exist
            if (connectionType !== 'internal') {
                log.error('Connection already exists:'+connectionId);
                return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
            }
        } else {
            // Connection not exist
            if (connectionType === 'internal') {
                conn = createInternalOut(options);
            } else if (connectionType === 'webrtc') {
                conn = createWebRTCConnection('out', options, callback);
            } else {
                log.error('Connection type invalid:' + connectionType);
                callback('callback', {type: 'failed', reason: 'Connection type invalid:' + connectionType});
                return;
            }
        }

        connections.addConnection(connectionId, connectionType, conn, 'out').then(onSuccess(callback), onError(callback));
    };

    that.unsubscribe = function (connectionId, callback) {
        log.debug('unsubscribe, connectionId:', connectionId);
        connections.removeConnection(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.linkup = function (connectionId, audioFrom, videoFrom, callback) {
        connections.linkupConnection(connectionId, audioFrom, videoFrom).then(onSuccess(callback), onError(callback));
    };

    that.cutoff = function (connectionId, callback) {
        connections.cutoffConnection(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.onConnectionSignalling = function (connectionId, msg, callback) {
        log.debug('onConnectionSignalling, connection id:', connectionId, 'msg:', msg);
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
                conn.connection.setVideoBitrate(bitrate);
                callback('callback', 'ok');
            } else {
                callback('callback', 'error', 'setVideoBitrate on non-webrtc connection');
            }
        } else {
          callback('callback', 'error', 'Connection does NOT exist:' + connectionId);
        }
    };

    return that;
};
