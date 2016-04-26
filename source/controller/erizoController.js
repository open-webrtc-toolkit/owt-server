/*global require, Buffer, exports, GLOBAL, process*/
'use strict';
var crypto = require('crypto');
var rpcPublic = require('./rpc/rpcPublic');
var ST = require('./Stream');
var Permission = require('./permission');
var Getopt = require('node-getopt');
var fs = require('fs');
var toml = require('toml');
var log = require('./logger').logger.getLogger('ErizoController');
var path = require('path');

try {
  GLOBAL.config = toml.parse(fs.readFileSync('./controller.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

// Configuration default values
GLOBAL.config.controller = GLOBAL.config.controller || {};
// GLOBAL.config.controller.stunServerUrl = GLOBAL.config.controller.stunServerUrl || 'stun:stun.l.google.com:19302';
GLOBAL.config.controller.stunServerUrl = GLOBAL.config.controller.stunServerUrl;
GLOBAL.config.controller.defaultVideoBW = GLOBAL.config.controller.defaultVideoBW || 300;
GLOBAL.config.controller.maxVideoBW = GLOBAL.config.controller.maxVideoBW || 300;
GLOBAL.config.controller.ip_address = GLOBAL.config.controller.ip_address || '';
GLOBAL.config.controller.hostname = GLOBAL.config.controller.hostname|| '';
GLOBAL.config.controller.port = GLOBAL.config.controller.port || 8080;
GLOBAL.config.controller.ssl = GLOBAL.config.controller.ssl || false;
GLOBAL.config.controller.turnServer = GLOBAL.config.controller.turnServer || undefined;
if (GLOBAL.config.controller.turnServer !== undefined) {
    GLOBAL.config.controller.turnServer.url = GLOBAL.config.controller.turnServer.url || '';
    GLOBAL.config.controller.turnServer.username = GLOBAL.config.controller.turnServer.username || '';
    GLOBAL.config.controller.turnServer.password = GLOBAL.config.controller.turnServer.password || '';
}
GLOBAL.config.controller.warning_n_rooms = (GLOBAL.config.controller.warning_n_rooms !== undefined ? GLOBAL.config.controller.warning_n_rooms : 15);
GLOBAL.config.controller.limit_n_rooms = (GLOBAL.config.controller.limit_n_rooms !== undefined ? GLOBAL.config.controller.limit_n_rooms : 20);
GLOBAL.config.controller.report.session_events = GLOBAL.config.controller.report.session_events || false;
GLOBAL.config.controller.roles = GLOBAL.config.controller.roles || {'presenter':{'publish': true, 'subscribe':true, 'record':true}, 'viewer':{'subscribe':true}, 'viewerWithData':{'subscribe':true, 'publish':{'audio':false,'video':false,'screen':false,'data':true}}};

GLOBAL.config.cluster = GLOBAL.config.cluster || {};
GLOBAL.config.cluster.name = GLOBAL.config.cluster.name || 'woogeen-cluster';
GLOBAL.config.cluster.join_retry = GLOBAL.config.cluster.join_retry || 5;
GLOBAL.config.cluster.join_interval = GLOBAL.config.cluster.join_interval || 3000;
GLOBAL.config.cluster.recover_interval = GLOBAL.config.cluster.recover_interval || 1000;
GLOBAL.config.cluster.keep_alive_interval = GLOBAL.config.cluster.keep_alive_interval || 1000;
GLOBAL.config.cluster.report_load_interval = GLOBAL.config.cluster.report_load_interval || 1000;
GLOBAL.config.cluster.max_load = GLOBAL.config.cluster.max_laod || 0.85;
GLOBAL.config.cluster.network_max_scale = GLOBAL.config.cluster.network_max_scale || 1000;

GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
GLOBAL.config.rabbit.host = GLOBAL.config.host || 'localhost';
GLOBAL.config.rabbit.port = GLOBAL.config.port || 5672;


// Parse command line arguments
var getopt = new Getopt([
  ['r' , 'rabbit-host=ARG'            , 'RabbitMQ Host'],
  ['g' , 'rabbit-port=ARG'            , 'RabbitMQ Port'],
  ['l' , 'logging-config-file=ARG'    , 'Logging Config File'],
  ['t' , 'stunServerUrl=ARG'          , 'Stun Server URL'],
  ['b' , 'defaultVideoBW=ARG'         , 'Default video Bandwidth'],
  ['M' , 'maxVideoBW=ARG'             , 'Max video bandwidth'],
  ['i' , 'ip_address=ARG'               , 'Erizo Controller\'s public IP'],
  ['H' , 'hostname=ARG'               , 'Erizo Controller\'s hostname'],
  ['p' , 'port'                       , 'Port where Erizo Controller will listen to new connections.'],
  ['S' , 'ssl'                        , 'Erizo Controller\'s hostname'],
  ['T' , 'turn-url'                   , 'Turn server\'s URL.'],
  ['U' , 'turn-username'              , 'Turn server\'s username.'],
  ['P' , 'turn-password'              , 'Turn server\'s password.'],
  ['h' , 'help'                       , 'display this help']
]);

var opt = getopt.parse(process.argv.slice(2));

for (var prop in opt.options) {
    if (opt.options.hasOwnProperty(prop)) {
        var value = opt.options[prop];
        switch (prop) {
            case 'help':
                getopt.showHelp();
                process.exit(0);
                break;
            case 'rabbit-host':
                GLOBAL.config.rabbit.host = value;
                break;
            case 'rabbit-port':
                GLOBAL.config.rabbit.port = value;
                break;
            default:
                GLOBAL.config.controller[prop] = value;
                break;
        }
    }
}

var amqper = require('./amqper');
var clusterWorker = require('./clusterWorker');
var controller = require('./roomController');
var WARNING_N_ROOMS = GLOBAL.config.controller.warning_n_rooms;
var LIMIT_N_ROOMS = GLOBAL.config.controller.limit_n_rooms;
var BINDED_INTERFACE_NAME = GLOBAL.config.controller.networkInterface;
var myId;
var rooms = {};
var myState;
var worker;
var tokenKey;
var io;

var calculateSignature = function (token) {
    var toSign = token.tokenId + ',' + token.host,
        signed = crypto.createHmac('sha256', tokenKey).update(toSign).digest('hex');
    return (new Buffer(signed)).toString('base64');
};

var checkSignature = function (token) {
    var calculatedSignature = calculateSignature(token);

    if (calculatedSignature !== token.signature) {
        log.info('Auth fail. Invalid signature.');
        return false;
    } else {
        return true;
    }
};

/*
 * Sends a message of type 'type' to all sockets in a determined room.
 */
var sendMsgToRoom = function (room, type, arg) {
    if (room && room.sockets) {
        room.sockets.map(function (sock) {
            log.info('Sending message to', sock.id, 'in room ', room.id);
            sock.emit(type, arg);
        });
    }
};

var eventReportHandlers = {
    'updateStream': function (roomId, spec) {
        if (rooms[roomId]) {
            sendMsgToRoom(rooms[roomId], 'update_stream', spec);
        } else {
            log.warn('room not found:', roomId);
        }
    },
    'unpublish': function (roomId, spec) {
        var room = rooms[roomId];
        if (!room) {
            log.warn('room not found:', roomId);
            return;
        }

        var streamId = spec.id;
        log.info('Unpublishing stream:', streamId);
        room.controller.unpublish(streamId);

        room.sockets.map(function (socket) {
            var index = socket.streams.indexOf(streamId);
            if (index !== -1) {
                socket.streams.splice(index, 1);
            }
        });

        if (room.streams[streamId]) {
            sendMsgToRoom(room, 'remove_stream', {id: streamId});
            delete room.streams[streamId];
        }
    },
    'deleteExternalOutput': function (roomId, spec) {
        var room = rooms[roomId];
        if (!room) {
            log.warn('room not found:', roomId);
            return;
        }

        var recorderId = spec.id;
        room.controller.unsubscribe(room.id, recorderId);

        log.info('Recorder has been deleted since', spec.message);

        sendMsgToRoom(room, 'remove_recorder', {id: recorderId});
        for (var i in room.streams) {
            if (room.streams.hasOwnProperty(i)) {
                room.streams[i].removeRecorder(recorderId);
            }
        }
    },
    'vad': function (roomId, spec) {
        var room = rooms[roomId];
        if (!room) {
            log.warn('room not found:', roomId);
            return;
        }

        var activeTerminal = spec.active_terminal;
        log.info('vad:', activeTerminal);
        room.controller.setPrimary(activeTerminal);
    }
};

var formatDate = function(date, format) {
    var dateTime = {
        'M+': date.getMonth() + 1,
        'd+': date.getDate(),
        'h+': date.getHours(),
        'm+': date.getMinutes(),
        's+': date.getSeconds(),
        'q+': Math.floor((date.getMonth() + 3) / 3),
        'S+': date.getMilliseconds()
    };

    if (/(y+)/i.test(format)) {
        format = format.replace(RegExp.$1, (date.getFullYear() + '').substr(4 - RegExp.$1.length));
    }

    for (var k in dateTime) {
        if (new RegExp('(' + k + ')').test(format)) {
            format = format.replace(RegExp.$1, RegExp.$1.length == 1 ?
                dateTime[k] : ('00' + dateTime[k]).substr(('' + dateTime[k]).length));
        }
    }

    return format;
};

// var privateRegexp;
var ip_address;
(function getPublicIP () {
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

    if (GLOBAL.config.controller.ip_address === '' || GLOBAL.config.controller.ip_address === undefined){
        ip_address = addresses[0];
    } else {
        ip_address = GLOBAL.config.controller.ip_address;
    }
})();

var joinCluster = function (on_ok) {
    var joinOK = function (id) {
        var onIOReady = function () {
            amqper.callRpc('nuve', 'getKey', id, {
                callback: function (key) {
                    if (key === 'error' || key === 'timeout') {
                        log.info('Failed to get token key.');
                        worker && worker.quit();
                        return process.exit();
                    }
                    tokenKey = key;
                    on_ok(id);
                }
            });
        };

        myId = id;
        myState = 2;

        var enableSSL = GLOBAL.config.controller.ssl;
        var server;
        if (enableSSL === true) {
            log.info('SSL enabled!');
            var cipher = require('./cipher');
            var keystore = path.resolve(path.dirname(GLOBAL.config.controller.keystorePath), '.woogeen.keystore');
            cipher.unlock(cipher.k, keystore, function cb (err, passphrase) {
                if (!err) {
                    try {
                        server = require('https').createServer({
                            pfx: require('fs').readFileSync(GLOBAL.config.controller.keystorePath),
                            passphrase: passphrase
                        }).listen(GLOBAL.config.controller.port);
                    } catch (e) {
                        err = e;
                    }
                }
                if (err) {
                   log.warn('Failed to setup secured server:', err);
                    worker && worker.quit();
                    return process.exit();
                }
                io = require('socket.io').listen(server);
                onIOReady();
            });
        } else {
            server = require('http').createServer().listen(GLOBAL.config.controller.port);
            io = require('socket.io').listen(server);
            onIOReady();
        }
    };

    var joinFailed = function (reason) {
        log.error('portal join cluster failed. reason:', reason);
        worker && worker.quit();
    };

    var loss = function () {
        log.info('portal lost.');
    };

    var recovery = function () {
        log.info('portal recovered.');
    };

    var spec = {amqper: amqper,
                purpose: 'portal',
                clusterName: GLOBAL.config.cluster.name,
                joinRery: GLOBAL.config.cluster.join_retry,
                joinPeriod: GLOBAL.config.cluster.join_interval,
                recoveryPeriod: GLOBAL.config.cluster.recover_interval,
                keepAlivePeriod: GLOBAL.config.cluster.keep_alive_interval,
                info: {ip: ip_address,
                       hostname: GLOBAL.config.controller.hostname,
                       port: GLOBAL.config.controller.port,
                       ssl: GLOBAL.config.controller.ssl,
                       state: 2,
                       max_load: GLOBAL.config.cluster.max_load
                      },
                onJoinOK: joinOK,
                onJoinFailed: joinFailed,
                onLoss: loss,
                onRecovery: recovery,
                loadCollection: {period: GLOBAL.config.cluster.report_load_interval,
                                 item: {name: 'cpu'}}
               };

    worker = clusterWorker(spec);
};

//*******************************************************************
//       When adding or removing rooms we use an algorithm to check the state
//       If there is a state change we send a message to cloudHandler
//
//       States:
//            0: Not available
//            1: Warning
//            2: Available
//*******************************************************************
var updateMyState = function () {
    var nRooms = 0, newState, i, info;

    for (i in rooms) {
        if (rooms.hasOwnProperty(i)) {
            nRooms += 1;
        }
    }

    if (nRooms < WARNING_N_ROOMS) {
        newState = 2;
    } else if (nRooms >= LIMIT_N_ROOMS) {
        newState = 0;
    } else {
        newState = 1;
    }

    if (newState === myState) {
        return;
    }

    myState = newState;

    worker && worker.reportState(myState);
};

function safeCall () {
    var callback = arguments[0];
    if (typeof callback === 'function') {
        var args = Array.prototype.slice.call(arguments, 1);
        callback.apply(null, args);
    }
}

//FIXME: to keep compatible to previous MCU, should be removed later.
var resolutionName2Value = {
    'cif': {width: 352, height: 288},
    'vga': {width: 640, height: 480},
    'svga': {width: 800, height: 600},
    'xga': {width: 1024, height: 768},
    'r640x360': {width: 640, height: 360},
    'hd720p': {width: 1280, height: 720},
    'sif': {width: 320, height: 240},
    'hvga': {width: 480, height: 320},
    'r480x360': {width: 480, height: 360},
    'qcif': {width: 176, height: 144},
    'r192x144': {width: 192, height: 144},
    'hd1080p': {width: 1920, height: 1080},
    'uhd_4k': {width: 3840, height: 2160},
    'r360x360': {width: 360, height: 360},
    'r480x480': {width: 480, height: 480},
    'r720x720': {width: 720, height: 720}
};
var resolutionValue2Name = {
    'r352x288': 'cif',
    'r640x480': 'vga',
    'r800x600': 'svga',
    'r1024x768': 'xga',
    'r640x360': 'r640x360',
    'r1280x720': 'hd720p',
    'r320x240': 'sif',
    'r480x320': 'hvga',
    'r480x360': 'r480x360',
    'r176x144': 'qcif',
    'r192x144': 'r192x144',
    'r1920x1080': 'hd1080p',
    'r3840x2160': 'uhd_4k',
    'r360x360': 'r360x360',
    'r480x480': 'r480x480',
    'r720x720': 'r720x720'
};

var listen = function () {
    log.info('server on');

    io.sockets.on('connection', function (socket) {
        log.info('Socket connect', socket.id);

        function sendMsgToOthersInRoom (room, type, arg) {
            room && room.sockets && room.sockets.map(function (sock) {
                if (sock.id !== socket.id) {
                    log.info('Sending message to', sock.id, 'in room ', room.id);
                    sock.emit(type, arg);
                }
            });
        }

        // Gets 'token' messages on the socket. Checks the signature and ask nuve if it is valid.
        // Then registers it in the room and callback to the client.
        socket.on('token', function (token, callback) {

            log.debug('New token', token);

            var tokenDB, user, streamList = [], index;

            if (checkSignature(token)) {
                amqper.callRpc('nuve', 'deleteToken', token.tokenId, {callback: function (resp) {
                    if (resp === 'error') {
                        log.info('Token does not exist');
                        safeCall(callback, 'error', 'Token does not exist');
                        socket.disconnect();

                    } else if (resp === 'timeout') {
                        log.warn('Nuve does not respond');
                        safeCall(callback, 'error', 'Nuve does not respond');
                        socket.disconnect();

                    } else if (token.host === resp.host) {
                        if (socket.disconnected) {
                            log.warn('Client already disconnected');
                            return;
                        }
                        tokenDB = resp;
                        var validateTokenOK = function () {
                            if (socket.disconnected) {
                                log.warn('Client already disconnected');
                                return;
                            }
                            user = {name: tokenDB.userName, role: tokenDB.role, id: socket.id};
                            socket.user = user;
                            var permissions = GLOBAL.config.controller.roles[tokenDB.role] || [];
                            socket.user.permissions = {};
                            for (var right in permissions) {
                                socket.user.permissions[right] = permissions[right];
                            }
                            socket.room = rooms[tokenDB.room];
                            socket.streams = []; //[list of streamIds]
                            socket.state = 'sleeping';

                            log.debug('OK, Valid token');

                            if (!tokenDB.p2p && GLOBAL.config.controller.report.session_events) {
                                var timeStamp = new Date();
                                amqper.broadcast('event', {room: tokenDB.room, user: socket.id, type: 'user_connection', timestamp:timeStamp.getTime()});
                            }

                            // Add client socket to room socket list and start to receive room events
                            socket.room.sockets.push(socket);

                            for (index in socket.room.streams) {
                                if (socket.room.streams.hasOwnProperty(index) && (socket.room.streams[index].getStatus() === 'ready')) {
                                    streamList.push(socket.room.streams[index].getPublicStream());
                                }
                            }

                            safeCall(callback, 'success', {streams: streamList,
                                                id: socket.room.id,
                                                clientId: socket.id,
                                                users: socket.room.sockets.map(function (sock) {
                                                    return sock.user;
                                                }),
                                                p2p: socket.room.p2p,
                                                defaultVideoBW: GLOBAL.config.controller.defaultVideoBW,
                                                maxVideoBW: GLOBAL.config.controller.maxVideoBW,
                                                stunServerUrl: GLOBAL.config.controller.stunServerUrl,
                                                turnServer: GLOBAL.config.controller.turnServer
                                                });
                            sendMsgToOthersInRoom(socket.room, 'user_join', {user: user});
                        };

                        if (rooms[tokenDB.room] === undefined) {
                            if (myState === 0) {
                                worker && worker.rejectTask(tokenDB.room);
                                var errMsg = 'Erizo: out of capacity';
                                log.warn(errMsg);
                                safeCall(callback, 'error', errMsg);
                                socket.disconnect();
                            }
                            var initRoom = function (roomID, on_ok, on_error) {
                                var room = {};
                                room.id = roomID;
                                room.sockets = [];
                                room.streams = {}; //streamId: Stream
                                rooms[roomID] = room;
                                if (tokenDB.p2p) {
                                    log.debug('Token of p2p room');
                                    room.p2p = true;
                                    on_ok();
                                } else {
                                    amqper.callRpc('nuve', 'getRoomConfig', room.id, {callback: function (resp) {
                                        if (resp === 'error') {
                                            log.error('Room does not exist');
                                            delete rooms[roomID];
                                            on_error();
                                        } else if (resp === 'timeout') {
                                            log.error('Nuve does not respond to "getRoomConfig"');
                                            delete rooms[roomID];
                                            on_error();
                                        } else {
                                            room.p2p = false;
                                            room.config = resp;
                                            if (room.config.userLimit === 0) {
                                                log.error('Room', roomID, 'disabled');
                                                delete rooms[roomID];
                                                return on_error();
                                            }
                                            room.controller = controller.RoomController(
                                                {cluster: GLOBAL.config.cluster.name || 'woogeen-cluster',
                                                 amqper: amqper,
                                                 room: roomID,
                                                 config: resp,
                                                 observer: myId},
                                                function (resolutions) {
                                                    room.enableMixing = resp.enableMixing;
                                                    on_ok();
                                                    if (resp.enableMixing) {
                                                        //var st = new ST.Stream({id: roomID, socket: '', audio: true, video: {category: 'mix'}, data: true, from: ''});
                                                        var st = new ST.Stream({id: roomID, socket: '', audio: true, video: {device: 'mcu', resolutions: resolutions.map(function (n) {return resolutionName2Value[n];})/*FIXME: to keep compatible to previous MCU, should be removed later.*/}, from: '', attributes: null});
                                                        room.streams[roomID] = st;
                                                        room.mixer = roomID;
                                                        log.debug('Mixed stream info:', st.getPublicStream(), 'resolutions:', st.getPublicStream().video.resolutions);
                                                        sendMsgToRoom(room, 'add_stream', st.getPublicStream());
                                                    }
                                                },
                                                function (reason) {
                                                    log.error('RoomController init failed.', reason);
                                                    delete rooms[roomID];
                                                    on_error();
                                                });
                                        }
                                    }});
                                }
                            };

                            initRoom(tokenDB.room, function () {
                                worker && worker.addTask(tokenDB.room);
                                updateMyState();
                                validateTokenOK();
                            }, function () {
                                log.warn('initRoom failed.');
                                safeCall(callback, 'error', 'initRoom failed.');
                                socket.disconnect();
                            });
                        } else {
                            if (rooms[tokenDB.room].config) {
                                if (rooms[tokenDB.room].config.userLimit > 0 &&
                                    rooms[tokenDB.room].sockets.length >= rooms[tokenDB.room].config.userLimit) {
                                    safeCall(callback, 'error', 'Room is full');
                                    return socket.disconnect();
                                }
                            }
                            validateTokenOK();
                        }
                    } else {
                        log.warn('Invalid host');
                        safeCall(callback, 'error', 'Invalid host');
                        socket.disconnect();
                    }
                }});

            } else {
                log.warn('Authentication error');
                safeCall(callback, 'error', 'Authentication error');
                socket.disconnect();
            }
        });

        // Gets 'customMessage' messages on the socket.
        socket.on('customMessage', function (msg, callback) {
            switch (msg.type) {
            case 'login': // reserved
                break;
            case 'control': // stream media manipulation
                if (typeof msg.payload === 'object' && msg.payload !== null) {
                    var action = msg.payload.action;
                    if (/^((audio)|(video))-((in)|(out))-((on)|(off))$/.test(action)) {
                        var cmdOpts = action.split('-'),
                            streamId = msg.payload.streamId/*FIXME: to be modified to msg.payload.connectionId along with client side.*/ + '',
                            connectionId = socket.room.streams[streamId] && socket.room.streams[streamId].getOwner() === socket.id ? streamId : socket.id + '-' + streamId;
                        socket.room.controller.onTrackControl(
                            'webrtc#'+socket.id/*FIXME: hard code terminalID*/,
                            connectionId,
                            cmdOpts[0],
                            cmdOpts[1],
                            cmdOpts[2],
                            function () {
                                safeCall(callback, 'success');
                                if (cmdOpts[1] === 'out') {
                                    var event = cmdOpts[0] === 'audio' ? 'Audio' : 'Video';
                                    event += cmdOpts[2] === 'on' ? 'Enabled' : 'Disabled';
                                    log.debug(socket.room.id, streamId, event);
                                    exports.handleEventReport('updateStream', socket.room.id, {
                                        event: event,
                                        id: streamId,
                                        data: null
                                    });
                                }
                            }, function (error_reason) {
                                safeCall(callback, 'error', error_reason);
                            }
                        );
                    }
                } else {
                    safeCall(callback, 'error', 'Invalid control message.');
                }
                break;
            case 'data':
                if (socket.user === undefined || !socket.user.permissions[Permission.PUBLISH]) {
                    return safeCall(callback, 'error', 'unauthorized');
                }
                if (socket.user.permissions[Permission.PUBLISH] !== true) {
                    var permissions = socket.user.permissions[Permission.PUBLISH];
                    if (permissions.data === false)
                        return safeCall(callback, 'error', 'unauthorized');
                }

                var receiver = msg.receiver;
                var data = {
                    data: msg.data,
                    from: socket.id,
                    to: receiver
                };
                if (receiver === 'all') {
                    sendMsgToRoom(socket.room, 'custom_message', data);
                    return safeCall(callback, 'success');
                } else {
                    for (var k in socket.room.sockets) {
                        if (socket.room.sockets[k].id === receiver) {
                            try {
                                socket.room.sockets[k].emit('custom_message', data);
                                safeCall(callback, 'success');
                            } catch (err) {
                                safeCall(callback, 'error', err);
                            }
                            return;
                        }
                    }
                    return safeCall(callback, 'error', 'invalid receiver');
                }
                break;
            default: // do nothing
                return safeCall(callback, 'error', 'invalid message type');
            }
        });

        //Gets 'updateStreamAttributes' messages on the socket in order to update attributes from the stream.
        socket.on('updateStreamAttributes', function (msg) {
            if (socket.room.streams[msg.id] === undefined){
                log.warn('Trying to update atributes from a non-initialized stream ', msg);
                return;
            }
            socket.room.streams[msg.id].setAttributes(msg.attrs);
            socket.room.streams[msg.id].getDataSubscribers().map(function (sockid) {
                log.info('Sending new attributes to', sockid, 'in stream ', msg.id);
                io.sockets.to(sockid).emit('onUpdateAttributeStream', msg);
            });
        });

        // Gets 'addToMixer' messages on the socket in order to put the published stream into the mix stream.
        socket.on('addToMixer', function (streamId, callback) {
            if (socket.user === undefined || !socket.user.permissions[Permission.PUBLISH]) {
                return safeCall(callback, 'error', 'unauthorized');
            }
            if (socket.room.enableMixing !== true || !socket.room.mixer) {
                return safeCall(callback, 'error', 'no mix stream found');
            }
            var stream = socket.room.streams[streamId];
            if (!stream) {
                return safeCall(callback, 'error', 'stream does not exist');
            }
            if (socket.room.p2p) {
                return safeCall(callback, 'error', 'p2p room does not support this action');
            }
            socket.room.controller.mix(streamId, function () {
                return safeCall(callback, 'success');
            }, function (err) {
                return safeCall(callback, 'error', err);
            });
        });

        // Gets 'removeFromMixer' messages on the socket in order to remove the published stream from the mix stream.
        socket.on('removeFromMixer', function (streamId, callback) {
            if (socket.user === undefined || !socket.user.permissions[Permission.PUBLISH]) {
                return safeCall(callback, 'error', 'unauthorized');
            }
            if (socket.room.enableMixing !== true || !socket.room.mixer) {
                return safeCall(callback, 'error', 'no mix stream found');
            }
            var stream = socket.room.streams[streamId];
            if (!stream) {
                return safeCall(callback, 'error', 'stream does not exist');
            }
            if (socket.room.p2p) {
                return safeCall(callback, 'error', 'p2p room does not support this action');
            }
            socket.room.controller.unmix(streamId, function () {
                return safeCall(callback, 'success');
            }, function (err) {
                return safeCall(callback, 'error', err);
            });
        });

        socket.on('signaling_message', function (msg) {
            if (socket.room.p2p) {
                io.sockets.to(msg.peerSocket).emit('signaling_message_peer', {streamId: msg.streamId, peerSocket: socket.id, msg: msg.msg});
            } else {
                var st = socket.room.streams[msg.streamId];
                if (st) {
                    log.debug('signaling_message, stream:', msg.streamId, 'owner:', st.getPublicStream().from, 'self:', socket.id);
                    /*FIXME: to modify msg.streamId to msg.connectionId along with client side.*/
                    var connectionId = st.getOwner() === socket.id ? msg.streamId : socket.id + '-' + msg.streamId;
                    socket.room.controller.onConnectionSignalling('webrtc#'+socket.id/*FIXME: hard code terminalID*/, connectionId, msg.msg);
                } else {
                   log.info('signaling_message, stream['+msg.streamId+'] does not exist');
                }
            }
        });

        //Gets 'publish' messages on the socket in order to add new stream to the room.
        socket.on('publish', function (options, sdp, callback) {
            var id, st;

            // format options
            // audio should be a boolean value to indicate whether audio is enabled or not.
            if (options.audio === undefined) {
              options.audio = true;
            } else {
              options.audio = !!options.audio;
            }
            // video should be an object containing video tracks detailed information if it is enabled.
            if (options.video === undefined || (typeof options.video !== 'object' && !!options.video)) {
              options.video = {};
            }
            if (typeof options.video === 'object' && typeof options.video.device !== 'string') {
              options.video.device = 'unknown';
            }

            if (socket.user === undefined || !socket.user.permissions[Permission.PUBLISH]) {
                return safeCall(callback, 'error', 'unauthorized');
            }
            if (socket.user.permissions[Permission.PUBLISH] !== true) {
                var permissions = socket.user.permissions[Permission.PUBLISH];
                for (var right in permissions) {
                    if ((options[right] === true) && (permissions[right] === false))
                        return safeCall(callback, 'error', 'unauthorized');
                }
            }
            if (options.state === 'url' || options.state === 'recording') {
                id = socket.id;
                var url = sdp,
                    stream_type = 'rtsp';
                if (options.state === 'recording') {
                    url = sdp + '.mkv';
                    stream_type = 'file';
                }
                if (!options.audio && !options.video) {
                    return safeCall(callback, 'error', 'no media input is specified to publish');
                }

                socket.room.controller.publish(
                    stream_type+'#'+socket.id/*FIXME: hard code terminalID*/,
                    id,
                    stream_type,
                    {url: url,
                     has_audio: options.audio,
                     has_video: options.video,
                     transport: options.transport,
                     buffer_size: options.bufferSize},
                    socket.room.enableMixing && !options.unmix,
                    function (response) {
                        if (response.type === 'ready') {
                            // Double check if this socket is still in the room.
                            // It can be removed from the room if the socket is disconnected
                            // before the publish succeeds.
                            var index = socket.room.sockets.indexOf(socket);
                            if (index === -1) {
                                socket.room.controller.unpublish(id);
                                return;
                            }

                            st = new ST.Stream({id: id, socket: socket.id, audio: response.audio_codecs.length > 0 ? options.audio : false, video: response.video_codecs.length > 0 ? options.video : false, attributes: options.attributes, from: url});
                            socket.streams.push(id);
                            socket.room.streams[id] = st;
                            sendMsgToRoom(socket.room, 'add_stream', st.getPublicStream());
                            safeCall(callback, 'success', id);
                        } else if (response.type !== 'initializing') {
                            safeCall(callback, response);
                        }
                    }
                );
            } else if (options.state === 'erizo') {
                log.info('New publisher');
                id = socket.id;
                var hasScreen = false;
                var unmix = options.unmix;
                if (options.video && options.video.device === 'screen') {
                    hasScreen = true;
                    id = id.slice(0, -8) + '_SCREEN_';
                    unmix = true;
                }

                if (socket.streams.indexOf(id) !== -1) {
                    return safeCall(callback, 'error', 'already published');
                }

                if (GLOBAL.config.controller.report.session_events) {
                    var timeStamp = new Date();
                    amqper.broadcast('event', [{room: socket.room.id, user: socket.id, name: socket.user.name, type: 'publish', stream: id, timestamp: timeStamp.getTime()}]);
                }

                socket.room.controller.publish(
                    'webrtc#'+socket.id/*FIXME: hard code terminalID*/,
                    id,
                    'webrtc',
                    {has_audio: options.audio, has_video: options.video},
                    !unmix,
                    function (signMess) {
                        if (typeof signMess !== 'object' || signMess === null) {
                            safeCall(callback, 'error', 'unexpected error');
                            return;
                        }
                        switch (signMess.type) {
                        case 'initializing':
                            safeCall(callback, 'initializing', id);
                            st = new ST.Stream({id: id, audio: options.audio, video: options.video, attributes: options.attributes, from: socket.id, status: 'initializing'});
                            socket.room.streams[id] = st;
                            return;
                        case 'failed': // If the connection failed we remove the stream
                            log.info(signMess.reason, 'removing', id);
                            socket.emit('connection_failed',{});

                            socket.state = 'sleeping';
                            if (!socket.room.p2p) {
                                socket.room.controller.unpublish(id);
                                if (GLOBAL.config.controller.report.session_events) {
                                    var timeStamp = new Date();
                                    amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'failed', stream: id, sdp: signMess.sdp, timestamp: timeStamp.getTime()});
                                }
                            }

                            var index = socket.streams.indexOf(id);
                            if (index !== -1) {
                                socket.streams.splice(index, 1);
                            }
                            delete socket.room.streams[id];
                            safeCall(callback, 'error', signMess.reason);
                            return;
                        case 'ready':
                            // Double check if this socket is still in the room.
                            // It can be removed from the room if the socket is disconnected
                            // before the publish succeeds.
                            if (socket.room.sockets.indexOf(socket) === -1) {
                                socket.room.controller.unpublish(id);
                                return;
                            }

                            st.setStatus('ready');
                            sendMsgToRoom(socket.room, 'add_stream', st.getPublicStream());
                            break;
                        default:
                            break;
                        }
                        socket.emit('signaling_message_erizo', {mess: signMess, streamId: id});
                    }
                );
                socket.streams.push(id);
            } else {
                id = Math.random() * 1000000000000000000;
                st = new ST.Stream({id: id, socket: socket.id, audio: options.audio, video: options.video, attributes: options.attributes, from: socket.id});
                socket.streams.push(id);
                socket.room.streams[id] = st;
                safeCall(callback, '', id);
                sendMsgToRoom(socket.room, 'add_stream', st.getPublicStream());
            }
        });

        //Gets 'subscribe' messages on the socket in order to add new subscriber to a determined stream (options.streamId).
        socket.on('subscribe', function (options, sdp, callback) {
            if (socket.user === undefined || !socket.user.permissions[Permission.SUBSCRIBE]) {
                return safeCall(callback, 'error', 'unauthorized');
            }
            if (socket.user.permissions[Permission.SUBSCRIBE] !== true) {
                var permissions = socket.user.permissions[Permission.SUBSCRIBE];
                for (var right in permissions) {
                    if ((options[right] === true) && (permissions[right] === false))
                        return safeCall(callback, 'error', 'unauthorized');
                }
            }

            var stream = socket.room.streams[options.streamId];

            if (stream === undefined) {
                return safeCall(callback, 'error', 'stream does not exist');
            }

            if (stream.hasData() && options.data !== false) {
                stream.addDataSubscriber(socket.id);
            }

            if (stream.hasAudio() || stream.hasVideo()) {

                if (socket.room.p2p) {
                    var s = stream.getSocket();
                    io.sockets.to(s).emit('publish_me', {streamId: options.streamId, peerSocket: socket.id});

                } else {
                    if (GLOBAL.config.controller.report.session_events) {
                        var timeStamp = new Date();
                        amqper.broadcast('event', {room: socket.room.id, user: socket.id, name: socket.user.name, type: 'subscribe', stream: options.streamId, timestamp: timeStamp.getTime()});
                    }
                    log.debug('Subscribe, options.video:', options.video);
                    if (typeof options.video === 'object' && options.video !== null && options.video.resolution && options.video.resolution.width) {
                        log.debug('Subscribe a multistreaming stream(its resolutions:', stream.video && stream.video.resolutions, ') with resolution:', options.video.resolution);
                        if (!stream.hasResolution(options.video.resolution)) {
                            options.video = true;
                        } else {
                            options.video.resolution = resolutionValue2Name['r'+options.video.resolution.width+'x'+options.video.resolution.height];//FIXME: to keep compatible to previous MCU, should be removed later.
                        }
                    }

                    socket.room.controller.subscribe(
                        'webrtc#'+socket.id/*FIXME: hard code terminalID*/,
                        socket.id + '-' + options.streamId,
                        'webrtc',
                        options.streamId,
                        {require_audio: options.audio === undefined ? stream.hasAudio() : !!options.audio,
                         require_video: options.video === undefined ? stream.hasVideo() : !!options.video,
                         video_resolution: (options.video && options.video.resolution) ? options.video.resolution : undefined},
                        function (signMess) {
                            if (typeof signMess !== 'object' || signMess === null) {
                                safeCall(callback, 'error', 'unexpected error');
                                return;
                            }
                            switch (signMess.type) {
                            case 'initializing':
                                log.info('Initializing subscriber');
                                safeCall(callback, 'initializing');
                                return;
                            case 'candidate':
                                // signMess.candidate = signMess.candidate.replace(privateRegexp, ip_address);
                                break;
                            case 'failed':
                                safeCall(callback, 'error', signMess.reason);
                                return;
                            default:
                                break;
                            }
                            socket.emit('signaling_message_erizo', {mess: signMess, peerId: options.streamId});
                        }
                    );
                    log.info('Subscriber added');
                    // safeCall(callback, '');
                }
            } else {
                safeCall(callback, '');
            }
        });

        //Gets 'startRecorder' messages
        socket.on('startRecorder', function (options, callback) {
            if (typeof options === 'function') {
                callback = options;
                options = {};
            } else if (typeof options !== 'object' || options === null) {
                options = {};
            }

            if (socket.user === undefined || !socket.user.permissions[Permission.RECORD]) {
                return safeCall(callback, 'error', 'unauthorized');
            }

            // Check the streams to be recorded
            var videoRequired = true;
            var audioRequired = true;
            var videoStreamId = options.videoStreamId || undefined;
            var audioStreamId = options.audioStreamId || undefined;
            if (videoStreamId === undefined && audioStreamId === undefined) {
                videoStreamId = socket.room.mixer;
                audioStreamId = socket.room.mixer;
            } else if (videoStreamId === undefined) {
                videoRequired = false;
            } else if (audioStreamId === undefined){
                audioRequired = false;
            }

            var videoStream = socket.room.streams[videoStreamId];
            var audioStream = socket.room.streams[audioStreamId];
            if (videoRequired && videoStream === undefined) {
                return safeCall(callback, 'error', 'Target video record stream does not exist.');
            }

            if (audioRequired && audioStream === undefined) {
                return safeCall(callback, 'error', 'Target audio record stream does not exist.');
            }

            if (videoRequired && !videoStream.hasVideo()) {
                return safeCall(callback, 'error', 'No video data from the video stream.');
            }

            if (audioRequired && !audioStream.hasAudio()) {
                return safeCall(callback, 'error', 'No audio data from the audio stream.');
            }

            if (options.videoCodec && typeof options.videoCodec !== 'string') {
                return safeCall(callback, 'error', 'Invalid video codec.');
            }

            if (options.audioCodec && typeof options.audioCodec !== 'string') {
                return safeCall(callback, 'error', 'Invalid audio codec.');
            }

            var timeStamp = new Date();
            var recorderId = options.recorderId || formatDate(timeStamp, 'yyyyMMddhhmmssSS');
            var specifiedPath = options.path;
            var filename = 'room' + socket.room.id + '_' + recorderId + '.mkv';
            var interval = (options.interval && options.interval > 0) ? options.interval : -1;
            var preferredVideoCodec = options.videoCodec || 'vp8';
            var preferredAudioCodec = options.audioCodec || 'pcmu';

            if (preferredAudioCodec.toLowerCase() === 'opus') {
                // Translate the incoming codec to internal codec by default
                preferredAudioCodec = 'opus_48000_2';
            }

            var isContinuous = false;
            for (var i in socket.room.streams) {
                if (socket.room.streams.hasOwnProperty(i) && socket.room.streams[i].removeRecorder(recorderId)) {
                    isContinuous = true;
                }
            }

            // Make sure the recording context clean for this 'startRecorder' subscription
            socket.room.controller.unsubscribe('file#'+socket.room.id/*FIXME: hard code terminalID*/, recorderId, true);

            socket.room.controller.subscribeSelectively(
                'file#'+socket.room.id/*FIXME: hard code terminalID*/,
                recorderId,
                'file',
                audioStreamId,
                videoStreamId,
                {require_audio: audioRequired,
                 audio_codec: preferredAudioCodec,
                 require_video: videoRequired,
                 video_codec: preferredVideoCodec,
                 path: specifiedPath,
                 filename: filename,
                 interval: interval,
                 observer: myId,
                 room_id: socket.room.id},
                function (response) {
                    if (response.type === 'ready') {
                        log.info('Media recording to ', filename);

                        if (videoStream) {
                            videoStream.addRecorder(recorderId);
                        }

                        if (audioStream) {
                            audioStream.addRecorder(recorderId);
                        }

                        if (isContinuous) {
                            sendMsgToRoom(socket.room, 'reuse_recorder', {id: recorderId});
                        } else {
                            sendMsgToRoom(socket.room, 'add_recorder', {id: recorderId});
                        }

                        safeCall(callback, 'success', {
                            recorderId : recorderId,
                            host: ip_address,
                            path: specifiedPath ? path.join(specifiedPath, filename) : filename
                        });
                    } else if (response.type === 'failed') {
                        safeCall(callback, 'error', response.reason);
                    }
                }
            );
        });

        //Gets 'stopRecorder' messages
        socket.on('stopRecorder', function (options, callback) {
            if (typeof options === 'function') {
                callback = options;
                options = {};
            } else if (typeof options !== 'object' || options === null) {
                options = {};
            }

            if (socket.user === undefined || !socket.user.permissions[Permission.RECORD]) {
                return safeCall(callback, 'error', 'unauthorized');
            }

            var recorderExisted = false;
            if (options.recorderId) {
                for (var i in socket.room.streams) {
                    if (socket.room.streams.hasOwnProperty(i) && socket.room.streams[i].removeRecorder(options.recorderId)) {
                        recorderExisted = true;
                    }
                }
            }

            // Stop recorder
            if (recorderExisted) {
                socket.room.controller.unsubscribe('file#'+socket.room.id/*FIXME: hard code terminalID*/, options.recorderId);

                log.info('Recorder stopped: ', options.recorderId);

                sendMsgToRoom(socket.room, 'remove_recorder', {id: options.recorderId});

                safeCall(callback, 'success', {
                            host: ip_address,
                            recorderId: options.recorderId
                        });
            } else {
                safeCall(callback, 'error', 'No recorder to stop.');
            }
        });

        socket.on('getRegion', function (options, callback) {
            if (typeof options === 'function') {
                callback = options;
                options = {};
            } else if (typeof options !== 'object' || options === null) {
                options = {};
            }

            if (options.id && socket.room.mixer) {
                socket.room.controller.getRegion(options.id, function (region) {
                    log.debug('Region for ', options.id, ': ', region);
                    safeCall(callback, 'success', {region: region});
                },function (error_reason) {
                    safeCall(callback, 'error', 'Error in getRegion: ' + error_reason);
                });
            } else {
                safeCall(callback, 'error', 'Invalid participant id or mixer not available.');
            }
        });

        socket.on('setRegion', function (options, callback) {
            if (typeof options === 'function') {
                callback = options;
                options = {};
            } else if (typeof options !== 'object' || options === null) {
                options = {};
            }

            if (options.id && options.region && socket.room.mixer) {
                socket.room.controller.setRegion(options.id, options.region, function () {
                    safeCall(callback, 'success');
                },function (error_reason) {
                    safeCall(callback, 'error', error_reason);
                });
            } else {
                safeCall(callback, 'error', 'Invalid participant/region id or mixer not available.');
            }
        });

        socket.on('setVideoBitrate', function (options, callback) {
            if (typeof options === 'function') {
                callback = options;
                options = {};
            } else if (typeof options !== 'object' || options === null) {
                options = {};
            }

            // TODO: Currently setVideoBitrate only works when the mixer is available.
            // We need to add the support in pure forwarding mode later.
            if (options.id && options.bitrate && socket.room.mixer) {
                socket.room.controller.setVideoBitrate(options.id, options.bitrate, function () {
                    safeCall(callback, 'success');
                }, function (error_reason) {
                    safeCall(callback, 'error', error_reason);
                });
            } else {
                safeCall(callback, 'error', 'Invalid participant id/bitrate or mixer not available.');
            }
        });

        //Gets 'unpublish' messages on the socket in order to remove a stream from the room.
        socket.on('unpublish', function (streamId, callback) {
            if (socket.user === undefined || !socket.user.permissions[Permission.PUBLISH]) {
                return safeCall(callback, 'error', 'unauthorized');
            }

            var index = socket.streams.indexOf(streamId);
            // Stream has been already deleted or it does not exist
            if (index === -1) {
                return safeCall(callback, 'error', 'stream does not exist');
            }

            socket.state = 'sleeping';
            if (!socket.room.p2p) {
                socket.room.controller.unpublish(streamId);
                if (GLOBAL.config.controller.report.session_events) {
                    var timeStamp = new Date();
                    amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'unpublish', stream: streamId, timestamp: timeStamp.getTime()});
                }
            }

            socket.streams.splice(index, 1);
            if (socket.room.streams[streamId]) {
                sendMsgToRoom(socket.room, 'remove_stream', {id: streamId});
                delete socket.room.streams[streamId];
            }
            safeCall(callback, 'success');
        });

        //Gets 'unsubscribe' messages on the socket in order to remove a subscriber from a determined stream (to).
        socket.on('unsubscribe', function (to, callback) {
            if (!socket.user.permissions[Permission.SUBSCRIBE]) {
                return safeCall(callback, 'error', 'unauthorized');
            }
            if (socket.room.streams[to] === undefined) {
                return safeCall(callback, 'error', 'stream does not exist');
            }

            socket.room.streams[to].removeDataSubscriber(socket.id);

            if (socket.room.streams[to].hasAudio() || socket.room.streams[to].hasVideo()) {
                if (!socket.room.p2p) {
                    socket.room.controller.unsubscribe('webrtc#'+socket.id/*FIXME: hard code terminalID*/, socket.id + '-' + to);
                    if (GLOBAL.config.controller.report.session_events) {
                        var timeStamp = new Date();
                        amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'unsubscribe', stream: to, timestamp:timeStamp.getTime()});
                    }
                }
            }
            safeCall(callback, 'success');
        });

        //When a client leaves the room erizoController removes its streams from the room if exists.
        socket.on('disconnect', function () {
            var i, index, id;

            log.info('Socket disconnect ', socket.id);

            if (socket.room !== undefined) {

                for (i in socket.room.streams) {
                    if (socket.room.streams.hasOwnProperty(i)) {
                        socket.room.streams[i].removeDataSubscriber(socket.id);
                    }
                }

                index = socket.room.sockets.indexOf(socket);
                if (index !== -1) {
                    socket.room.sockets.splice(index, 1);
                }

                if (socket.room.controller) {
                    socket.room.controller.unsubscribeAll('webrtc#'+socket.id/*FIXME: hard code terminalID*/);
                }

                for (i in socket.streams) {
                    if (socket.streams.hasOwnProperty(i)) {
                        id = socket.streams[i];
                        if (!socket.room.p2p) {
                            socket.room.controller.unpublish(id);
                            if (GLOBAL.config.controller.report.session_events) {
                                amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'unpublish', stream: id, timestamp: (new Date()).getTime()});
                            }
                        }

                        if (socket.room.streams[id]) {
                            sendMsgToRoom(socket.room, 'remove_stream', {id: id});
                            delete socket.room.streams[id];
                        }
                    }
                }
            }

            if (socket.room !== undefined && !socket.room.p2p && GLOBAL.config.controller.report.session_events) {
                amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'user_disconnection', timestamp: (new Date()).getTime()});
            }

            if (socket.room !== undefined && socket.room.sockets.length === 0) {
                log.info('Empty room ', socket.room.id, '. Deleting it');
                if (!socket.room.p2p) {
                    if (socket.room.mixer !== undefined) {
                        if (socket.room.controller && typeof socket.room.controller.destroy === 'function') {
                            socket.room.controller.destroy();
                        }
                        if (socket.room.streams[socket.room.mixer]) {
                            delete socket.room.streams[socket.room.mixer];
                        }
                        socket.room.mixer = undefined;
                    }
                }
                delete rooms[socket.room.id];
                worker && worker.removeTask(socket.room.id);
                updateMyState();
            } else if (socket.room !== undefined) {
                sendMsgToOthersInRoom(socket.room, 'user_leave', {user: socket.user});
            }
        });
    });
};

