/*global require, module, GLOBAL*/
'use strict';

var internalIO = require('./internalIO/build/Release/internalIO');
var InternalIn = internalIO.In;
var InternalOut = internalIO.Out;

var avstream = require('./avstreamLib/build/Release/avstream');
var AVStreamIn = avstream.AVStreamIn;
var AVStreamOut = avstream.AVStreamOut;
var logger = require('./logger').logger;
var path = require('path');
var Connections = require('./connections');

// Logger
var log = logger.getLogger('RecordingNode');

var InternalConnectionFactory = require('./InternalConnectionFactory');

module.exports = function () {
    var that = {};
    var connections = new Connections;
    var internalConnFactory = new InternalConnectionFactory;

    var createFileIn = function (options, callback) {
        var avstream_options = {type: 'file',
                                url: options.url};

        var connection = new AVStreamIn(avstream_options);
        // FIXME: There should be a better chance to start playing.
        setTimeout(function () {connection.startPlay();}, 6000);
        callback('onStatus', {type: 'ready'});

        return connection;
    };

    var createFileOut = function (options, callback) {
        var recordingPath = options.path ? options.path : GLOBAL.config.recording.path;
        var avstream_options = {type: 'file',
                                require_audio: !!options.audio,
                                require_video: !!options.video,
                                audio_codec: (options.audio ? options.audio.codecs[0] : undefined),
                                video_codec: (options.video ? options.video.codecs[0] : undefined),
                                url: path.join(recordingPath, options.filename),
                                interval: options.interval};

        var connection = new AVStreamOut(avstream_options, function (error) {
            if (error) {
                log.error('media recording init error:', error);
                callback('onStatus', {type: 'failed', reason: error});
            } else {
                callback('onStatus', {type: 'ready', audio_codecs: (options.audio ? options.audio.codecs : []), video_codecs: (options.video ? options.video.codecs : [])});
            }
        });
        connection.addEventListener('fatal', function (error) {
            log.error('media recording error:', error);
            callback('onStatus', {type: 'failed', reason: 'media recording error: ' + error});
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

    that.createInternalConnection = function (connectionId, direction, internalOpt, callback) {
        internalOpt.minport = GLOBAL.config.internal.minport;
        internalOpt.maxport = GLOBAL.config.internal.maxport;
        var portInfo = internalConnFactory.create(connectionId, direction, internalOpt);
        callback('callback', {ip: that.clusterIP, port: portInfo});
    };

    that.destroyInternalConnection = function (connectionId, direction, callback) {
        internalConnFactory.destroy(connectionId, direction);
        callback('callback', 'ok');
    };

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
        if (connections.getConnection(connectionId)) {
            return callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
        }

        var conn = null;
        switch (connectionType) {
            case 'internal':
                conn = internalConnFactory.fetch(connectionId, 'out');
                if (conn)
                    conn.connect(options);
                break;
            case 'recording':
                conn = createFileOut(options, callback);
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
        connections.linkupConnection(connectionId, audioFrom, videoFrom).then(onSuccess(callback), onError(callback));
    };

    that.cutoff = function (connectionId, callback) {
        connections.cutoffConnection(connectionId).then(onSuccess(callback), onError(callback));
    };

    that.onFaultDetected = function (message) {
        connections.onFaultDetected(message);
    };

    return that;
};
