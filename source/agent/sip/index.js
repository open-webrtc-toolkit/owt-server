/*global require, exports*/
'use strict';

var woogeenInternalIO = require('./internalIO/build/Release/internalIO');
var woogeenSipGateway = require('./sipIn/build/Release/sipIn');
var SipCallConnection = require('./sipCallConnection').SipCallConnection;
var InternalIn = woogeenInternalIO.In;
var InternalOut = woogeenInternalIO.Out;
var logger = require('./logger').logger;
var amqper = require('./amqper');
var path = require('path');
var makeRPC = require('./makeRPC').makeRPC;

var cluster_name = ((GLOBAL.config || {}).cluster || {}).name || 'woogeen-cluster';

// Logger
var log = logger.getLogger('SipNode');

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
        amqper,
        session_ctl,
        'join',
        [room, {id: user, name: user, role: 'presenter', portal: selfPortal}], function(joinResult) {
            log.debug('join ok');
            safeCall(ok, joinResult.streams[0].id);
        }, function (reason) {
            safeCall(err,reason);
        });
}

function do_leave(session_ctl, user) {
    makeRPC(
        amqper,
        session_ctl,
        'leave',
        [user],
        function() {
            log.debug('leave ok');
        });

}
function do_query(session_ctl, user, room, ok, err) {
    makeRPC(
        amqper,
        session_ctl,
        'query',
        [user, room], function(streams) {
            safeCall(ok, streams);
        }, function(reason) {
            safeCall(err, reason);
        });
}

function do_publish(session_ctl, user, stream_id, accessNode, stream_info, ok, err) {
    makeRPC(
        amqper,
        session_ctl,
        'publish',
        [user, stream_id, accessNode, stream_info, false], function() {
            safeCall(ok);
        }, function(reason) {
            safeCall(err, reason);
        });
}

function do_subscribe(session_ctl, user, subscription_id, accessNode, subInfo, ok, err) {
    makeRPC(
        amqper,
        session_ctl,
        'subscribe',
        [user, subscription_id, accessNode, subInfo], function() {
            safeCall(ok);
        }, function(reason) {
            safeCall(err, reason);
        });
}

function do_unpublish(session_ctl, user, stream_id) {
    makeRPC(
        amqper,
        session_ctl,
        'unpublish',
        [user, stream_id],
        function() {
            log.debug('unpublish ok');
        });
}

function do_unsubscribe(session_ctl, user, subscription_id) {
    makeRPC(
        amqper,
        session_ctl,
        'unsubscribe',
        [user, subscription_id],
        function() {
            log.debug('unsubscribe ok');
        });
}

