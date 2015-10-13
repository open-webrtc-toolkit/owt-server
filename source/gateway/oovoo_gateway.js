/*global require, logger. setInterval, clearInterval, Buffer, exports*/
var Controller = require('./controller');
var ST = require('./util/Stream');
var logger = require('./util/logger').logger;
var Permission = require('./util/permission');
var config = require('./../etc/gateway_config');
var fs = require("fs");
var path = require("path");
var conns = {};
var dirname = __dirname || path.dirname(fs.readlinkSync('/proc/self/exe'));
var kaConfigFile = path.resolve(dirname, '../etc/', 'heartbeat_config.json');
var accumulatedClientInfo = {};
var accumulatedClients = 0;

function getPublicIP() {
    var interfaces = require("os").networkInterfaces(),
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
                        addresses.push(address.address);
                    }
                }
            }
        }
    }
    return addresses[0];
}

var publicIP = getPublicIP();
var privateRegexp = new RegExp(publicIP, 'g');
publicIP = config.gateway.publicIP || publicIP;

var https_options = {
  key: fs.readFileSync(path.resolve(dirname, config.gateway.certificate.key)).toString(),
  cert: fs.readFileSync(path.resolve(dirname, config.gateway.certificate.cert)).toString(),
  passphrase: config.gateway.certificate.passphrase
};

if (config.gateway.certificate.ca) {
    https_options.ca = fs.readFileSync(path.resolve(dirname, config.gateway.certificate.ca)).toString();
}

var server = require("http").createServer().listen(config.gateway.port || 8080);
var servers = require("https").createServer(https_options).listen(config.gateway.sport || 8443);
var io = require('socket.io').listen(server, {log: false});
var ios = require('socket.io').listen(servers, {log: false});

var notify = function (sock, type, arg) { // sock: a socket, or socket.broadcast
    "use strict";
    logger.info('Notify Event:', type);
    sock.emit(type, arg);
};

