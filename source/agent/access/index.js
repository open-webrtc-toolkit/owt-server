/*global require, module, GLOBAL*/
'use strict';

var internalIO = require('./internalIO/build/Release/internalIO');
var InternalIn = internalIO.In;
var InternalOut = internalIO.Out;

var avstream = require('./avstream/build/Release/avstream');
var AVStreamIn = avstream.AVStreamIn;
var AVStreamOut = avstream.AVStreamOut;
var logger = require('./logger').logger;
var path = require('path');
var Connections = require('./connections');

// Logger
var log = logger.getLogger('AccessNode');

module.exports = function () {
    var that = {};
    var connections = new Connections;

    var createInternalIn = function (options) {
        var connection = new InternalIn(options.protocol, GLOBAL.config.internal.minport, GLOBAL.config.internal.maxport);
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

    var createAVStreamIn = function (options, callback) {
        var avstream_options = {type: 'avstream',
                                has_audio: !!options.audio,
                                has_video: !!options.video,
                                transport: options.transport,
                                buffer_size: options.buffer_size,
                                url: options.url};

        var connection = new AVStreamIn(avstream_options, function (message) {
            log.debug('avstream-in status message:', message);
            callback('onStatus', JSON.parse(message));
        });

        return connection;
    };

    var createAVStreamOut = function (options, callback) {
        if (options.audio.codecs[0] === 'aac') {
            options.audio.codecs = ['pcm_raw'];
        }
        var avstream_options = {type: 'avstream',
                                require_audio: !!options.audio,
                                require_video: !!options.video,
                                audio_codec: (options.audio ? options.audio.codecs[0] : undefined),
                                video_codec: (options.video ? options.video.codecs[0] : undefined),
                                video_resolution: (options.video ? options.video.resolution : undefined),
                                url: options.url};

        var audio_codec = ((avstream_options.audio_codec && avstream_options.audio_codec === 'aac' ? 'pcm_raw' : avstream_options.audio_codec));
        var connection = new AVStreamOut(avstream_options, function (error) {
            if (error) {
                log.error('avstream-out init error:', error);
                callback('onStatus', {type: 'failed', reason: error});
            } else {
                callback('onStatus', {type: 'ready', audio_codecs: options.audio ? [audio_codec] : [], video_codecs: options.video ? options.video.codecs : []});
            }
        });
        connection.addEventListener('fatal', function (error) {
            if (error) {
                log.error('avstream-out fatal error:', error);
                callback('onStatus', {type: 'failed', reason: 'avstream_out fatal error: ' + error});
            }
        });

        connection.receiver = function(type) {
            return this;
        };
        return connection;
    };

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

    // Add functions: publish, unpublish, subscribe, unsubscribe, linkup, cutoff
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
            } else if (connectionType === 'avstream') {
                conn = createAVStreamIn(options, callback);
            } else if (connectionType === 'recording') {
                conn = createFileIn(options, callback);
            } else {
                log.error('Connection type invalid:' + connectionType);
                callback('callback', {type: 'failed', reason: 'Connection type invalid:' + connectionType});
                return;
            }
        }

        connections.addConnection(connectionId, connectionType, options.controller, conn, 'in')
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
            } else if (connectionType === 'avstream') {
                conn = createAVStreamOut(options, callback);
            } else if (connectionType === 'recording') {
                conn = createFileOut(options, callback);
            } else {
                log.error('Connection type invalid:' + connectionType);
                callback('callback', {type: 'failed', reason: 'Connection type invalid:' + connectionType});
                return;
            }
        }

        connections.addConnection(connectionId, connectionType, options.controller, conn, 'out').then(onSuccess(callback), onError(callback));
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

    that.onFaultDetected = function (message) {
        connections.onFaultDetected(message);
    };

    return that;
};