/*
 *Gets a list of users in a determined room.
 */
exports.getUsersInRoom = function (room, callback) {
    if (rooms[room] === undefined) {
        return safeCall(callback, undefined);
    }
    var users = [];
    rooms[room].sockets.map(function (sock) {
        users.push(sock.user);
    });

    safeCall(callback, users);
};

/*
 *Delete a user in a determined room.
 */
exports.deleteUser = function (user, room, callback) {
     if (rooms[room] === undefined) {
         return safeCall(callback, 'Success');
     }
    var sockets_to_delete = [];
    rooms[room].sockets.map(function (sock) {
        if (sock.user.name === user) {
            sockets_to_delete.push(sock);
        }
    });
    if (sockets_to_delete.length === 0) {
        log.error('User', user, 'does not exist');
        return safeCall(callback, 'User does not exist', 404);
    }
    sockets_to_delete.map(function (sock) {
        log.info('Deleted user', sock.user.name);
        sock.disconnect();
    });
    return safeCall(callback, 'Success');
};

/*
 * Delete a determined room.
 */
exports.deleteRoom = function (roomId, callback) {
    log.info('Deleting room ', roomId);

    var room = rooms[roomId];
    if (room === undefined) {
        return safeCall(callback, 'Success');
    }

    room.sockets.map(function (sock) {
        room.controller.unsubscribeAll(sock.id);
    });

    var streams = room.streams;
    for (var j in streams) {
        if (streams[j].hasAudio() || streams[j].hasVideo()) {
            if (!room.p2p) {
                room.controller.unpublish(j);
            }
        }
    }

    if (room.controller && typeof room.controller.destroy === 'function') {
        room.controller.destroy();
    }

    delete rooms[roomId];
    worker && worker.removeTask(roomId);
    updateMyState();
    log.info('Deleted room ', roomId, rooms);
    safeCall(callback, 'Success');
};