var listen = function (io) {
    "use strict";
    var osRegExp = /windows|linux|mac os x|android/;
    var browserRegExp = /chrome|firefox|opera/;

    io.sockets.on('connection', function (socket) {
        logger.info("Socket connect ", socket.id);
        var ison = false;
        var sessionId = '';
        var clientId = '';
        var client_leaving = false;
        var ua = (socket.handshake.headers['user-agent'] || 'others').toLowerCase();
        var client_info = ((browserRegExp.exec(ua) || 'others') + '_' + (osRegExp.exec(ua) || 'others')).replace(/ /g, '_');
        socket.on('token', function (token, callback) {
            var Client = token;
            socket.conn = {
                streams: {},
                controller: new Controller()
            };
            socket.conn.controller.init(config.oovoo.codecCapability);

            conns[socket.id] = socket.conn;
            socket.user = {name: Client.username, role: Client.role};
            var permissions = config.roles[Client.role] || [];
            socket.user.permissions = {};
            for (var i in permissions) {
                var permission = permissions[i];
                socket.user.permissions[permission] = true;
            }
            socket.state = 'sleeping';

            socket.conn.controller.addEventListener('ClientJoin', function(userId) {
                ison = true;
                logger.info('Client joining:', userId);
                clientId = userId;
                notify(socket, 'onClientId', clientId);
            });
            socket.conn.controller.addEventListener('ClientLeave', function(userId) {
                ison = false;
                logger.info('Client leaving:', userId);
                socket.conn.controller.keepAlive = true;
                leaveConn();
                socket.conn.controller.keepAlive = false;
            });
            socket.conn.controller.addEventListener('InboundVideoCreate', function(streamId) {
                logger.debug('Inbound Video Create:', streamId);
                notify(socket, 'onVideoReady', {id: streamId});
            });
            socket.conn.controller.addEventListener('InboundVideoClose', function(streamId) {
                logger.debug('Inbound Video Close:', streamId);
                notify(socket, 'onVideoHold', {id: streamId});
            });
            socket.conn.controller.addEventListener('InboundAudioCreate', function(streamId) {
                logger.debug('Inbound Audio Create:', streamId);
                notify(socket, 'onAudioReady', {id: streamId});
            });
            socket.conn.controller.addEventListener('InboundAudioClose', function(streamId) {
                logger.debug('Inbound Audio Close:', streamId);
                notify(socket, 'onAudioHold', {id: streamId});
            });
            socket.conn.controller.addEventListener('AllInboundAudioCreate', function(arg) {
                logger.debug('All inbound Audio Create');
                notify(socket, 'onAllAudioReady');
            });
            socket.conn.controller.addEventListener('AllInboundAudioClose', function(arg) {
                logger.debug('All inbound Audio Close');
                notify(socket, 'onAllAudioHold');
            });
            socket.conn.controller.addEventListener('OutboundVideoCreate', function(streamId) {
                logger.debug('Outbound Video Create:', streamId);
                notify(socket, 'onVideoOn', {id: streamId});
            });
            socket.conn.controller.addEventListener('OutboundVideoClose', function(streamId) {
                logger.debug('Outbound Video Close:', streamId);
                notify(socket, 'onVideoOff', {id: streamId});
            });
            socket.conn.controller.addEventListener('OutboundAudioCreate', function(streamId) {
                logger.debug('Outbound Audio Create:', streamId);
                notify(socket, 'onAudioOn', {id: streamId});
            });
            socket.conn.controller.addEventListener('OutboundAudioClose', function(streamId) {
                logger.debug('Outbound Audio Close:', streamId);
                notify(socket, 'onAudioOff', {id: streamId});
            });
            socket.conn.controller.addEventListener('CreateInboundClient', function(msg) {
                var user = JSON.parse(msg);
                logger.info('Create Inbound Client: id', user.userId, "name", user.name, "participant info", user.info);
                notify(socket, 'onClientJoin', {user: user});
                var id = parseInt(user.userId, 10);
                var st = new ST.Stream({id: id, audio: true, video: true, screen: false});
                socket.conn.streams[id] = st;
                notify(socket, 'onAddStream', st.getPublicStream());
            });
            socket.conn.controller.addEventListener('CloseInboundClient', function(userId) {
                logger.info('Close Inbound Client:', userId);
                var id = parseInt(userId, 10);
                socket.conn.controller.removeSubscriber(socket.id, id);
                notify(socket, 'onRemoveStream', {id: id});
                if (socket.conn.streams[id]) {
                    delete socket.conn.streams[id];
                }
                notify(socket, 'onClientLeave', {user: userId});
            });
            socket.conn.controller.addEventListener('CustomMessage', function(msg) {
                logger.info('Custom Message:', msg);
                notify(socket, 'onCustomMessage', {msg: msg});
            });

            socket.on('customMessage', function (msg, callback) {
                logger.debug(msg);
                switch (msg.type) {
                    case 'login': // TODO: error handling
                    if (typeof msg.payload === 'object' && msg.payload !== null) {
                        socket.conn.controller.customMessage(msg);
                        sessionId = msg.payload.conf_key + '';
                        socket.join(sessionId);
                        var sessionInfo = [sessionId, msg.payload.avs_ip].join('##');
                        logger.info('Set session => [%s]', sessionInfo);
                        if (typeof callback === 'function') callback('success', sessionInfo);
                        // set client info until successfully login
                        if (!accumulatedClientInfo[client_info]) accumulatedClientInfo[client_info] = 1;
                        else accumulatedClientInfo[client_info]++;
                        ++accumulatedClients;
                    } else {
                        if (typeof callback === 'function') callback('error');
                    }
                    break;
                    case 'control':
                    if (typeof msg.payload === 'object' && msg.payload !== null) {
                        var action = msg.payload.action;
                        var streamId = msg.payload.streamId;
                        if (/^((audio)|(video))-((in)|(out))-((on)|(off))$/.test(action)) {
                            action = socket.conn.controller[action];
                            if (typeof streamId === 'string') {
                                streamId = parseInt(streamId, 10);
                            }
                            if (typeof action === 'function') {
                                action(streamId);
                                if (typeof callback === 'function') callback('success');
                                return;
                            }
                        }
                    }
                    if (typeof callback === 'function') callback('error');
                    break;
                    default:
                    socket.conn.controller.customMessage(msg);
                    if (typeof callback === 'function') callback('success');
                }
            });
            if (typeof callback === 'function') callback('success', {
                id: socket.id,
                defaultVideoBW: config.gateway.defaultVideoBW,
                maxVideoBW: config.gateway.maxVideoBW,
                stunServerUrl: config.gateway.stunServerUrl,
                turnServer: config.gateway.turnServer
            });
        });

        var leaveConn = function () {
            logger.info(socket.id+' leaving connection.');
            if (socket.conn !== undefined) {
                if (socket.conn.controller) {
                    (function recycle() {
                        if (socket.conn.controller.keepAlive !== true) {
                            socket.conn.controller.removeClient(socket.id);
                            if (!client_leaving) {
                                socket.disconnect();
                            }
                            return;
                        }
                        process.nextTick(recycle);
                    }());
                }
                for (var i in socket.conn.streams) {
                    if (socket.conn.streams.hasOwnProperty(i)) {
                        delete socket.conn.streams[i];
                    }
                }
            }
        };

        socket.on('signaling_message', function (msg) {
            socket.conn.controller.processSignaling(msg.streamId, socket.id, msg.msg);
        });

        //Gets 'publish' messages on the socket in order to add new stream to the conn.
        socket.on('publish', function (options, sdp, callback) {
            if (!ison) {
                if (typeof callback === 'function') callback('error', 'not ready');
                return;
            }
            var id = socket.id, st;
            if (!socket.user.permissions[Permission.PUBLISH]) {
                if (typeof callback === 'function') callback('error', 'unauthorized');
                return;
            }
            if (options.state === 'url') {
                socket.conn.controller.addExternalInput(id, sdp, function (result) {
                    if (result === 'success') {
                        st = new ST.Stream({id: id, audio: options.audio, video: options.video, attributes: options.attributes});
                        socket.conn.streams[id] = st;
                        if (typeof callback === 'function') callback(result, id);
                        notify(socket, 'onAddStream', st.getPublicStream());
                    } else {
                        if (typeof callback === 'function') callback(result);
                    }
                });
            } else {
                if (options.state === 'erizo') {
                    options.attributes = options.attributes || {};
                    var resolution = options.attributes.resolution || '';
                    socket.conn.controller.addPublisher(id, function (signMess) {
                        if (signMess.type === 'initializing') {
                            if (typeof callback === 'function') callback('initializing', {id: id});
                            st = new ST.Stream({id: id, audio: options.audio, video: options.video, screen: options.screen, attributes: options.attributes});
                            return;
                        }

                        if (signMess.type === 'ready') {
                            socket.conn.streams[id] = st;
                            notify(socket, 'onPublishStream', {id: id});
                        }

                        socket.emit('signaling_message_erizo', {mess: signMess, streamId: id});
                    }, resolution);
                }
                /*
                if (options.state === 'offer' && socket.state === 'sleeping') {
                    options.attributes = options.attributes || {};
                    var resolution = options.attributes.resolution || '';
                    socket.conn.controller.addPublisher(id, sdp, function (answer) {
                        socket.state = 'waitingOk';
                        answer = answer.replace(privateRegexp, publicIP);
                        if (typeof callback === 'function') callback(answer, {id: id});
                    }, function() {
                        notify(socket, 'onPublishStream', {id: id});
                    }, resolution);

                } else if (options.state === 'ok' && socket.state === 'waitingOk') {
                    st = new ST.Stream({id: id, audio: options.audio, video: options.video, screen: options.screen, attributes: options.attributes});
                    socket.state = 'sleeping';
                    socket.conn.streams[id] = st;
                }
                */
            }
        });

        //Gets 'subscribe' messages on the socket in order to add new subscriber to a determined stream (options.streamId).
        socket.on('subscribe', function (options, sdp, callback) {
            if (!ison) {
                if (typeof callback === 'function') callback('error', 'not ready');
                return;
            }
            if (!socket.user.permissions[Permission.SUBSCRIBE]) {
                if (typeof callback === 'function') callback('error', 'unauthorized');
                return;
            }
            var stream = socket.conn.streams[options.streamId];

            if (stream === undefined) {
                if (typeof callback === 'function') callback('error', 'no such stream');
                return;
            }

            if (stream.hasAudio() || stream.hasVideo() || stream.hasScreen()) {
                socket.conn.controller.addSubscriber(socket.id, options.streamId, options.audio, options.video, function (signMess) {
                    if (signMess.type === 'initializing') {
                        if (typeof callback === 'function') callback('initializing');
                        return;
                    }
                    socket.emit('signaling_message_erizo', {mess: signMess, peerId: options.streamId});
                });
                /*
                socket.conn.controller.addSubscriber(socket.id, options.streamId, options.audio, options.video, sdp, function (answer) {
                    answer = answer.replace(privateRegexp, publicIP);
                    if (typeof callback === 'function') callback(answer);
                });
                */
            } else {
                if (typeof callback === 'function') callback(undefined);
            }

        });

        //Gets 'startRecorder' messages
        socket.on('startRecorder', function (options, callback) {
            if (!ison) {
                if (typeof callback === 'function') callback('error', 'not ready');
                return;
            }
            if (!socket.user.permissions[Permission.RECORD]) {
                if (typeof callback === 'function') callback('error', 'unauthorized');
                return;
            }
            var streamId = options.to;
            var url = options.url;
            logger.info("gateway: Starting recorder streamID " + streamId + " url " + url);
            if (socket.conn.streams[streamId].hasAudio() || socket.conn.streams[streamId].hasVideo() || socket.conn.streams[streamId].hasScreen()) {
                socket.conn.controller.addExternalOutput(streamId, url);
                logger.info("gateway: Recorder Started");
            }
        });

        socket.on('stopRecorder', function (options, callback) {
            if (!ison) {
                if (typeof callback === 'function') callback('error', 'not ready');
                return;
            }
            if (!socket.user.permissions[Permission.RECORD]) {
                if (typeof callback === 'function') callback('error', 'unauthorized');
                return;
            }
            logger.info("gateway: Stoping recorder to streamId " + options.to + " url " + options.url);
            socket.conn.controller.removeExternalOutput(options.to, options.url);
        });

        //Gets 'unpublish' messages on the socket in order to remove a stream from the conn.
        socket.on('unpublish', function (streamId, callback) {
            if (!ison) {
                if (typeof callback === 'function') callback('error', 'not ready');
                return;
            }
            if (!socket.user.permissions[Permission.PUBLISH]) {
                if (typeof callback === 'function') callback('error', 'unauthorized');
                return;
            }
            if (socket.conn.streams[streamId] === undefined) {
                if (typeof callback === 'function') callback('error', 'no such stream');
                return;
            }

            if (socket.conn.streams[streamId].hasAudio() || socket.conn.streams[streamId].hasVideo() || socket.conn.streams[streamId].hasScreen()) {
                socket.state = 'sleeping';
                socket.conn.controller.removePublisher(streamId);
            }

            delete socket.conn.streams[streamId];
            if (typeof callback === 'function') callback('success');
        });

        //Gets 'unsubscribe' messages on the socket in order to remove a subscriber from a determined stream (to).
        socket.on('unsubscribe', function (to, callback) {
            if (!ison) {
                if (typeof callback === 'function') callback('error', 'not ready');
                return;
            }
            if (!socket.user.permissions[Permission.SUBSCRIBE]) {
                if (typeof callback === 'function') callback('error', 'unauthorized');
                return;
            }
            if (socket.conn.streams[to] === undefined) {
                if (typeof callback === 'function') callback('error', 'no such stream');
                return;
            }

            if (socket.conn.streams[to].hasAudio() || socket.conn.streams[to].hasVideo() || socket.conn.streams[to].hasScreen()) {
                socket.conn.controller.removeSubscriber(socket.id, to);
            }
            if (typeof callback === 'function') callback('success');
        });

        //When a client leaves the gateway removes its streams from the conn if exists.
        socket.on('disconnect', function () {
            client_leaving = true;
            logger.info('Socket disconnect ', socket.id);
            leaveConn();
            logger.info('Deleting conn', socket.id);
            delete conns[socket.id];
            socket.conn = undefined;
        });
    });
};

