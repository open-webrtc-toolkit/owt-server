// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var internalIO = require('../internalIO/build/Release/internalIO');
var InternalIn = internalIO.In;
var InternalOut = internalIO.Out;

var avstream = require('../avstreamLib/build/Release/avstream');
var AVStreamIn = avstream.AVStreamIn;
var AVStreamOut = avstream.AVStreamOut;

const MediaFrameMulticaster = require(
    '../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');

var logger = require('../logger').logger;
var path = require('path');
var Connections = require('./connections');

// Logger
var log = logger.getLogger('StreamingNode');

var {InternalConnectionRouter} = require('./internalConnectionRouter');

// Setup GRPC server
var createGrpcInterface = require('./grpcAdapter').createGrpcInterface;
var enableGRPC = global.config.agent.enable_grpc || false;

var EventEmitter = require('events').EventEmitter;

module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    var that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    };
    var connections = new Connections;
    var router = new InternalConnectionRouter(global.config.internal);
    // For GRPC notifications
    var streamingEmitter = new EventEmitter();

    var notifyStatus = (controller, sessionId, direction, status) => {
        rpcClient.remoteCast(controller, 'onSessionProgress', [sessionId, direction, status]);
        // Emit GRPC notifications
        const notification = {
            name: 'onSessionProgress',
            data: {
                id: sessionId,
                direction,
                status
            }
        };
        streamingEmitter.emit('notification', notification);
    };

    var createAVStreamIn = function (sessionId, options) {
        var avstream_options = {type: 'streaming',
                                has_audio: (options.media.audio === 'auto' ? 'auto' : (!!options.media.audio ? 'yes' : 'no')),
                                has_video: (options.media.video === 'auto' ? 'auto' : (!!options.media.video ? 'yes' : 'no')),
                                transport: options.connection.transportProtocol,
                                buffer_size: options.connection.bufferSize,
                                url: options.connection.url};

        var connection = new AVStreamIn(avstream_options, function (message) {
            log.debug('avstream-in status message:', message);
            notifyStatus(options.controller, sessionId, 'in', JSON.parse(message));
        });

        var dispatcher = new MediaFrameMulticaster();
        var source = dispatcher.source();
        connection.addDestination('audio', dispatcher);
        connection.addDestination('video', dispatcher);
        source.close = function () {
            connection.removeDestination('audio', dispatcher);
            connection.removeDestination('video', dispatcher);
            connection.close();
            dispatcher.close();
        };
        connection.source = function () {
            return source;
        };

        return connection;
    };

    var createAVStreamOut = function (connectionId, options) {
        var avstream_options = {type: 'streaming',
                                require_audio: !!options.media.audio,
                                require_video: !!options.media.video,
                                connection: options.connection,
                                initializeTimeout: global.config.avstream.initializeTimeout};

        if ((options.connection.protocol === 'dash' || options.connection.protocol === 'hls') && !options.connection.url.startsWith('http')) {
            var fs = require('fs');
            if (fs.existsSync(options.connection.url)) {
                log.error('avstream-out init error: file existed.');
                notifyStatus(options.controller, connectionId, 'out', {type: 'failed', reason: 'file existed.'});
                return;
            }
        }

        var connection = new AVStreamOut(avstream_options, function (error) {
            if (error) {
                log.error('avstream-out init error:', error);
                notifyStatus(options.controller, connectionId, 'out', {type: 'failed', reason: error});
            } else {
                notifyStatus(options.controller, connectionId, 'out', {type: 'ready', info: options.connection.url});
            }
        });
        connection.addEventListener('fatal', function (error) {
            if (error) {
                log.error('avstream-out fatal error:', error);
                notifyStatus(options.controller, connectionId, 'out', {type: 'failed', reason: 'avstream_out fatal error: ' + error});
            }
        });

        connection.receiver = function(type) {
            return this;
        };
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

    that.getInternalAddress = function(callback) {
        const ip = global.config.internal.ip_address;
        const port = router.internalPort;
        callback('callback', {ip, port});
    };

    that.publish = function (connectionId, connectionType, options, callback) {
        log.debug('publish, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if (connections.getConnection(connectionId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
        }

        var conn = null;
        switch (connectionType) {
            case 'streaming':
                conn = createAVStreamIn(connectionId, options);
                break;
            default:
                log.error('Connection type invalid:' + connectionType);
        }
        if (!conn) {
            log.error('Create connection failed', connectionId);
            return callback('callback', {type: 'failed', reason: 'Create Connection failed'});
        }

        router.addLocalSource(connectionId, connectionType, conn.source())
        .then(onSuccess(callback), onError(callback));
    };

    that.unpublish = function (connectionId, callback) {
        log.debug('unpublish, connectionId:', connectionId);
        var conn = router.getConnection(connectionId);
        router.removeConnection(connectionId).then(function(ok) {
            conn.close();
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
            case 'streaming':
                conn = createAVStreamOut(connectionId, options);
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
            conn.close();
            callback('callback', 'ok');
        }, onError(callback));
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

    that.close = function() {
        log.debug('close called');
        router.clear();
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