/*
 * Apply new configuration.
 */
exports.updateConfig = function (conf, callback) {
    safeCall(callback, 'Success');
};

exports.getConfig = function (callback) {
    safeCall(callback, {
        defaultVideoBW: GLOBAL.config.controller.defaultVideoBW,
        maxVideoBW: GLOBAL.config.controller.maxVideoBW,
        turnServer: GLOBAL.config.controller.turnServer,
        stunServerUrl: GLOBAL.config.controller.stunServerUrl,
        warning_n_rooms: GLOBAL.config.controller.warning_n_rooms,
        limit_n_rooms: GLOBAL.config.controller.limit_n_rooms
    });
};

exports.handleEventReport = function (event, room, spec) {
    log.debug(event, room, spec);
    if (typeof eventReportHandlers[event] === 'function') {
        eventReportHandlers[event](room, spec);
    }
};

var onTerminate = function () {
    for (var roomId in rooms) {
        exports.deleteRoom(roomId, function (){});
    }
    rooms = {};
};

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, function () {
        log.warn('Exiting on', sig);
        onTerminate();
        worker && worker.quit();
        process.exit();
    });
});

(function validateLocalConfig () {
    if (LIMIT_N_ROOMS === 0) {
        log.error('Invalid config: limit_n_rooms == 0');
    } else if (WARNING_N_ROOMS >= LIMIT_N_ROOMS) {
        log.error('Invalid config: warning_n_rooms >= limit_n_rooms');
    } else {
        amqper.connect(GLOBAL.config.rabbit, function () {
            try {
                amqper.setPublicRPC(rpcPublic);
                joinCluster(function (rpcID) {
                    log.info('portal join cluster ok, with rpcID:', rpcID);
                    amqper.bind(rpcID, listen);
                    controller.myId = rpcID;
                });
            } catch (error) {
                log.error('Error in Erizo Controller:', error);
            }
        });
    }
})();