listen(io);
listen(ios);

var gatherGatewayStatistics = function () {
    var gwCounterFile = process.env.GW_COUNTER_OUTPUT || '/tmp/webrtc_gateway_counters.json';
    var totalPacketsReceived = 0;
    var totalPacketsLost = 0;

    function calcAccumulatedClientInfo() {
        return Object.keys(accumulatedClientInfo).map(function (id) {
           return '"'+id+'": '+accumulatedClientInfo[id];
        }).join(' ,\n');
    }

    var intervalId = setInterval(function () {
        var text = "";
        if (accumulatedClients == 0) {
            text = '{\n"packetsReceived": 0 ,\n"packetsLost": 0 ,\n"averageRTT": 0 ,\n"connections": 0 ,\n' + calcAccumulatedClientInfo() + '}';
        } else {
            var accumulatedRTT = 0;
            var connectionsWithValidInfo = 0;
            for (var i in conns) {
                if (conns.hasOwnProperty(i) && conns[i].controller) {
                    var stats = conns[i].controller.retrieveGatewayStatistics();
                    if (stats !== undefined) {
                        totalPacketsReceived += stats.packetsReceived;
                        totalPacketsLost += stats.packetsLost;
                        accumulatedRTT += stats.averageRTT;
                        ++connectionsWithValidInfo;
                    }
                }
            }

            if (connectionsWithValidInfo > 0) {
                var averageRTT = accumulatedRTT / connectionsWithValidInfo;

                text = '{\n"packetsReceived": ' + totalPacketsReceived
                    + ' ,\n"packetsLost": ' + totalPacketsLost
                    + ' ,\n"averageRTT": ' + averageRTT
                    + ' ,\n"connections": ' + accumulatedClients
                    + ' ,\n' + calcAccumulatedClientInfo() + '\n}';
            }
        }

        // Don't update the statistics if we don't get a valid one this time.
        if (text !== "") {
            fs.writeFile(gwCounterFile, text, function (err) {
                if (err) {
                    logger.error('File write error', err);
                }
            });
        }

    }, 5000);

    return intervalId;
}

