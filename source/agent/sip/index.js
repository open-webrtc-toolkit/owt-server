/*global require, exports*/
'use strict';

var woogeenInternalIO = require('./internalIO/build/Release/internalIO');
var woogeenSipGateway = require('./sipIn/build/Release/sipIn');
var SipCallConnection = require('./sipCallConnection').SipCallConnection;
var InternalIn = woogeenInternalIO.In;
var InternalOut = woogeenInternalIO.Out;
var logger = require('./logger').logger;
var rpcClient;
var path = require('path');
var makeRPC = require('./makeRPC').makeRPC;

var cluster_name = ((GLOBAL.config || {}).cluster || {}).name || 'woogeen-cluster';

// Logger
var log = logger.getLogger('SipNode');

// resolution map
var resolution_map = {'sif' : {'width' : 352, 'height' : 240},
                      'vga' : {'width' : 640, 'height' : 480},
                      'hd720p' : {'width' : 1280, 'height' : 720},
                      'hd1080p' : {'width' : 1920, 'height' : 1080},
                      'svga' : {'width' : 800, 'height' : 600},
                      'r640x360' : {'width' : 640, 'height' : 360}}

var generateStreamId = (function() {
  function s4() {
    return Math.floor((1 + Math.random()) * 0x10000)
               .toString(16)
               .substring(1);
  }
  return function() {
    return s4() + s4() + s4();
  };
})();

function safeCall () {
  var callback = arguments[0];
  if (typeof callback === 'function') {
    var args = Array.prototype.slice.call(arguments, 1);
    callback.apply(null, args);
  }
}

function do_join(session_ctl, user, room, selfPortal, ok, err) {
    makeRPC(
        rpcClient,
        session_ctl,
        'join',
        [room, {id: user, name: user, role: 'presenter', portal: selfPortal}], function(joinResult) {
            log.debug('join ok');
            for(var index in joinResult.streams){
                if(joinResult.streams[index].video.device === 'mcu'){
                    safeCall(ok, joinResult.streams[index]);
                    return;
                }
            }
            safeCall(ok);
        }, function (reason) {
            safeCall(err,reason);
        });
}

function do_leave(session_ctl, user) {
    makeRPC(
        rpcClient,
        session_ctl,
        'leave',
        [user],
        function() {
            log.debug('leave ok');
        });

}
function do_query(session_ctl, user, room, ok, err) {
    makeRPC(
        rpcClient,
        session_ctl,
        'query',
        [user, room], function(streams) {
            safeCall(ok, streams);
        }, function(reason) {
            safeCall(err, reason);
        });
}

function do_publish(session_ctl, user, stream_id, accessNode, stream_info) {
    return new Promise(function(resolve, reject) {
        makeRPC(
            rpcClient,
            session_ctl,
            'publish',
            [user, stream_id, accessNode, stream_info, false],
            resolve,
            reject);
    });
}

function do_subscribe(session_ctl, user, subscription_id, accessNode, subInfo) {
    return new Promise(function(resolve, reject) {
        makeRPC(
            rpcClient,
            session_ctl,
            'subscribe',
            [user, subscription_id, accessNode, subInfo],
            resolve,
            reject);
    });
}

function do_unpublish(session_ctl, user, stream_id) {
    return new Promise(function(resolve, reject) {
        makeRPC(
            rpcClient,
            session_ctl,
            'unpublish',
            [user, stream_id],
            resolve,
            reject);
    });
}

function do_unsubscribe(session_ctl, user, subscription_id) {
    return new Promise(function(resolve, reject) {
        makeRPC(
            rpcClient,
            session_ctl,
            'unsubscribe',
            [user, subscription_id],
            resolve,
            reject);
    });
}

