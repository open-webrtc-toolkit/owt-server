/*global require, exports, , setInterval, clearInterval*/
'use strict';
var addon = require('./../../bindings/mcu/build/Release/addon');
var WrtcConnection = require('./wrtcConnection').WrtcConnection;
var RtspIn = require('./rtspIn');
var logger = require('./../common/logger').logger;
var amqper = require('./../common/amqper');

// Logger
var log = logger.getLogger("AccessNode");

var BINDED_INTERFACE_NAME = GLOBAL.config.erizoController.networkInterface;
var privateRegexp;
var publicIP;

exports.AccessNode = function (spec) {
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


    var collectIPAddr = function () {
        var interfaces = require('os').networkInterfaces(),
            addresses = [],
            k,
            k2,
            address;


        for (k in interfaces) {
            if (interfaces.hasOwnProperty(k)) {
                for (k2 in interfaces[k]) {
                    if (interfaces[k].hasOwnProperty(k2)) {
                        address = interfaces[k][k2];
                        if (address.family === 'IPv4' && !address.internal) {
                            if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
                                addresses.push(address.address);
                            }
                        }
                    }
                }
            }
        }

        privateRegexp = new RegExp(addresses[0], 'g');

        if (GLOBAL.config.erizoController.publicIP === '' || GLOBAL.config.erizoController.publicIP === undefined){
            publicIP = addresses[0];
        } else {
            publicIP = GLOBAL.config.erizoController.publicIP;
        }
    };

    that.publish = function (stream_id, stream_type, options, callback) {
        if (streams[stream_id]) {
            log.error("Stream already exists:"+stream_id);
            callback('callback', {type: 'failed', reason: "Stream already exists:"+stream_id});
            return;
        }

        var conn = undefined;

        if (stream_type === 'webrtc') {
            conn = new WrtcConnection({direction: 'in',
                                       private_ip_regexp: privateRegexp,
                                       public_ip: publicIP});
            conn.init(options.has_audio, options.has_video, function (response) {
                callback('callback', response);
            });
        } else if (stream_type === 'internal') {
            conn = new addon.InternalIn(options.protocol);
            callback('callback', conn.getListeningPort());
        } else if (stream_type === 'rtsp') {
            conn = new RtspIn();
            conn.init(options.url, options.has_audio, options.has_video, options.transport, options.buffer_size, function (response) {
                callback('callback', response);
            });
        } else if (stream_type === 'file') {
            conn = new addon.MediaFileIn(options.path, options.has_audio, options.has_video);
            // FIXME: There should be a better chance to start playing.
            setTimeout(function () {conn.startPlay();}, 6000);
            callback('callback', {type: 'ready'});
        } else {
            log.error("Stream type invalid:"+stream_type);
            callback('callback', {type: 'failed', reason: "Stream type invalid:"+stream_type});
            return;
        }

        streams[stream_id] = {type: stream_type, connection: conn};
    };

    that.unpublish = function (stream_id) {
        if (streams[stream_id]) {
            for (var subscription_id in subscriptions) {
                if (subscriptions[subscription_id].audio === stream_id) {
                    streams[stream_id].connection.removeDestination("audio", subscriptions[subscription_id].connection);
                    subscriptions[subscription_id].audio = undefined;
                }

                if (subscriptions[subscription_id].video === stream_id) {
                    streams[stream_id].connection.removeDestination("video", subscriptions[subscription_id].connection);
                    subscriptions[subscription_id].video = undefined;
                }

                if (subscriptions[subscription_id].audio === undefined && subscriptions[subscription_id].video === undefined) {
                    subscriptions[subscription_id].connection.close();
                    delete subscriptions[subscription_id];
                }
            }

            streams[stream_id].connection.close();
            delete streams[stream_id];
        }
    };

    that.connect = function (subscription_id, subscription_type, options, callback) {
        if (subscriptions[subscription_id]) {
            log.error("Subscription already exists:"+subscription_id);
            callback('callback', {type: 'failed', reason: "Subscription already exists:"+subscription_id});
            return;
        }

        var conn = undefined;

        if (subscription_type === 'webrtc') {
            conn = new WrtcConnection({direction: 'out',
                                       private_ip_regexp: privateRegexp,
                                       public_ip: publicIP});
            conn.init(options.require_audio, options.require_video, function (response) {
                callback('callback', response);
            });
        } else{
            log.error("Pre-subscription, Subscription type invalid:"+subscription_type);
            callback('callback', {type: 'failed', reason: "Pre-subscription, Subscription type invalid:"+subscription_type});
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
            log.info("Subscription does NOT exist:"+subscription_id);
        }
    };

    that.subscribe = function (subscription_id, subscription_type, audio_stream_id, video_stream_id, options, callback) {
        if (audio_stream_id && streams[audio_stream_id] === undefined) {
            log.error("Audio stream does not exist:"+audio_stream_id);
            callback('callback', {type: 'failed', reason: "Audio stream does not exist:"+audio_stream_id});
            return;
        }

        if (video_stream_id && streams[video_stream_id] === undefined) {
            log.error("Video stream does not exist:"+video_stream_id);
            callback('callback', {type: 'failed', reason: "Video stream does not exist:"+video_stream_id});
            return;
        }

        var conn = subscriptions[subscription_id] && subscriptions[subscription_id].connection || undefined;

        if (!conn) {
            if (subscription_type === 'webrtc') {
                log.error("Subscription connection does not exist:"+subscription_id);
                callback('callback', {type: 'failed', reason: "Subscription connection does not exist:"+subscription_id});
                return;
            } else if (subscription_type === 'internal') {
                conn = new addon.InternalOut(options.protocol, options.dest_ip, options.dest_port);
            } else if (subscription_type === 'rtsp') {
                conn = new addon.RtspOut(options.url);
            } else if (subscription_type === 'file') {
                conn = new addon.MediaFileOut(options.path, options.interval);
                conn.addEventListener('RecordingStream', function (message) {
                    log.error('External output for media recording error occurs.');
                    if (options.observer !== undefined) {
                        amqper.callRpc(options.observer, 'eventReport', ['deleteExternalOutput', options.room_id, {message: message, id: subscription_id, data: null}]);
                    }
                });
            } else {
                log.error("Subscription type invalid:"+subscription_type);
                callback('callback', {type: 'failed', reason: "Subscription type invalid:"+subscription_type});
                return;
            }
            subscriptions[subscription_id] = {type: subscription_type,
                                              audio: undefined,
                                              video: undefined,
                                              connection: conn};
        }

        if (audio_stream_id) {
            streams[audio_stream_id].connection.addDestination("audio", subscription_type === 'webrtc' ? conn.receiver("audio") : conn);
            subscriptions[subscription_id].audio = audio_stream_id;
        }

        if (video_stream_id) {
            streams[video_stream_id].connection.addDestination("video", subscription_type === 'webrtc' ? conn.receiver("video") : conn);
            subscriptions[subscription_id].video = video_stream_id;
        }
        callback('callback', {type: 'ready'});
    };

    that.unsubscribe = function (subscription_id) {
        if (subscriptions[subscription_id] !== undefined) {
            if (subscriptions[subscription_id].audio
                && streams[subscriptions[subscription_id].audio]) {
                streams[subscriptions[subscription_id].audio].connection.removeDestination("audio", subscriptions[subscription_id].type === 'webrtc' ? subscriptions[subscription_id].connection.receiver("audio") : subscriptions[subscription_id].connection);
            }

            if (subscriptions[subscription_id].video
                && streams[subscriptions[subscription_id].video]) {
                streams[subscriptions[subscription_id].video].connection.removeDestination("video", subscriptions[subscription_id].type === 'webrtc' ? subscriptions[subscription_id].connection.receiver("audio") : subscriptions[subscription_id].connection);
            }

            subscriptions[subscription_id].connection.close();
            delete subscriptions[subscription_id];
        } else {
            log.info("Subscription does NOT exist:"+subscription_id);
        }
    };

    that.onConnectionSignalling = function (terminal_id, stream_id, msg) {
        log.debug("onConnectionSignalling, stream_id:", stream_id, "msg:", msg);
        if (streams[stream_id] && streams[stream_id].type === 'webrtc') {
            log.debug("on publisher's ConnectionSignalling");
            streams[stream_id].connection.onSignalling(msg);
        } else if (subscriptions[terminal_id+'-'+stream_id] && subscriptions[terminal_id+'-'+stream_id].type === 'webrtc') {
            log.debug("on subscriber's ConnectionSignalling");
            subscriptions[terminal_id+'-'+stream_id].connection.onSignalling(msg);
        }
    };

    that.onTrackControl = function (terminal_id, stream_id, track, direction, action, callback) {
        if (streams[stream_id] && streams[stream_id].type === 'webrtc') {
            streams[stream_id].connection.onTrackControl(track, direction, action, function () {callback('callback', 'ok');}, function (error_reason) {callback('callback', 'error', error_reason);});
        } else if (subscriptions[terminal_id+'-'+stream_id] && subscriptions[terminal_id+'-'+stream_id].type === 'webrtc') {
            subscriptions[terminal_id+'-'+stream_id].connection.onTrackControl(track, direction, action, function () {callback('callback', 'ok');}, function (error_reason) {callback('callback', 'error', error_reason);});
        } else {
            callback('callback', 'error', 'No such a connection.');
        }
    };

    that.setVideoBitrate = function (stream_id, bitrate, callback) {
        if (streams[stream_id] && streams[stream_id].type === 'webrtc') {
            streams[stream_id].connection.setVideoBitrate(bitrate),
            callback('callback', 'ok');
        } else {
            callback('callback', 'error', 'Stream:'+stream_id+' does not exist.');
        }
    };

    collectIPAddr();
    return that;
};