gatherGatewayStatistics();

(function() {
var opt;
var heartbeatId;

function startHeartBeat () {
    var heartbeat = require('./oovoo_heartbeat');
    opt.ip = publicIP;
    heartbeatId = heartbeat.init(opt).start(conns, function (status, response) {
       logger.info('HEART-BEAT:', status, response);
    }, function (err) {
        logger.error('HEART-BEAT Error:', err);
    });
}

try {
    opt = JSON.parse(fs.readFileSync(kaConfigFile));
} catch (err) {
    logger.error('Failed to load heart-beat configuration:', err);
    opt = {};
}

if (opt.enabled) {
    startHeartBeat();
}

process.on('SIGHUP', function() {
  logger.info('Reloading heart-beat configuration on SIGHUP');
  try {
    opt = JSON.parse(fs.readFileSync(kaConfigFile)) || {};
    if (opt.enabled) {
        if (!heartbeatId) {
            logger.info('HEART-BEAT: enabled');
        } else {
            clearInterval(heartbeatId);
            logger.info('HEART-BEAT: updated');
        }
        startHeartBeat();
    } else if (heartbeatId !== undefined) {
        clearInterval(heartbeatId);
        heartbeatId = undefined;
        logger.info('HEART-BEAT: disabled');
    }
  } catch (err) {
    logger.error('Failed to reload heart-beat configuration:', err);
  }
});

process.on('SIGPIPE', function () {
  logger.warn('SIGPIPE caught and ignored.');
});

}());