var getSessionControllerForRoom = function (roomId, on_ok, on_error) {
    var keepTrying = true;

    var tryFetchingSessionController = function (attempts) {
        if (attempts <= 0) {
            return on_error('Timeout to fetech controller');
        }

        log.info('Send controller schedule RPC request to ', cluster_name, ' for room ', roomId);

        makeRPC(rpcClient, cluster_name, 'schedule', ['session', roomId, 60 * 1000],
            function (result) {
                makeRPC(rpcClient, result.id, 'getNode', [{session: roomId, consumer: roomId}], function (sessionController) {
                    on_ok(sessionController);
                    keepTrying = false;
                }, function(reason) {
                    if (keepTrying) {
                        log.warn('Failed on get node for', roomId, ', keep trying.');
                        setTimeout(function () {tryFetchingSessionController(attempts - (reason === 'timeout' ? 4 : 1));}, 1000);
                    }
                });
            }, function (reason) {
                if (keepTrying) {
                    log.warn('Failed on scheduling session for', roomId, ', keep trying.');
                    setTimeout(function () {tryFetchingSessionController(attempts - (reason === 'timeout' ? 4 : 1));}, 1000);
                }
            });
    };

    tryFetchingSessionController(25);
};

module.exports = function (rpcC, spec) {
    rpcClient = rpcC;

    var that = {},
        erizo = {id:spec.id, addr:spec.addr},
        room_id,
        mixed_stream_id,
        mixed_stream_resolutions,
        gateway,
        streams = {},
        calls = {},
        subscriptions = {},
        recycling_mode = false;

    var handleIncomingCall = function (peerURI, on_ok, on_error) {
        var client_id = peerURI;

        getSessionControllerForRoom(room_id, function(result) {
            var session_controller = result;
            calls[client_id] = {session_controller : session_controller};
            do_join(session_controller, client_id, room_id, erizo.id, function(mixedStream) {
                if(mixedStream !== undefined){
                    mixed_stream_id = mixedStream.id;
                    mixed_stream_resolutions = mixedStream.video.resolutions;
                }
                on_ok();
            }, function (err) {
                on_error(err);
            });
        }, on_error);
    };

    //TODO: should complete the following procedure to protect against the unexpected situations such as partial failure.
    var setupCall = function (client_id, info) {
        var session_controller = calls[client_id].session_controller;

        var published = Promise.resolve('ok');
        var subscribed = Promise.resolve('ok');

        var audio_info, video_info;
        if (info.audio) {
            var tmp;
            if (info.audio_codec === 'opus') {
               tmp = 'opus_48000_2';
            } else if (info.audio_codec === 'PCMU') {
               tmp = 'pcmu';
            }
            audio_info = {codec:tmp};
        }
        if (info.video) {
            video_info = {codec:info.video_codec.toLowerCase(), resolution:'vga', framerate:30};
        }

        // publish stream to controller
        if ((info.audio && info.audio_dir !== 'sendonly') || (info.video && info.video_dir !== 'sendonly')) {
            var stream_id = generateStreamId();

            //TODO: the streams binding should be done in the success callback.
            streams[stream_id] = {type: 'sip', connection: calls[client_id].conn};
            calls[client_id].stream_id = stream_id;

            var pubInfo = {};
            if (info.audio && info.audio_dir !== 'sendonly') {
                pubInfo.audio = audio_info;
            }
            if (info.video && info.video_dir !== 'sendonly') {
                pubInfo.video = video_info;
            }

            published = do_publish(calls[client_id].session_controller,
                                   client_id,
                                   stream_id,
                                   {agent:that.agentID, node: erizo.id},
                                   pubInfo);
        }

        // subscribe the mix streams
        if ((info.audio && info.audio_dir !== 'recvonly') || (info.video && info.video_dir !== 'recvonly')) {
            if (mixed_stream_id) {
                var subInfo = {};
                if (info.audio && info.audio_dir !== 'recvonly') {
                    subInfo.audio = {
                        fromStream: mixed_stream_id,
                        codecs: [audio_info.codec]
                    };
                }
                if (info.video && info.video_dir !== 'recvonly') {
                    //check resolution
                    var fmtp = info.videoResolution;
                    var preferred_subscription_resolution = mixed_stream_resolutions[0];
                    //TODO: currently we only check CIF/QCIF, there might be other options in fmtp from other devices.
                    if((fmtp.indexOf('CIF') != -1 || fmtp.indexOf('QCIF') != -1)){
                            if(mixed_stream_resolutions.indexOf('sif') != -1){
                                    preferred_subscription_resolution = 'sif';
                            }else{
                                    log.warn('MCU does not support the resolution required by sip client');
                                    var diff = Number.MAX_VALUE;
                                    //looking for the resolution closest to 352 * 240(sif)
                                    for(var index in mixed_stream_resolutions){
                                            var current_diff = resolution_map[mixed_stream_resolutions[index]].width - 352
                                                                 + resolution_map[mixed_stream_resolutions[index]].height - 240;
                                            if(current_diff < diff){
                                                    diff = current_diff;
                                                    preferred_subscription_resolution = mixed_stream_resolutions[index];
                                            }
                                    }
                            }
                    }
                    subInfo.video = {
                        fromStream: mixed_stream_id,
                        codecs: [video_info.codec],
                        resolution: preferred_subscription_resolution
                    };
                }
                //TODO: The subscriptions binding should be done in the success callback.
                calls[client_id].mix_stream_id = mixed_stream_id;
                subscriptions[client_id] = {type: 'sip',
                                            audio: undefined,
                                            video: undefined,
                                            connection: calls[client_id].conn};

                subscribed = do_subscribe(calls[client_id].session_controller,
                                          client_id,
                                          client_id,
                                          {agent:that.agentID, node: erizo.id},
                                          subInfo);
            } else {
                log.warn("invalid mix stream id");
            }
        }

        return Promise.all([published, subscribed]).then(function(result) {
            log.debug('setup call ok:', info);
            if (calls[client_id]) {
                // keep the current info
                calls[client_id].currentInfo = info;
            }
            return info;
        });
    };

    var teardownCall = function (client_id) {
        log.debug("teardownCall, client_id: ", client_id);
        if (subscriptions[client_id]) {
            var audio_from = subscriptions[client_id].audio,
                video_from = subscriptions[client_id].video;

            if (streams[audio_from]) {
                var dest = subscriptions[client_id].connection.receiver('audio');
                streams[audio_from].connection.removeDestination('audio', dest);
                subscriptions[client_id].audio = undefined;
            }

            if (streams[video_from]) {
                var dest = subscriptions[client_id].connection.receiver('video');
                streams[video_from].connection.removeDestination('video', dest);
                subscriptions[client_id].video = undefined;
            }

            delete subscriptions[client_id];
        }

        var stream_id = calls[client_id].stream_id;
        if (stream_id && streams[stream_id]) {
            for (var subscription_id in subscriptions) {
                if (subscriptions[subscription_id].audio === stream_id) {
                    log.debug('remove audio:', subscriptions[subscription_id].audio);
                    var dest = subscriptions[subscription_id].connection;
                    log.debug('remove audio removeDestination: ', streams[stream_id].connection, ' dest: ', dest);
                    streams[stream_id].connection.removeDestination('audio', dest);
                    subscriptions[subscription_id].audio = undefined;
                }

                if (subscriptions[subscription_id].video === stream_id) {
                    log.debug('remove video:', subscriptions[subscription_id].video);
                    var dest = subscriptions[subscription_id].connection;
                    log.debug('remove video removeDestination: ', streams[stream_id].connection, ' dest: ', dest);
                    streams[stream_id].connection.removeDestination('video', dest);
                    subscriptions[subscription_id].video = undefined;
                }

                if (subscriptions[subscription_id].audio === undefined && subscriptions[subscription_id].video === undefined) {
                    subscriptions[subscription_id].type === 'internal' && subscriptions[subscription_id].connection.close();
                    delete subscriptions[subscription_id];
                }
            }
            delete streams[stream_id];
            calls[client_id].stream_id = undefined;
        }
    };

    var handleCallEstablished = function (info) {
        log.info('CallEstablished:', info.peerURI, 'audio='+info.audio, 'video='+info.video,
                 (info.audio ? (' audio codec:' + info.audio_codec + ' audio dir: ' + info.audio_dir) : ''),
                 (info.video ? (' video codec: ' + info.video_codec + ' video dir: ' + info.video_dir) : ''));
        var client_id = info.peerURI;

        if (calls[client_id]) {
            calls[client_id].conn = new SipCallConnection({gateway: gateway, clientID: client_id, audio : info.audio, video : info.video});
            setupCall(client_id, info)
            .catch(function(err) {
                log.error('Error during call establish:', err);
            });
        } else {
            log.error("gateway can not handle event with invalid status");
        }
    };

    var handleCallUpdated = function (info) {
        log.info('CallUpdated:', info, calls);

        var client_id = info.peerURI;
        if(calls[client_id] === undefined || calls[client_id].session_controller === undefined || calls[client_id].currentInfo === undefined) {
            log.warn('Call ' + client_id + ' not established, ignore it');
            return;
        }

        if (calls[client_id].updating) {
            log.warn("Too frequent call update request, process it later");
            calls[client_id].latestInfo = info;
            return;
        }

        // Call info compare function
        var infoEqual = function(a, b) {
            var audioEqual = (a.audio === b.audio);
            if (a.audio && b.audio) {
                audioEqual = (a.audio_codec === b.audio_codec && a.audio_dir === b.audio_dir);
            }
            var videoEqual = (a.video === b.video);
            if (a.video && b.video) {
                videoEqual = (a.video_codec === b.video_codec && a.videoResolution === b.videoResolution && a.video_dir === b.video_dir);
            }

            return (audioEqual && videoEqual);
        }

        // Ignore duplicate update requests
        if (infoEqual(calls[client_id].currentInfo, info)) {
            log.warn('Same as current info:', info, 'ignore it.');
            return;
        }

        calls[client_id].updating = true;

        var session_controller = calls[client_id].session_controller;
        var old_stream_id = calls[client_id].stream_id;

        // Ignore unpublish/unsubscribe failure for send-only/receive-only clients
        var unpublished = do_unpublish(session_controller, client_id, old_stream_id)
            .then(function(ok) {
                return ok;
            }).catch(function(err) {
                return err;
            });
        var unsubscribed = do_unsubscribe(session_controller, client_id, client_id).then(
            function(ok) {
                return ok;
            }).catch(function(err) {
                return err;
            });

        Promise.all([unpublished, unsubscribed])
        .then(function(result) {
            log.debug('handleCallUpdated unsubscribe/unpublish ok');

            teardownCall(client_id);
            // recreate a sip call connection
            calls[client_id].conn.close({input: true, output: true});
            calls[client_id].conn = new SipCallConnection({gateway: gateway, clientID: client_id, audio : info.audio, video : info.video});
            return setupCall(client_id, info);
        })
        .then(function(result) {
            log.debug('handleCallUpdated re-setup call ok');
            calls[client_id].updating = undefined;

            if (calls[client_id].latestInfo) {
                // Process saved latest update request
                log.info('Received call update request during updating');
                var latestInfo = calls[client_id].latestInfo;
                calls[client_id].latestInfo = undefined;

                handleCallUpdated(latestInfo);
            }
        }).catch(function(err) {
            log.error('Error during call updating:', err);
        });
    };

    var handleCallClosed = function (peerURI) {
        var client_id = peerURI;

        log.info('CallClosed:', client_id);
        if (calls[client_id]) {
            teardownCall(client_id);
            calls[client_id].conn.close({input: true, output: true});
            do_leave(calls[client_id].session_controller, client_id);
            delete calls[client_id];
        }
    };

    that.init = function(options, callback) {
        log.debug('init SipGateway:', options.sip_server, options.sip_user);

        room_id = options.room_id;

        gateway = new woogeenSipGateway.SipGateway();

        gateway.addEventListener('SIPRegisterOK', function() {
            callback('callback', 'ok');
        });

        gateway.addEventListener('SIPRegisterFailed', function() {
            log.error("Register Failed");
            callback('callback', 'error', 'SIP registration fail');
        });

        if (!gateway.register(options.sip_server, options.sip_user, options.sip_passwd, options.sip_user)) {
            log.error("Register error!");
            callback('callback', 'error', 'SIP registration fail');
        }

        gateway.addEventListener('IncomingCall', function(peerURI) {
            log.info('IncommingCall: ', peerURI);
            if (calls[peerURI] === undefined) {
                if (!recycling_mode) {
                    handleIncomingCall(peerURI, function () {
                        log.info('Accept call');
                        gateway.accept(peerURI);
                    }, function (reason) {
                        log.error('reject call error: ', reason);
                        gateway.reject(peerURI);
                    });
                } else {
                    gateway.reject(peerURI);
                    log.info('working on recycling mode, do not accept incoming call');
                }
            } else {
                log.error('Duplicated call from same user, ignore it.');
            }
        });

        gateway.addEventListener('CallEstablished', function(data) {
            var info = JSON.parse(data);
            handleCallEstablished(info);
        });

        gateway.addEventListener('CallUpdated', function(data) {
            var info = JSON.parse(data);
            handleCallUpdated(info);
        });

        gateway.addEventListener('CallClosed', function(peerURI) {
            handleCallClosed(peerURI);
        });
    };

    that.clean = function() {
        log.debug('Clean SipGateway');

        recycling_mode = true;
        for (var client_id in calls) {
            log.debug('force leaving room ', room_id, ' user: ', client_id);
            gateway.hangup(client_id);
            teardownCall(client_id);
            calls[client_id].conn.close({input: true, output: true});
            do_leave(calls[client_id].session_controller, client_id);
            delete calls[client_id];
        }
        gateway.close();
        gateway = undefined;
    };

    that.publish = function (stream_id, stream_type, options, callback) {
        log.debug('publish stream_id:', stream_id, ', stream_type:', stream_type, ', audio:', options.audio, ', video:', options.video);
        if (stream_type === 'internal') {
            if (streams[stream_id] === undefined) {
                var conn = new InternalIn(options.protocol, GLOBAL.config.internal.minport, GLOBAL.config.internal.maxport);
                streams[stream_id] = {type: stream_type, connection: conn};
                callback('callback', {ip: that.clusterIP, port: conn.getListeningPort()});
            } else {
                callback('callback', {ip: that.clusterIP, port: streams[stream_id].connection.getListeningPort()});
            }
        } else {
            log.error('Stream type invalid:'+stream_type);
            callback('callback', {type: 'failed', reason: 'Stream type invalid:'+stream_type});
        }
    };

    that.unpublish = function (stream_id, callback) {
        log.debug('unpublish enter, stream_id:', stream_id);
        if (streams[stream_id]) {
            for (var subscription_id in subscriptions) {
                if (subscriptions[subscription_id].audio === stream_id) {
                    log.debug('remove audio:', subscriptions[subscription_id].audio);
                    var dest = subscriptions[subscription_id].connection.receiver('audio');
                    log.debug('remove audio removeDestination: ', streams[stream_id].connection, ' dest: ', dest);
                    streams[stream_id].connection.removeDestination('audio', dest);
                    subscriptions[subscription_id].audio = undefined;
                }

                if (subscriptions[subscription_id].video === stream_id) {
                    log.debug('remove video:', subscriptions[subscription_id].video);
                    var dest = subscriptions[subscription_id].connection.receiver('video');
                    log.debug('remove video removeDestination: ', streams[stream_id].connection, ' dest: ', dest);
                    streams[stream_id].connection.removeDestination('video', dest);
                    subscriptions[subscription_id].video = undefined;
                }
            }
            streams[stream_id].connection.close();
            delete streams[stream_id];
            callback('callback', 'ok');
        } else {
            log.info('Connection does NOT exist:' + stream_id);
            callback('callback', 'error', 'Connection does NOT exist:' + stream_id);
        }
    };

    that.subscribe = function (subscription_id, subscription_type, options, callback) {
        log.debug('subscribe, subscription_id:', subscription_id, ', subscription_type:', subscription_type, ',options:', options);
        if (subscription_type === 'internal') {
            if (subscriptions[subscription_id] === undefined) {
                var conn = new InternalOut(options.protocol, options.dest_ip, options.dest_port);
                subscriptions[subscription_id] = {type: subscription_type,
                                                  audio: undefined,
                                                  video: undefined,
                                                  connection: conn};
            }
            callback('callback', 'ok');
        } else {
            log.error('Stream type invalid:'+stream_type);
            callback('callback', {type: 'failed', reason: 'Stream type invalid:'+stream_type});
        }
    };

    that.unsubscribe = function (subscription_id, callback) {
        log.debug('unsubscribe, subscription_id:', subscription_id);

        if (subscriptions[subscription_id] !== undefined) {
            if (subscriptions[subscription_id].audio
                && streams[subscriptions[subscription_id].audio]) {
                var dest = (subscriptions[subscription_id].type === 'sip'?
                    subscriptions[subscription_id].connection.receiver('audio') :
                    subscriptions[subscription_id].connection);
                log.debug("connection: ", streams[subscriptions[subscription_id].audio].connection, ' remove Dest: ', dest);
                streams[subscriptions[subscription_id].audio].connection.removeDestination('audio', dest);
            }

            if (subscriptions[subscription_id].video
                && streams[subscriptions[subscription_id].video]) {
                var dest = (subscriptions[subscription_id].type === 'sip'?
                    subscriptions[subscription_id].connection.receiver('video') :
                    subscriptions[subscription_id].connection);
                log.debug("connection: ", streams[subscriptions[subscription_id].video].connection, ' remove Dest: ', dest);
                streams[subscriptions[subscription_id].video].connection.removeDestination('video', dest);
            }

            subscriptions[subscription_id].connection.close();
            delete subscriptions[subscription_id];
            callback('callback', 'ok');
        } else {
            log.info('Connection does NOT exist:' + subscription_id);
            callback('callback', 'error', 'Connection does NOT exist:' + subscription_id);
        }
    };

    that.linkup = function (connectionId, audioFrom, videoFrom, callback) {
        log.debug('linkup, connectionId:', connectionId, ', audioFrom:', audioFrom, ', videoFrom:', videoFrom);

        if (audioFrom && streams[audioFrom] === undefined) {
            log.error('Audio stream does not exist:' + audioFrom);
            return callback('callback', {type: 'failed', reason: 'Audio stream does not exist:' + audioFrom});
        }

        if (videoFrom && streams[videoFrom] === undefined) {
            log.error('Video stream does not exist:' + videoFrom);
            return callback('callback', {type: 'failed', reason: 'Video stream does not exist:' + videoFrom});
        }

        var conn = subscriptions[connectionId] && subscriptions[connectionId].connection;

        if (!conn) {
            log.error('connectionId not valid:', connectionId);
            return callback('callback', {type: 'failed', reason: 'connectionId not valid:' + connectionId});
        }

        var is_sip = (subscriptions[connectionId].type === 'sip');

        if (audioFrom) {
            var dest = (is_sip ? conn.receiver('audio') : conn);
            log.debug("subscribe addDestination: ", streams[audioFrom].connection, " dest: ", dest);
            streams[audioFrom].connection.addDestination('audio', dest);
            subscriptions[connectionId].audio = audioFrom;
        }

        if (videoFrom) {
            var dest = (is_sip ? conn.receiver('video') : conn);
            streams[videoFrom].connection.addDestination('video', dest);
            if (streams[videoFrom].type === 'sip') {streams[videoFrom].connection.requestKeyFrame();}//FIXME: Temporarily add this interface to workround the hardware mode's absence of feedback mechanism.
            subscriptions[connectionId].video = videoFrom;
        }

        callback('callback', 'ok');
    };

    that.cutoff = function (connectionId, callback) {
        log.debug('cutoff, connectionId:', connectionId);
        if (subscriptions[connectionId]) {
            var is_sip = (subscriptions[connectionId].type === 'sip');
            if (subscriptions[connectionId].audio
                && streams[subscriptions[connectionId].audio]) {
                var dest = is_sip ? subscriptions[connectionId].connection.receiver('audio') : subscriptions[connectionId].connection;
                log.debug("connection: ", streams[subscriptions[connectionId].audio].connection, ' remove Dest: ', dest);
                streams[subscriptions[connectionId].audio].connection.removeDestination('audio', dest);
                subscriptions[connectionId].audio = undefined;
            }

            if (subscriptions[connectionId].video
                && streams[subscriptions[connectionId].video]) {
                var dest = is_sip ? subscriptions[connectionId].connection.receiver('video') : subscriptions[connectionId].connection;
                log.debug("connection: ", streams[subscriptions[connectionId].video].connection, ' remove Dest: ', dest);
                streams[subscriptions[connectionId].video].connection.removeDestination('video', dest);
                subscriptions[connectionId].video = undefined;
            }
            callback('callback', 'ok');
        } else {
            log.info('Connection does NOT exist:' + connectionId);
            callback('callback', 'error', 'Connection does NOT exist:' + connectionId);
        }
    };

    that.drop = function(participantId, fromRoom, callback) {
        that.clean();
        callback('callback', 'ok');
    };

    that.notify = function(participantId, event, data, callback) {
        //TODO: notify text message to sip end.
        callback('callback', 'ok');
    };

    that.close = function() {
        if (gateway) {
            this.clean();
        }
    };

    return that;
};