var getSessionControllerForRoom = function (roomId, on_ok, on_error) {
    var keepTrying = true;

    var tryFetchingSessionController = function (attempts) {
        if (attempts <= 0) {
            return on_error('Timeout to fetech controller');
        }

        log.info('Send controller schedule RPC request to ', cluster_name, ' for room ', roomId);

        makeRPC(amqper, cluster_name, 'schedule', ['session', roomId, 60 * 1000],
            function (result) {
                makeRPC(amqper, result.id, 'getNode', [{session: roomId, consumer: roomId}], function (sessionController) {
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

module.exports = function (spec) {
    var that = {},
        erizo = {id:spec.id, addr:spec.addr},
        room_id,
        mixed_stream_id,
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
            do_join(session_controller, client_id, room_id, erizo.id, function(sid) {
                mixed_stream_id = sid;
                on_ok();
            }, function (err) {
                on_error(err);
            });
        }, on_error);
    };

    var handleCallEstablished = function (info) {
        var client_id = info.peerURI;

        if (calls[client_id]) {
            var session_controller = calls[client_id].session_controller;
            log.info('CallEstablished:', client_id, 'audio='+info.audio, 'video='+info.video, ' audio codec:', info.audio_codec, (info.video ? (' video codec: ' + info.video_codec) : ''));
            // the user join the room
            var stream_id = generateStreamId();
            var audio_info, video_info;
            // TODO yaojie need retrive the info from sipua
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
                // codec list
                video_info = {codec:info.video_codec.toLowerCase(), resolution:'vga', framerate:30};
            }

            var subInfo = {};
            if (audio_info) {
                subInfo.audio = {
                    fromStream: mixed_stream_id,
                    codecs: [audio_info.codec]
                };
            }
            if (video_info) {
                subInfo.video = {
                    fromStream: mixed_stream_id,
                    codecs: [video_info.codec],
                    resolution: 'vga'
                };
            }

            // var st = new ST.Stream({id: stream_id, audio: info.audio, video: info.video, screen: false});
            var conn = new SipCallConnection({gateway: gateway, clientID: client_id, audio : info.audio, video : info.video});
            streams[stream_id] = {type: 'sip', connection: conn};
            calls[client_id].stream_id = stream_id;
            calls[client_id].conn = conn;

            do_publish(session_controller,
                       client_id,
                       stream_id,
                       {agent:that.agentID, node: erizo.id},
                       {audio: audio_info, video:video_info},
                       function() {
                           // conn.enablePort('in');
                           log.debug("publish stream OK!");
                       }, function() {
                           log.error("publish stream failed!");
                       });

            subscriptions[client_id] = {type: 'sip',
                                        audio: undefined,
                                        video: undefined,
                                        connection: conn};
            // subscribe the mix streams
            if (mixed_stream_id) {
                calls[client_id].mix_stream_id = mixed_stream_id;
                do_subscribe(session_controller,
                             client_id,
                             client_id,
                             {agent:that.agentID, node: erizo.id},
                             subInfo,
                             function() {
                                log.info("subscribe stream ok");
                             }, function(err) {
                                log.error("subscribe stream failed with  ", err);
                             });
            } else {
                log.warn("invalid mix stream id");
            }

            //subscribe remote stream
            //streams[stream_id] = {stream:st};
        } else {
            log.error("gateway can not handle event with invalid status");
        }
    };

    var handleCallUpdated = function (info) {
        log.info('CallUpdated:', info, calls);

        var client_id = info.peerURI;
        if(calls[client_id] === undefined || calls[client_id].session_controller === undefined) {
            log.warn("Too frequent call update request, ignore it");
            return;
        }
        var session_controller = calls[client_id].session_controller;
        do_leave(session_controller, client_id);
        delete calls[client_id];

        do_join(session_controller, client_id, room_id, erizo.id, function(sid) {
            mixed_stream_id = sid;
            calls[client_id] = {session_controller : session_controller};
            handleCallEstablished(info);
        }, function (err) {
            log.error("Handle call update error: ", err);
        });
    };

    var handleCallClosed = function (peerURI) {
        var client_id = peerURI;

        log.info('CallClosed:', client_id);
        if (calls[client_id]) {
            var session_controller = calls[client_id].session_controller;

            log.debug('try to leave room ', room_id, ' user: ', client_id);
            do_leave(session_controller, client_id);
            delete calls[client_id];
            log.debug('complete leaving room ', room_id, ' user: ', client_id);
        } else {
          log.error("gateway can not handle event with invalid status");
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
            do_leave(calls[client_id].session_controller, client_id);
        }
        gateway.close();
    };

    // for every stream
    // stream_id = the sip client uri.
    that.publish = function (stream_id, stream_type, options, callback) {
        // finds the stream
        log.debug('publish stream_id:', stream_id, ', stream_type:', stream_type, ', audio:', options.has_audio, ', video:', options.has_video);
        if (streams[stream_id]) {
            log.error('Stream already exists:'+stream_id);
            callback('callback', {type: 'failed', reason: 'Stream already exists:'+stream_id});
            return;
        }

        var conn;
        if (stream_type === 'internal') {
            conn = new InternalIn(options.protocol);
            callback('callback', {ip: that.clusterIP, port: conn.getListeningPort()});
            streams[stream_id] = {type: stream_type, connection: conn};
        } else {
            log.error('Stream type invalid:'+stream_type);
            callback('callback', {type: 'failed', reason: 'Stream type invalid:'+stream_type});
            return;
        }
    };

    that.unpublish = function (stream_id, callback) {
        log.debug('unpublish enter, stream_id:', stream_id);
        if (streams[stream_id]) {
            for (var subscription_id in subscriptions) {
                var is_sip = (subscriptions[subscription_id].type === 'sip');
                if (subscriptions[subscription_id].audio === stream_id) {
                    log.debug('remove audio:', subscriptions[subscription_id].audio);
                    var dest = (is_sip ? subscriptions[subscription_id].connection.receiver('audio'):subscriptions[subscription_id].connection);
                    log.debug('remove audio removeDestination: ', streams[stream_id].connection, ' dest: ', dest);
                    streams[stream_id].connection.removeDestination('audio', dest);
                    subscriptions[subscription_id].audio = undefined;
                }

                if (subscriptions[subscription_id].video === stream_id) {
                    log.debug('remove video:', subscriptions[subscription_id].video);
                    var dest = (is_sip ? subscriptions[subscription_id].connection.receiver('video'):subscriptions[subscription_id].connection);
                    log.debug('remove video removeDestination: ', streams[stream_id].connection, ' dest: ', dest);
                    streams[stream_id].connection.removeDestination('video', dest);
                    subscriptions[subscription_id].video = undefined;
                }

                subscriptions[subscription_id].connection.close({output:true});
                delete subscriptions[subscription_id];
            }
            if (streams[stream_id].type !== 'sip'){
                streams[stream_id].connection.close();
            } else {
                streams[stream_id].connection.close({input:true});
            }
            delete streams[stream_id];
            callback('callback', 'ok');
        } else {
            log.info('Connection does NOT exist:' + stream_id);
            callback('callback', 'error', 'Connection does NOT exist:' + stream_id);
        }
    };

    that.subscribe = function (subscription_id, subscription_type, options, callback) {
        log.debug('subscribe, subscription_id:', subscription_id, ', subscription_type:', subscription_type, ',options:', options);
        if (subscriptions[subscription_id]) {
            log.error('Connection already exists:'+subscription_id);
            callback('callback', {type: 'failed', reason: 'Connection already exists:'+subscription_id});
            return;
        }

        var is_sip = (subscription_type === 'sip');

        var conn;

        switch (subscription_type) {
            case 'internal':
                conn = new InternalOut(options.protocol, options.dest_ip, options.dest_port);
                break;
            case "sip":
                conn = calls[subscription_id].conn;
                if (conn === undefined)
                    log.error('no sip connect');
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
        callback('callback', 'ok');
    };

    that.unsubscribe = function (subscription_id, callback) {
        var reserve_subscription = (arguments[1]===true);
        log.debug('unsubscribe enter, subscription_id:', subscription_id);

        if (subscriptions[subscription_id] !== undefined) {
            var is_sip = (subscriptions[subscription_id].type === 'sip');
            if (subscriptions[subscription_id].audio
                && streams[subscriptions[subscription_id].audio]) {
                var dest = is_sip ? subscriptions[subscription_id].connection.receiver('audio') : subscriptions[subscription_id].connection;
                log.debug("connection: ", streams[subscriptions[subscription_id].audio].connection, ' remove Dest: ', dest);
                streams[subscriptions[subscription_id].audio].connection.removeDestination('audio', dest);
            }

            if (subscriptions[subscription_id].video
                && streams[subscriptions[subscription_id].video]) {
                var dest = is_sip ? subscriptions[subscription_id].connection.receiver('video') : subscriptions[subscription_id].connection;
                log.debug("connection: ", streams[subscriptions[subscription_id].video].connection, ' remove Dest: ', dest);
                streams[subscriptions[subscription_id].video].connection.removeDestination('video', dest);
            }

           if (!reserve_subscription) {
                if (is_sip) {
                    subscriptions[subscription_id].connection.close({output:true});
                } else {
                    subscriptions[subscription_id].connection.close();
                }
                delete subscriptions[subscription_id];
            }

            callback('callback', 'ok');
        } else {
            log.info('Connection does NOT exist:' + subscription_id);
            callback('callback', 'error', 'Connection does NOT exist:' + subscription_id);
        }

        log.debug('unsubscribe exit');
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

    return that;
};
