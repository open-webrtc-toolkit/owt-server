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
var log = logger.getLogger('RecordingNode');

var {InternalConnectionRouter} = require('./internalConnectionRouter');


module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    var that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    };
    var connections = new Connections;
    var router = new InternalConnectionRouter(global.config.internal);

    var notifyStatus = (controller, sessionId, direction, status) => {
        rpcClient.remoteCast(controller, 'onSessionProgress', [sessionId, direction, status]);
    };

    var createFileIn = function (options, callback) {
        var avstream_options = {type: 'file',
                                url: options.url};

        var connection = new AVStreamIn(avstream_options);
        // FIXME: There should be a better chance to start playing.
        setTimeout(function () {connection.startPlay();}, 6000);
        callback('onStatus', {type: 'ready'});

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

    var createFileOut = function (connectionId, options) {
        var recording_path = path.join(global.config.recording.path, connectionId + '.' + options.connection.container);
        var avstream_options = {type: 'file',
                                require_audio: !!options.media.audio,
                                require_video: !!options.media.video,
                                audio_codec: 'opus_48000_2'/*FIXME: should be removed later*/,
                                video_codec: 'h264'/*FIXME: should be removed later*/,
                                url: recording_path,
                                interval: 1000/*FIXME: should be removed later*/,
                                initializeTimeout: global.config.recording.initializeTimeout};

        var connection = new AVStreamOut(avstream_options, function (error) {
            if (error) {
                log.error('media recording init error:', error);
                notifyStatus(options.controller, connectionId, 'out', {type: 'failed', reason: error});
            } else {
                notifyStatus(options.controller, connectionId, 'out', {type: 'ready', info: {host: selfRpcId.split('@')[1].split('_')[0], file: recording_path}});
            }
        });
        connection.addEventListener('fatal', function (error) {
            log.error('media recording error:', error);
            notifyStatus(options.controller, connectionId, 'out', {type: 'failed', reason: 'recording fatal error: ' + error});
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
            case 'recording':
                conn = createFileIn(options, callback);
                break;
            default:
                log.error('Connection type invalid:' + connectionType);
        }
        if (!conn) {
            log.error('Create connection failed', connectionId, connectionType);
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
            case 'recording':
                conn = createFileOut(connectionId, options);
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

    return that;
};
