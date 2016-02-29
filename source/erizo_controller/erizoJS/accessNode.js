/*global require, exports*/
'use strict';

var woogeenInternalIO = require('./../../bindings/internalIO/build/Release/internalIO');
var woogeenMediaFileIO = require('./../../bindings/mediaFileIO/build/Release/mediaFileIO');
var InternalIn = woogeenInternalIO.InternalIn;
var InternalOut = woogeenInternalIO.InternalOut;
var MediaFileIn = woogeenMediaFileIO.MediaFileIn;
var MediaFileOut = woogeenMediaFileIO.MediaFileOut;
var RtspOut = require('./../../bindings/rtspOut/build/Release/rtspOut').RtspOut;
var WrtcConnection = require('./wrtcConnection').WrtcConnection;
var RtspIn = require('./rtspIn').RtspIn;
var logger = require('./../common/logger').logger;
var amqper = require('./../common/amqper');

// Logger
var log = logger.getLogger('AccessNode');

exports.AccessNode = function () {
    var that = {},
        /*{StreamID: {type: 'webrtc' | 'rtsp' | 'file' | 'internal',
                      connection: WebRtcConnection | InternalIn | RTSPConnectionIn}
          }
        */
        streams = {},

        /*{SubscriptionID: {type: 'webrtc' | 'rtsp' | 'file' | 'internal',
                            audio: StreamID | undefined,
                            video: StreamID | undefined,
                            connnection: WebRtcConnection | InternalOut | RTSPConnectionOut
                           }
          }
        */
        subscriptions = {};

    that.publish = function (stream_id, stream_type, options, callback) {
        log.debug('publish, stream_id:', stream_id, ', stream_type:', stream_type, ', audio:', options.has_audio, ', video:', options.has_video);
        if (streams[stream_id]) {
            log.error('Stream already exists:'+stream_id);
            callback('callback', {type: 'failed', reason: 'Stream already exists:'+stream_id});
            return;
        }

        var conn;

        if (stream_type === 'webrtc') {
            conn = new WrtcConnection({direction: 'in',
                                       private_ip_regexp: that.privateRegexp,
                                       public_ip: that.publicIP});
            conn.init(options.has_audio, options.has_video, function (response) {
                callback('callback', response);
            });
        } else if (stream_type === 'internal') {
            conn = new InternalIn(options.protocol);
            callback('callback', conn.getListeningPort());
        } else if (stream_type === 'rtsp') {
            conn = new RtspIn();
            conn.init(options.url, options.has_audio, options.has_video, options.transport, options.buffer_size, function (response) {
                callback('callback', response);
            });
        } else if (stream_type === 'file') {
            conn = new MediaFileIn(options.path, options.has_audio, options.has_video);
            // FIXME: There should be a better chance to start playing.
            setTimeout(function () {conn.startPlay();}, 6000);
            callback('callback', {type: 'ready'});
        } else {
            log.error('Stream type invalid:'+stream_type);
            callback('callback', {type: 'failed', reason: 'Stream type invalid:'+stream_type});
            return;
        }

        streams[stream_id] = {type: stream_type, connection: conn};
    };

    that.unpublish = function (stream_id) {
        log.debug('unpublish, stream_id:', stream_id);
        if (streams[stream_id]) {
            for (var subscription_id in subscriptions) {
                if (subscriptions[subscription_id].audio === stream_id) {
                    log.debug('remove audio:', subscriptions[subscription_id].audio);
                    var dest = subscriptions[subscription_id].type === 'webrtc' ? subscriptions[subscription_id].connection.receiver('audio') : subscriptions[subscription_id].connection;
                    streams[stream_id].connection.removeDestination('audio', dest);
                    subscriptions[subscription_id].audio = undefined;
                }

                if (subscriptions[subscription_id].video === stream_id) {
                    log.debug('remove video:', subscriptions[subscription_id].video);
                    var dest = subscriptions[subscription_id].type === 'webrtc' ? subscriptions[subscription_id].connection.receiver('video') : subscriptions[subscription_id].connection;
                    streams[stream_id].connection.removeDestination('video', dest);
                    subscriptions[subscription_id].video = undefined;
                }

                if (subscriptions[subscription_id].audio === undefined && subscriptions[subscription_id].video === undefined) {
                    subscriptions[subscription_id].connection.close();
                    delete subscriptions[subscription_id];
                }
            }

            streams[stream_id].connection.close();
            delete streams[stream_id];
        } else {
            log.warn('stream['+stream_id+'] doesn\'t exist.');
        }
    };

    that.connect = function (subscription_id, subscription_type, options, callback) {
        if (subscriptions[subscription_id]) {
            log.error('Subscription already exists:'+subscription_id);
            callback('callback', {type: 'failed', reason: 'Subscription already exists:'+subscription_id});
            return;
        }

        var conn;

        if (subscription_type === 'webrtc') {
            conn = new WrtcConnection({direction: 'out',
                                       private_ip_regexp: that.privateRegexp,
                                       public_ip: that.publicIP});
            conn.init(options.require_audio, options.require_video, function (response) {
                callback('callback', response);
            });
        } else if (subscription_type === 'rtsp') {
            conn = new RtspOut(options.url);
            callback('callback', {type: 'ready', audio_codecs: [options.audio_codec], video_codecs: [options.video_codec]});
        } else if (subscription_type === 'file') {
            conn = new MediaFileOut(options.path, options.interval);
            conn.addEventListener('RecordingStream', function (message) {
                log.error('External output for media recording error occurs.');
                if (options.observer !== undefined) {
                    amqper.callRpc(options.observer, 'eventReport', ['deleteExternalOutput', options.room_id, {message: message, id: subscription_id, data: null}]);
                }
            });
            callback('callback', {type: 'ready', audio_codecs: [options.audio_codec], video_codecs: [options.video_codec]});
        } else {
            log.error('Pre-subscription, Subscription type invalid:' + subscription_type);
            callback('callback', {type: 'failed', reason: 'Pre-subscription, Subscription type invalid:' + subscription_type});
            return;
        }

        subscriptions[subscription_id] = {type: subscription_type,
                                          audio: undefined,
                                          video: undefined,
                                          connection: conn};
    };

    that.disconnect = function (subscription_id) {
        if (subscriptions[subscription_id] !== undefined) {
            subscriptions[subscription_id].connection.close();
            delete subscriptions[subscription_id];
        } else {
            log.info('Subscription does NOT exist:'+subscription_id);
        }
    };

    that.subscribe = function (subscription_id, subscription_type, audio_stream_id, video_stream_id, options, callback) {
        log.debug('subscribe, subscription_id:', subscription_id, ', subscription_type:', subscription_type, ', audio_stream_id:', audio_stream_id, ', video_stream_id:', video_stream_id);
        if (audio_stream_id && streams[audio_stream_id] === undefined) {
            log.error('Audio stream does not exist:'+audio_stream_id);
            callback('callback', {type: 'failed', reason: 'Audio stream does not exist:'+audio_stream_id});
            return;
        }

        if (video_stream_id && streams[video_stream_id] === undefined) {
            log.error('Video stream does not exist:'+video_stream_id);
            callback('callback', {type: 'failed', reason: 'Video stream does not exist:'+video_stream_id});
            return;
        }

        var conn = subscriptions[subscription_id] && subscriptions[subscription_id].connection || undefined;

        if (!conn) {
            switch (subscription_type) {
                case 'webrtc':
                case 'file':
                case 'rtsp':
                    log.error('Subscription connection does not exist:' + subscription_id);
                    callback('callback', {type: 'failed', reason: 'Subscription connection does not exist:'+subscription_id});
                    return;
                case 'internal':
                    conn = new InternalOut(options.protocol, options.dest_ip, options.dest_port);
                    break;
                default:
                    log.error('Subscription type invalid:' + subscription_type);
                    callback('callback', {type: 'failed', reason: 'Subscription type invalid:' + subscription_type});
                    return;
            }

            subscriptions[subscription_id] = {type: subscription_type,
                                              audio: undefined,
                                              video: undefined,
                                              connection: conn};
        }

        if (audio_stream_id) {
            var dest = subscription_type === 'webrtc' ? conn.receiver('audio') : conn;
            streams[audio_stream_id].connection.addDestination('audio', dest);
            subscriptions[subscription_id].audio = audio_stream_id;
        }

        if (video_stream_id) {
            var dest = subscription_type === 'webrtc' ? conn.receiver('video') : conn;
            streams[video_stream_id].connection.addDestination('video', dest);
            if (streams[video_stream_id].type === 'webrtc') {streams[video_stream_id].connection.requestKeyFrame();}//FIXME: Temporarily add this interface to workround the hardware mode's absence of feedback mechanism.
            subscriptions[subscription_id].video = video_stream_id;
        }
        callback('callback', {type: 'ready'});
    };

    that.unsubscribe = function (subscription_id) {
        log.debug('unsubscribe, subscription_id:', subscription_id);
        if (subscriptions[subscription_id] !== undefined) {
            if (subscriptions[subscription_id].audio
                && streams[subscriptions[subscription_id].audio]) {
                log.debug('remove audio:', subscriptions[subscription_id].audio);
                var dest = subscriptions[subscription_id].type === 'webrtc' ? subscriptions[subscription_id].connection.receiver('audio') : subscriptions[subscription_id].connection;
                streams[subscriptions[subscription_id].audio].connection.removeDestination('audio', dest);
            }

            if (subscriptions[subscription_id].video
                && streams[subscriptions[subscription_id].video]) {
                log.debug('remove video:', subscriptions[subscription_id].video);
                var dest = subscriptions[subscription_id].type === 'webrtc' ? subscriptions[subscription_id].connection.receiver('video') : subscriptions[subscription_id].connection;
                streams[subscriptions[subscription_id].video].connection.removeDestination('video', dest);
            }

            subscriptions[subscription_id].connection.close();
            delete subscriptions[subscription_id];
        } else {
            log.info('Subscription does NOT exist:'+subscription_id);
        }
    };

    that.onConnectionSignalling = function (connection_id, msg) {
        log.debug('onConnectionSignalling, connection_id:', connection_id, 'msg:', msg);
        if (streams[connection_id] && streams[connection_id].type === 'webrtc') {
            log.debug('on publisher\'s ConnectionSignalling');
            streams[connection_id].connection.onSignalling(msg);
        } else if (subscriptions[connection_id] && subscriptions[connection_id].type === 'webrtc') {
            log.debug('on subscriber\'s ConnectionSignalling');
            subscriptions[connection_id].connection.onSignalling(msg);
        }
    };

    that.onTrackControl = function (connection_id, track, direction, action, callback) {
        log.debug('onTrackControl, connection_id:', connection_id, 'track:', track, 'direction:', direction, 'action:', action);
        if (streams[connection_id] && streams[connection_id].type === 'webrtc') {
            streams[connection_id].connection.onTrackControl(track, direction, action, function () {callback('callback', 'ok');}, function (error_reason) {callback('callback', 'error', error_reason);});
        } else if (subscriptions[connection_id] && subscriptions[connection_id].type === 'webrtc') {
            subscriptions[connection_id].connection.onTrackControl(track, direction, action, function () {callback('callback', 'ok');}, function (error_reason) {callback('callback', 'error', error_reason);});
        } else {
            callback('callback', 'error', 'No such a connection.');
        }
    };

    that.setVideoBitrate = function (stream_id, bitrate, callback) {
        if (streams[stream_id] && streams[stream_id].type === 'webrtc') {
            streams[stream_id].connection.setVideoBitrate(bitrate);
            callback('callback', 'ok');
        } else {
            callback('callback', 'error', 'Stream:'+stream_id+' does not exist.');
        }
    };

    return that;
};
