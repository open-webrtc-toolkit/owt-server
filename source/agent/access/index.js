/*global require, module, GLOBAL*/
'use strict';

var internalIO = require('./internalIO/build/Release/internalIO');
var avstream = require('./avstream/build/Release/avstream');
var InternalIn = internalIO.In;
var InternalOut = internalIO.Out;
var AVStreamIn = avstream.AVStreamIn;
var AVStreamOut = avstream.AVStreamOut;
var WrtcConnection = require('./wrtcConnection');
var logger = require('./logger').logger;
var amqper = require('./amqper');
var path = require('path');

// Logger
var log = logger.getLogger('AccessNode');

module.exports = function () {
    var that = {},
        /*{ConnectionID: {type: 'webrtc' | 'avstream' | 'recording' | 'internal',
                          direction: 'in' | 'out',
                          audioFrom: ConnectionID | undefined,
                          videoFrom: ConnectionID | undefined,
                          connnection: WebRtcConnection | InternalOut | RTSPConnectionOut
                         }
          }
        */
        connections = {};

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

        callback('callback', 'ok');
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
        callback('callback', 'ok');
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
        callback('callback', 'ok');
        return connection;
    };

    var createFileIn = function (options, callback) {
        var avstream_options = {type: 'file',
                                url: options.url};

        var connection = new AVStreamIn(avstream_options);
        // FIXME: There should be a better chance to start playing.
        setTimeout(function () {connection.startPlay();}, 6000);
        callback('callback', 'ok');
        callback('onStatus', {type: 'ready'});
    };

    var createInternalIn = function (options, callback) {
        var connection = new InternalIn(options.protocol);
        callback('callback', {ip: that.clusterIP, port: connection.getListeningPort()});
        return connection;
    };

    var createInternalOut = function (options, callback) {
        var connection = new InternalOut(options.protocol, options.dest_ip, options.dest_port);
        callback('callback', 'ok');
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
        callback('callback', 'ok');
        return connection;
    };

    var cutOffFrom = function (connectionId) {
        log.debug('remove subscriptions from connection:', connectionId);
        if (connections[connectionId] && connections[connectionId].direction === 'in') {
            for (var connection_id in connections) {
                if (connections[connection_id].direction === 'out') {
                    if (connections[connection_id].audioFrom === connectionId) {
                        log.debug('remove audio subscription:', connections[connection_id].audioFrom);
                        var dest = (connections[connection_id].type === 'webrtc' ? connections[connection_id].connection.receiver('audio') : connections[connection_id].connection);
                        connections[connectionId].connection.removeDestination('audio', dest);
                        connections[connection_id].audioFrom = undefined;
                    }

                    if (connections[connection_id].videoFrom === connectionId) {
                        log.debug('remove video subscription:', connections[connection_id].videoFrom);
                        var dest = connections[connection_id].type === 'webrtc' ? connections[connection_id].connection.receiver('video') : connections[connection_id].connection;
                        connections[connectionId].connection.removeDestination('video', dest);
                        connections[connection_id].videoFrom = undefined;
                    }
                }
            }
        }
    };

    var cutOffTo = function (connectionId) {
        log.debug('remove subscription to connection:', connectionId);
        if (connections[connectionId] && connections[connectionId].direction === 'out') {
            var audioFrom = connections[connectionId].audioFrom,
                videoFrom = connections[connectionId].videoFrom;

            if (audioFrom && connections[audioFrom] && connections[audioFrom].direction === 'in') {
                log.debug('remove audio from:', audioFrom);
                var dest = (connections[connectionId].type === 'webrtc' ? connections[connectionId].connection.receiver('audio') : connections[connectionId].connection);
                connections[audioFrom].connection.removeDestination('audio', dest);
                connections[connectionId].audioFrom = undefined;
            }

            if (videoFrom && connections[videoFrom] && connections[videoFrom].direction === 'in') {
                log.debug('remove video from:', videoFrom);
                var dest = (connections[connectionId].type === 'webrtc' ? connections[connectionId].connection.receiver('video') : connections[connectionId].connection);
                connections[videoFrom].connection.removeDestination('video', dest);
                connections[connectionId].audioFrom = undefined;
            }
        }
    };

    that.publish = function (connectionId, connectionType, options, callback) {
        log.debug('publish, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if (connections[connectionId]) {
            log.error('Connection already exists:'+connectionId);
            callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
            return;
        }

        var conn;

        if (connectionType === 'webrtc') {
            conn = createWebRTCConnection('in', options, callback);
        } else if (connectionType === 'avstream') {
            conn = createAVStreamIn(options, callback);
        } else if (connectionType === 'recording') {
            conn = createFileIn(options, callback);
        } else if (connectionType === 'internal') {
            conn = createInternalIn(options, callback);
        } else {
            log.error('Connection type invalid:' + connectionType);
            callback('callback', {type: 'failed', reason: 'Connection type invalid:' + connectionType});
            return;
        }

        connections[connectionId] = {type: connectionType,
                                     direction: 'in',
                                     connection: conn};
    };

    that.unpublish = function (connectionId, callback) {
        log.debug('unpublish, connectionId:', connectionId);
        if (connections[connectionId] !== undefined) {
            cutOffFrom(connectionId);
            connections[connectionId].connection.close();
            delete connections[connectionId];
            callback('callback', 'ok');
        } else {
            log.info('Connection does NOT exist:' + connectionId);
            callback('callback', 'error', 'Connection does NOT exist:' + connectionId);
        }
    };

    that.subscribe = function (connectionId, connectionType, options, callback) {
        log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if (connections[connectionId]) {
            log.error('Connection already exists:'+connectionId);
            callback('callback', {type: 'failed', reason: 'Connection already exists:'+connectionId});
            return;
        }

        var conn;

        if (connectionType === 'webrtc') {
            conn = createWebRTCConnection('out', options, callback);
        } else if (connectionType === 'avstream') {
            conn = createAVStreamOut(options, callback);
        } else if (connectionType === 'recording') {
            conn = createFileOut(options, callback);
        } else if (connectionType === 'internal') {
            conn = createInternalOut(options, callback);
        } else {
            log.error('Connection type invalid:' + connectionType);
            callback('callback', {type: 'failed', reason: 'Connection type invalid:' + connectionType});
            return;
        }

        connections[connectionId] = {type: connectionType,
                                     direction: 'out',
                                     audioFrom: undefined,
                                     videoFrom: undefined,
                                     connection: conn};
    };

    that.unsubscribe = function (connectionId, callback) {
        log.debug('unsubscribe, connectionId:', connectionId);
        if (connections[connectionId] !== undefined) {
            cutOffTo(connectionId);
            connections[connectionId].connection.close();
            delete connections[connectionId];
            callback('callback', 'ok');
        } else {
            log.info('Connection does NOT exist:' + connectionId);
            callback('callback', 'error', 'Connection does NOT exist:' + connectionId);
        }
    };

    that.linkup = function (connectionId, audioFrom, videoFrom, callback) {
        log.debug('linkup, connectionId:', connectionId, ', audioFrom:', audioFrom, ', videoFrom:', videoFrom);
        if (!connectionId || !connections[connectionId]) {
            log.error('Subscription does not exist:' + connectionId);
            return callback('callback', 'error', 'Subscription does not exist:' + connectionId);
        }

        if (audioFrom && connections[audioFrom] === undefined) {
            log.error('Audio stream does not exist:' + audioFrom);
            return callback('callback', {type: 'failed', reason: 'Audio stream does not exist:' + audioFrom});
        }

        if (videoFrom && connections[videoFrom] === undefined) {
            log.error('Video stream does not exist:' + videoFrom);
            return callback('callback', {type: 'failed', reason: 'Video stream does not exist:' + videoFrom});
        }

        var conn = connections[connectionId];

        if (audioFrom) {
            var dest = (conn.type === 'webrtc' ? conn.connection.receiver('audio') : conn.connection);
            connections[audioFrom].connection.addDestination('audio', dest);
            connections[connectionId].audioFrom = audioFrom;
        }

        if (videoFrom) {
            var dest = (conn.type === 'webrtc' ? conn.connection.receiver('video') : conn.connection);
            connections[videoFrom].connection.addDestination('video', dest);
            connections[connectionId].videoFrom = videoFrom;
        }
        callback('callback', 'ok');
    };

    that.cutoff = function (connectionId, callback) {
        log.debug('cutoff, connectionId:', connectionId);
        if (connections[connectionId]) {
            cutOffTo(connectionId);
            callback('callback', 'ok');
        } else {
            log.info('Connection does NOT exist:' + connectionId);
            callback('callback', 'error', 'Connection does NOT exist:' + connectionId);
        }
    };

    that.onConnectionSignalling = function (connectionId, msg, callback) {
        log.debug('onConnectionSignalling, connection id:', connectionId, 'msg:', msg);
        if (connections[connectionId]) {
            if (connections[connectionId].type === 'webrtc') {//NOTE: Only webrtc connection supports signaling.
                connections[connectionId].connection.onSignalling(msg);
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
        if (connections[connectionId]) {
            if (connections[connectionId].type === 'webrtc') {//NOTE: Only webrtc connection supports media-on-off
                connections[connectionId].connection.onTrackControl(track,
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
        if (connections[connectionId] && connections[connectionId].direction === 'in') {
            if (connections[connectionId].type === 'webrtc') {//NOTE: Only webrtc connection supports setting video bitrate.
                connections[connectionId].connection.setVideoBitrate(bitrate);
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
