/*global require, setInterval, clearInterval, Buffer, exports, GLOBAL, process*/
'use strict';
var crypto = require('crypto');
var rpcPublic = require('./rpc/rpcPublic');
var ST = require('./Stream');
var config = require('./../../etc/woogeen_config');
var Permission = require('./permission');
var Getopt = require('node-getopt');
var io;

// Configuration default values
GLOBAL.config = config || {};
GLOBAL.config.erizoController = GLOBAL.config.erizoController || {};
// GLOBAL.config.erizoController.stunServerUrl = GLOBAL.config.erizoController.stunServerUrl || 'stun:stun.l.google.com:19302';
GLOBAL.config.erizoController.stunServerUrl = GLOBAL.config.erizoController.stunServerUrl;
GLOBAL.config.erizoController.defaultVideoBW = GLOBAL.config.erizoController.defaultVideoBW || 300;
GLOBAL.config.erizoController.maxVideoBW = GLOBAL.config.erizoController.maxVideoBW || 300;
GLOBAL.config.erizoController.publicIP = GLOBAL.config.erizoController.publicIP || '';
GLOBAL.config.erizoController.hostname = GLOBAL.config.erizoController.hostname|| '';
GLOBAL.config.erizoController.port = GLOBAL.config.erizoController.port || 8080;
GLOBAL.config.erizoController.ssl = GLOBAL.config.erizoController.ssl || false;
GLOBAL.config.erizoController.turnServer = GLOBAL.config.erizoController.turnServer || undefined;
if (config.erizoController.turnServer !== undefined) {
    GLOBAL.config.erizoController.turnServer.url = GLOBAL.config.erizoController.turnServer.url || '';
    GLOBAL.config.erizoController.turnServer.username = GLOBAL.config.erizoController.turnServer.username || '';
    GLOBAL.config.erizoController.turnServer.password = GLOBAL.config.erizoController.turnServer.password || '';
}
GLOBAL.config.erizoController.warning_n_rooms = (GLOBAL.config.erizoController.warning_n_rooms !== undefined ? GLOBAL.config.erizoController.warning_n_rooms : 15);
GLOBAL.config.erizoController.limit_n_rooms = (GLOBAL.config.erizoController.limit_n_rooms !== undefined ? GLOBAL.config.erizoController.limit_n_rooms : 20);
GLOBAL.config.erizoController.interval_time_keepAlive = GLOBAL.config.erizoController.interval_time_keepAlive || 1000;
GLOBAL.config.erizoController.report.session_events = GLOBAL.config.erizoController.report.session_events || false;
GLOBAL.config.erizoController.recording_path = GLOBAL.config.erizoController.recording_path || undefined;
GLOBAL.config.erizoController.roles = GLOBAL.config.erizoController.roles || {'presenter':{'publish': true, 'subscribe':true, 'record':true}, 'viewer':{'subscribe':true}, 'viewerWithData':{'subscribe':true, 'publish':{'audio':false,'video':false,'screen':false,'data':true}}};

// Parse command line arguments
var getopt = new Getopt([
  ['r' , 'rabbit-host=ARG'            , 'RabbitMQ Host'],
  ['g' , 'rabbit-port=ARG'            , 'RabbitMQ Port'],
  ['l' , 'logging-config-file=ARG'    , 'Logging Config File'],
  ['t' , 'stunServerUrl=ARG'          , 'Stun Server URL'],
  ['b' , 'defaultVideoBW=ARG'         , 'Default video Bandwidth'],
  ['M' , 'maxVideoBW=ARG'             , 'Max video bandwidth'],
  ['i' , 'publicIP=ARG'               , 'Erizo Controller\'s public IP'],
  ['H' , 'hostname=ARG'               , 'Erizo Controller\'s hostname'],
  ['p' , 'port'                       , 'Port where Erizo Controller will listen to new connections.'],
  ['S' , 'ssl'                        , 'Erizo Controller\'s hostname'],
  ['T' , 'turn-url'                   , 'Turn server\'s URL.'],
  ['U' , 'turn-username'              , 'Turn server\'s username.'],
  ['P' , 'turn-password'              , 'Turn server\'s password.'],
  ['R' , 'recording_path'             , 'Recording path.'],
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
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.host = value;
                break;
            case 'rabbit-port':
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.port = value;
                break;
            case 'logging-config-file':
                GLOBAL.config.logger = GLOBAL.config.logger || {};
                GLOBAL.config.logger.config_file = value;
                break;
            default:
                GLOBAL.config.erizoController[prop] = value;
                break;
        }
    }
}

// Load submodules with updated config
var logger = require('./../common/logger').logger;
var amqper = require('./../common/amqper');
var controller = require('./roomController');

// Logger
var log = logger.getLogger('ErizoController');

var nuveKey;

var WARNING_N_ROOMS = GLOBAL.config.erizoController.warning_n_rooms;
var LIMIT_N_ROOMS = GLOBAL.config.erizoController.limit_n_rooms;

var INTERVAL_TIME_KEEPALIVE = GLOBAL.config.erizoController.interval_time_keepAlive;

var BINDED_INTERFACE_NAME = GLOBAL.config.erizoController.networkInterface;

var myId;
var rooms = {};
var myState;

var calculateSignature = function (token) {
    var toSign = token.tokenId + ',' + token.host,
        signed = crypto.createHmac('sha256', nuveKey).update(toSign).digest('hex');
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
    room.sockets.map(function (sock) {
        log.info('Sending message to', sock.id, 'in room ', room.id);
        sock.emit(type, arg);
    });
};

var eventReportHandlers = {
    'updateStream': function (roomId, spec) {
        if (rooms[roomId]) {
            sendMsgToRoom(rooms[roomId], 'onUpdateStream', spec);
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
        room.controller.removePublisher(streamId);

        room.sockets.map(function (socket) {
            var index = socket.streams.indexOf(streamId);
            if (index !== -1) {
                socket.streams.splice(index, 1);
            }
        });

        if (room.streams[streamId]) {
            delete room.streams[streamId];
        }

        sendMsgToRoom(room, 'onRemoveStream', {id: streamId});
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

var privateRegexp;
var publicIP;

var addToCloudHandler = function (callback) {
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

    var addECToCloudHandler = function(attempt) {
        if (attempt <= 0) {
            return;
        }

        var controller = {
            cloudProvider: GLOBAL.config.cloudProvider.name,
            ip: publicIP,
            hostname: GLOBAL.config.erizoController.hostname,
            port: GLOBAL.config.erizoController.port,
            ssl: GLOBAL.config.erizoController.ssl
        };
        amqper.callRpc('nuve', 'addNewErizoController', controller, {callback: function (msg) {

            if (msg === 'timeout') {
                log.info('CloudHandler does not respond');

                // We'll try it more!
                setTimeout(function() {
                    attempt = attempt - 1;
                    addECToCloudHandler(attempt);
                }, 3000);
                return;
            }
            if (msg == 'error') {
                log.info('Error in communication with cloudProvider');
            }

            publicIP = msg.publicIP;
            myId = msg.id;
            myState = 2;

            var enableSSL = msg.ssl;
            var server;
            if (enableSSL === true) {
                log.info('SSL enabled!');
                var cipher = require('../../common/cipher');
                cipher.unlock(cipher.k, '../../cert/.woogeen.keystore', function cb (err, obj) {
                    if (!err) {
                        try {
                            server = require('https').createServer({
                                pfx: require('fs').readFileSync(config.erizoController.keystorePath),
                                passphrase: obj.erizoController
                            }).listen(msg.port);
                        } catch (e) {
                            err = e;
                        }
                    }
                    if (err) {
                        log.warn('Failed to setup secured server:', err);
                        return process.exit();
                    }
                    io = require('socket.io').listen(server);
                });
            } else {
                server = require('http').createServer().listen(msg.port);
                io = require('socket.io').listen(server);
            }

            var intervarId = setInterval(function () {

                amqper.callRpc('nuve', 'keepAlive', myId, {callback: function (result) {
                    if (result === 'whoareyou') {

                        // TODO: It should try to register again in Cloud Handler. But taking into account current rooms, users, ...
                        log.info('I don`t exist in cloudHandler. I`m going to be killed');
                        clearInterval(intervarId);
                        amqper.callRpc('nuve', 'killMe', publicIP, {callback: function () {}});
                    }
                }});

            }, INTERVAL_TIME_KEEPALIVE);

            amqper.callRpc('nuve', 'getKey', myId, {
                callback: function (key) {
                    if (key === 'error' || key === 'timeout') {
                        amqper.callRpc('nuve', 'killMe', publicIP, {callback: function () {}});
                        log.info('Failed to join nuve network.');
                        return process.exit();
                    }
                    nuveKey = key;
                    callback();
                }
            });

        }});
    };
    addECToCloudHandler(5);
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

    info = {id: myId, state: myState};
    amqper.callRpc('nuve', 'setInfo', info, {callback: function () {}});
};

function safeCall () {
    var callback = arguments[0];
    if (typeof callback === 'function') {
        var args = Array.prototype.slice.call(arguments, 1);
        callback.apply(null, args);
    }
}

var VideoResolutionMap = { // definition adopted from VideoLayout.h
    'cif':      [{width: 352, height: 288}],
    'vga':      [{width: 640, height: 480}, {width: 320, height: 240}],
    'svga':     [{width: 800, height: 600}, {width: 320, height: 240}],
    'xga':      [{width: 1024, height: 768}, {width: 320, height: 240}],
    'hd720p':   [{width: 1280, height: 720}, {width: 640, height: 480}, {width: 640, height: 360}],
    'sif':      [{width: 320, height: 240}],
    'hvga':     [{width: 480, height: 320}],
    'r480x360': [{width: 480, height: 360}, {width: 320, height: 240}],
    'qcif':     [{width: 176, height: 144}],
    'r192x144': [{width: 192, height: 144}],
    'hd1080p':  [{width: 1920, height: 1080}, {width: 1280, height: 720}, {width: 800, height: 600}, {width: 640, height: 480}, {width: 640, height: 360}],
    'uhd_4k':   [{width: 3840, height: 2160}, {width: 1920, height: 1080}, {width: 1280, height: 720}, {width: 800, height: 600}, {width: 640, height: 480}, {width: 640, height: 360}]
};

function calculateResolutions(rootResolution, useMultistreaming) {
    var base = VideoResolutionMap[rootResolution.toLowerCase()];
    if (!base) return [];
    if (!useMultistreaming) return [base[0]];
    return base.map(function (r) {
        return r;
    });
}

var initMixer = function (room, roomConfig, immediately) {
    if (roomConfig.enableMixing && room.mixer === undefined && room.initMixerTimer === undefined) {
        var id = room.id;
        room.enableMixing = true;
        var resolutions = calculateResolutions(roomConfig.mediaMixing.video.resolution, roomConfig.mediaMixing.video.multistreaming);
        if (immediately) {
            room.controller.initMixer(id, roomConfig.mediaMixing, function (result) {
                // TODO: Better to utilize 'result' to retrieve information from mixer to create the mix-stream.
                if (result === 'success') {
                    var st = new ST.Stream({id: id, socket: '', audio: true, video: {device: 'mcu', resolutions: resolutions}, from: '', attributes: null});
                    room.streams[id] = st;
                    room.mixer = id;
                    sendMsgToRoom(room, 'onAddStream', st.getPublicStream());
                    if (room.config && room.config.publishLimit > 0) {
                        room.config.publishLimit++;
                    }
                }
            });
        }
        // In case we need to do an RPC call to initialize the mixer when the first
        // client is connected, we may need to wait for a while because the room may
        // still be dirty at this moment (due to the fact that there's currently no
        // inter process synchronization for room deletion). This could happen when
        // the last user in the room refreshes the web page.
        // TODO: Revisit here for a better solution.
        var tryOut = 10;
        room.initMixerTimer = setInterval(function() {
            if (room.mixer !== undefined) {
                log.info('Mixer already existed in Room ', room.id);
                clearInterval(room.initMixerTimer);
                room.initMixerTimer = undefined;
                return;
            }

            room.controller.initMixer(id, roomConfig.mediaMixing, function (result) {
                // TODO: Better to utilize 'result' to retrieve information from mixer to create the mix-stream.
                if (result === 'success') {
                    var st = new ST.Stream({id: id, socket: '', audio: true, video: {device: 'mcu', resolutions: resolutions}, from: '', attributes: null});
                    room.streams[id] = st;
                    room.mixer = id;
                    sendMsgToRoom(room, 'onAddStream', st.getPublicStream());
                    clearInterval(room.initMixerTimer);
                    room.initMixerTimer = undefined;
                    if (room.config && room.config.publishLimit > 0) {
                        room.config.publishLimit++;
                    }
                } else if (--tryOut === 0) {
                    log.info('Mixer initialization failed in Room ', room.id);
                    clearInterval(room.initMixerTimer);
                    room.initMixerTimer = undefined;
                }
            });
        }, 100);
    }
};

var listen = function () {
    log.info('server on');

    io.sockets.on('connection', function (socket) {
        log.info('Socket connect', socket.id);

        function sendMsgToOthersInRoom (room, type, arg) {
            room.sockets.map(function (sock) {
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
                            var permissions = GLOBAL.config.erizoController.roles[tokenDB.role] || [];
                            socket.user.permissions = {};
                            for (var right in permissions) {
                                socket.user.permissions[right] = permissions[right];
                            }
                            socket.room = rooms[tokenDB.room];
                            socket.streams = []; //[list of streamIds]
                            socket.state = 'sleeping';

                            log.debug('OK, Valid token');

                            if (!tokenDB.p2p && GLOBAL.config.erizoController.report.session_events) {
                                var timeStamp = new Date();
                                amqper.broadcast('event', {room: tokenDB.room, user: socket.id, type: 'user_connection', timestamp:timeStamp.getTime()});
                            }

                            // Add client socket to room socket list and start to receive room events
                            socket.room.sockets.push(socket);

                            for (index in socket.room.streams) {
                                if (socket.room.streams.hasOwnProperty(index)) {
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
                                                defaultVideoBW: GLOBAL.config.erizoController.defaultVideoBW,
                                                maxVideoBW: GLOBAL.config.erizoController.maxVideoBW,
                                                stunServerUrl: GLOBAL.config.erizoController.stunServerUrl,
                                                turnServer: GLOBAL.config.erizoController.turnServer
                                                });
                            sendMsgToOthersInRoom(socket.room, 'onUserJoin', {user: user});
                        };

                        if (rooms[tokenDB.room] === undefined) {
                            if (myState === 0) {
                                return amqper.callRpc('nuve', 'reschedule', tokenDB.room, {callback: function () {
                                    var errMsg = 'Erizo: out of capacity';
                                    log.warn(errMsg);
                                    safeCall(callback, 'error', errMsg);
                                    socket.disconnect();
                                }});
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
                                            amqper.callRpc('nuve', 'allocErizoAgent', room.id, {callback: function (erizoAgent) {
                                                if (erizoAgent === 'error') {
                                                    log.error('Alloc ErizoAgent error.');
                                                    delete rooms[roomID];
                                                    on_error();
                                                } else if (erizoAgent === 'timeout') {
                                                    log.error('Alloc ErizoAgent timeout.');
                                                    delete rooms[roomID];
                                                    on_error();
                                                } else {
                                                    room.agent = erizoAgent.id;
                                                    room.controller = controller.RoomController({amqper: amqper, agent_id: erizoAgent.id, id: room.id});
                                                    room.controller.addEventListener(function(type, event) {
                                                        // TODO Send message to room? Handle ErizoJS disconnection.
                                                        if (type === 'unpublish') {
                                                            var streamId = event;
                                                            log.info('ErizoJS stopped', streamId);
                                                            sendMsgToRoom(room, 'onRemoveStream', {id: streamId});
                                                            room.controller.removePublisher(streamId);

                                                            for (var s in room.sockets) {
                                                                var streams = io.sockets.to(room.sockets[s]).streams;
                                                                var index = streams instanceof Array ? streams.indexOf(streamId) : -1;
                                                                if (index !== -1) {
                                                                    streams.splice(index, 1);
                                                                }
                                                            }

                                                            if (room.streams[streamId]) {
                                                                delete room.streams[streamId];
                                                            }

                                                            if (room.mixer !== undefined && room.mixer === streamId) {
                                                                room.mixer = undefined;
                                                                // Re-initialize the mixer in the room.
                                                                // Don't do it immediately because we want to wait for 
                                                                // the original mixer being cleaned-up.
                                                                initMixer(room, resp, false);
                                                            }
                                                        }

                                                    });

                                                    initMixer(room, resp, true);
                                                    on_ok();
                                                }
                                            }});
                                        }
                                    }});
                                }
                            };

                            var checkRoomReady = function(room, num) {
                                setTimeout(function () {
                                    if (room.enableMixing && room.mixer === undefined && num > 0) {
                                        if (num === 1) {
                                            log.warn('room initialzation is not ready before room connection return.');
                                        }
                                        checkRoomReady(room, num-1);
                                    } else {
                                        updateMyState();
                                        validateTokenOK();
                                    }
                                }, 100);
                            };

                            initRoom(tokenDB.room, function () {
                                checkRoomReady(rooms[tokenDB.room], 10);
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
                        var streamId = msg.payload.streamId + '';
                        action = socket.room.controller[action];
                        if (typeof action === 'function') {
                            action(streamId, socket.id, function (err) {
                                if (typeof callback === 'function') callback(err||'success');
                            });
                        } else {
                            if (typeof callback === 'function') callback('not implemented');
                        }
                        return;
                    }
                }
                if (typeof callback === 'function') callback('error');
                break;
            case 'data':
                if (socket.user === undefined || !socket.user.permissions[Permission.PUBLISH]) {
                    return safeCall(callback, 'error', 'unauthorized');
                }
                if (socket.user.permissions[Permission.PUBLISH] !== true) {
                    var permissions = socket.user.permissions[Permission.PUBLISH];
                    if (permissions['data'] === false)
                        return safeCall(callback, 'error', 'unauthorized');
                }

                var receiver = msg.receiver;
                var data = {
                    data: msg.data,
                    from: socket.id,
                    to: receiver
                };
                if (receiver === 'all') {
                    sendMsgToRoom(socket.room, 'onCustomMessage', data);
                    return safeCall(callback, 'success');
                } else {
                    for (var k in socket.room.sockets) {
                        if (socket.room.sockets[k].id === receiver) {
                            try {
                                socket.room.sockets[k].emit('onCustomMessage', data);
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
            socket.room.controller.addToMixer(streamId, socket.room.mixer, function (err) {
                if (err) return safeCall(callback, 'error', err);
                safeCall(callback, 'success');
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
            socket.room.controller.removeFromMixer(streamId, socket.room.mixer, function (err) {
                if (err) return safeCall(callback, 'error', err);
                safeCall(callback, 'success');
            });
        });

        socket.on('signaling_message', function (msg) {
            if (socket.room.p2p) {
                io.sockets.to(msg.peerSocket).emit('signaling_message_peer', {streamId: msg.streamId, peerSocket: socket.id, msg: msg.msg});
            } else {
                socket.room.controller.processSignaling(msg.streamId, socket.id, msg.msg);
            }
        });

        //Gets 'publish' messages on the socket in order to add new stream to the room.
        socket.on('publish', function (options, sdp, callback) {
            var id, st;
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
                var url = sdp;
                if (options.state === 'recording') {
                    var recordingId = sdp;
                    if (GLOBAL.config.erizoController.recording_path) {
                        url = GLOBAL.config.erizoController.recording_path + recordingId + '.mkv';
                    } else {
                        url = '/tmp/' + recordingId + '.mkv';
                    }
                }
                if (!options.audio && !options.video) {
                    return safeCall(callback, 'error', 'no media input is specified to publish');
                }

                socket.room.controller.addExternalInput(id, {
                    url: url,
                    audio: options.audio,
                    video: options.video,
                    unmix: options.unmix,
                    transport: options.transport,
                    buffer_size: options.bufferSize
                }, socket.room.mixer, function (result) {
                    if (result !== 'success') {
                        safeCall(callback, result); // TODO: ensure `callback' is accurately invoked once and only once.
                    }
                }, function () {
                    // Double check if this socket is still in the room.
                    // It can be removed from the room if the socket is disconnected
                    // before the publish succeeds.
                    var index = socket.room.sockets.indexOf(socket);
                    if (index === -1) {
                        socket.room.controller.removePublisher(id);
                        return;
                    }

                    st = new ST.Stream({id: id, socket: socket.id, audio: options.audio, video: options.video, attributes: options.attributes, from: url});
                    socket.streams.push(id);
                    socket.room.streams[id] = st;
                    sendMsgToRoom(socket.room, 'onAddStream', st.getPublicStream());
                    safeCall(callback, 'success', id);
                });
            } else if (options.state === 'erizo') {
                log.info("New publisher");
                id = socket.id;
                var mixer = socket.room.mixer;
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

                if (socket.room.config) {
                    if (socket.room.config.publishLimit >= 0 &&
                        socket.room.controller.publisherNum() >= socket.room.config.publishLimit) {
                        return safeCall(callback, 'error', 'max publishers');
                    }
                }

                if (GLOBAL.config.erizoController.report.session_events) {
                    var timeStamp = new Date();
                    amqper.broadcast('event', [{room: socket.room.id, user: socket.id, name: socket.user.name, type: 'publish', stream: id, timestamp: timeStamp.getTime()}]);
                }

                socket.room.controller.addPublisher(id, mixer, unmix, function (signMess) {
                    if (signMess.type === 'initializing') {
                        safeCall(callback, 'initializing', id);
                        st = new ST.Stream({id: id, audio: options.audio, video: options.video, attributes: options.attributes, from: socket.id});
                        return;
                    }

                    // If the connection failed we remove the stream
                    if (signMess.type ==='failed'){ 
                        log.info("IceConnection Failed on publisher, removing " , id);
                        socket.emit('connection_failed',{});

                        socket.state = 'sleeping';
                        if (!socket.room.p2p) {
                            socket.room.controller.removePublisher(id);
                            if (GLOBAL.config.erizoController.report.session_events) {
                                var timeStamp = new Date();
                                amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'failed', stream: id, sdp: signMess.sdp, timestamp: timeStamp.getTime()});
                            }
                        }

                        var index = socket.streams.indexOf(id);
                        if (index !== -1) {
                            socket.streams.splice(index, 1);
                        }
                        return;
                    }

                    if (signMess.type === 'ready') {
                        socket.room.streams[id] = st;
                        sendMsgToRoom(socket.room, 'onAddStream', st.getPublicStream());
                    } else if (signMess.type === 'timeout') {
                        safeCall(callback, 'error', 'No ErizoAgent available');
                    }

                    socket.emit('signaling_message_erizo', {mess: signMess, streamId: id});
                });

                socket.streams.push(id);
            } else {
                id = Math.random() * 1000000000000000000;
                st = new ST.Stream({id: id, socket: socket.id, audio: options.audio, video: options.video, attributes: options.attributes, from: socket.id});
                socket.streams.push(id);
                socket.room.streams[id] = st;
                safeCall(callback, '', id);
                sendMsgToRoom(socket.room, 'onAddStream', st.getPublicStream());
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
                    if (GLOBAL.config.erizoController.report.session_events) {
                        var timeStamp = new Date();
                        amqper.broadcast('event', {room: socket.room.id, user: socket.id, name: socket.user.name, type: 'subscribe', stream: options.streamId, timestamp: timeStamp.getTime()});
                    }
                    if (typeof options.video === 'object' && options.video !== null) {
                        if (!stream.hasResolution(options.video.resolution)) {
                            options.video = true;
                        }
                    }
                    socket.room.controller.addSubscriber(socket.id, options.streamId, options, function (signMess, errText) {
                        if (signMess.type === 'initializing') {
                            log.info("Initializing subscriber");
                            safeCall(callback, 'initializing');
                            return;
                        }

                        if (signMess.type === 'candidate') {
                            // signMess.candidate = signMess.candidate.replace(privateRegexp, publicIP);
                        }
                        socket.emit('signaling_message_erizo', {mess: signMess, peerId: options.streamId});
                    });
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

            // Check the stream to be recorded
            var videoStreamId = options.videoStreamId || undefined;
            var audioStreamId = options.audioStreamId || undefined;
            if (videoStreamId === undefined && audioStreamId === undefined) {
                videoStreamId = socket.room.mixer;
                audioStreamId = socket.room.mixer;
            } else if (videoStreamId === undefined) {
                videoStreamId = audioStreamId;
            } else if (audioStreamId === undefined){
                audioStreamId = videoStreamId;
            }

            var videoStream = socket.room.streams[videoStreamId];
            var audioStream = socket.room.streams[audioStreamId];
            if (videoStream === undefined || audioStream === undefined) {
                return safeCall(callback, 'error', 'Target video or audio record stream does not exist.');
            }

            if (!videoStream.hasVideo()) {
                return safeCall(callback, 'error', 'No video data from the video stream.');
            }

            if (!audioStream.hasAudio()) {
                return safeCall(callback, 'error', 'No audio data from the audio stream.');
            }

            var timeStamp = new Date();
            var recorderId = options.recorderId || formatDate(timeStamp, 'yyyyMMddhhmmssSS');
            var recorderPath = options.path || GLOBAL.config.erizoController.recording_path || '/tmp';
            var url = require('path').join(recorderPath, 'room' + socket.room.id + '_' + recorderId + '.mkv');
            var interval = (options.interval && options.interval > 0) ? options.interval : -1;

            var videoRecorder = videoStream.getVideoRecorder();
            var audioRecorder = audioStream.getAudioRecorder();
            if ((videoRecorder && videoRecorder !== recorderId) || (audioRecorder && audioRecorder !== recorderId)) {
                return safeCall(callback, 'error', 'Media recording is going on.');
            }

            // Make sure the recording context clean for this 'startRecorder'
            socket.room.controller.removeExternalOutput(recorderId, false, function (result) {
                if (result.success) {
                    for (var i in socket.room.streams) {
                        if (socket.room.streams.hasOwnProperty(i)) {
                            if (socket.room.streams[i].getVideoRecorder() === recorderId+'') {
                                socket.room.streams[i].setVideoRecorder('');
                            }

                            if (socket.room.streams[i].getAudioRecorder() === recorderId+'') {
                                socket.room.streams[i].setAudioRecorder('');
                            }
                        }
                    }

                    log.info('Recorder context cleaned: ', result.text);

                    // Start the recorder
                    socket.room.controller.addExternalOutput(videoStreamId, audioStreamId, recorderId, socket.room.mixer, url, interval, function (result) {
                        if (result.success) {
                            videoStream.setVideoRecorder(recorderId);
                            audioStream.setAudioRecorder(recorderId);

                            log.info('Recorder started: ', url);

                            safeCall(callback, 'success', {
                                recorderId : recorderId,
                                host: publicIP,
                                path: url
                            });
                        } else {
                            safeCall(callback, 'error', 'Error in start recording: ' + result.text);
                        }
                    });
                } else {
                    safeCall(callback, 'error', 'Error during cleaning recording: ' + result.text);
                }
            });
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

            // Stop recorder
            if (options.recorderId) {
                socket.room.controller.removeExternalOutput(options.recorderId, true, function (result) {
                    if (result.success) {
                        for (var i in socket.room.streams) {
                            if (socket.room.streams.hasOwnProperty(i)) {
                                if (socket.room.streams[i].getVideoRecorder() === options.recorderId+'') {
                                    socket.room.streams[i].setVideoRecorder('');
                                }

                                if (socket.room.streams[i].getAudioRecorder() === options.recorderId+'') {
                                    socket.room.streams[i].setAudioRecorder('');
                                }
                            }
                        }

                        log.info('Recorder stopped: ', result.text);

                        safeCall(callback, 'success', {
                            host: publicIP,
                            recorderId: result.text
                        });
                    } else {
                        safeCall(callback, 'error', 'Error in stop recording: ' + result.text);
                    }
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
                socket.room.controller.getRegion(socket.room.mixer, options.id, function (result) {
                    if (result.success) {
                        log.info('Region for ', options.id, ': ', result.text);
                        safeCall(callback, 'success', {
                            region: result.text
                        });
                    } else {
                        safeCall(callback, 'error', 'Error in getRegion: ' + result.text);
                    }
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
                socket.room.controller.setRegion(socket.room.mixer, options.id, options.region, function (err) {
                    if (err) return safeCall(callback, 'error', err);
                    safeCall(callback, 'success');
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
                socket.room.controller.setVideoBitrate(socket.room.mixer, options.id, options.bitrate, function (err) {
                    if (err) return safeCall(callback, 'error', err);
                    safeCall(callback, 'success');
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

            sendMsgToRoom(socket.room, 'onRemoveStream', {id: streamId});

            socket.state = 'sleeping';
            if (!socket.room.p2p) {
                socket.room.controller.removePublisher(streamId);
                if (GLOBAL.config.erizoController.report.session_events) {
                    var timeStamp = new Date();
                    amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'unpublish', stream: streamId, timestamp: timeStamp.getTime()});
                }
            }

            socket.streams.splice(index, 1);
            if (socket.room.streams[streamId]) {
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
                    socket.room.controller.removeSubscriber(socket.id, to);
                    if (GLOBAL.config.erizoController.report.session_events) {
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

            for (i in socket.streams) {
                if (socket.streams.hasOwnProperty(i)) {
                    sendMsgToRoom(socket.room, 'onRemoveStream', {id: socket.streams[i]});
                }
            }

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
                    socket.room.controller.removeSubscriptions(socket.id);
                }

                for (i in socket.streams) {
                    if (socket.streams.hasOwnProperty(i)) {
                        id = socket.streams[i];
                        if (!socket.room.p2p) {
                            socket.room.controller.removePublisher(id);
                            if (GLOBAL.config.erizoController.report.session_events) {
                                amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'unpublish', stream: id, timestamp: (new Date()).getTime()});
                            }
                        }

                        if (socket.room.streams[id]) {
                            delete socket.room.streams[id];
                        }
                    }
                }
            }

            if (socket.room !== undefined && !socket.room.p2p && GLOBAL.config.erizoController.report.session_events) {
                amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'user_disconnection', timestamp: (new Date()).getTime()});
            }

            if (socket.room !== undefined && socket.room.sockets.length === 0) {
                log.info('Empty room ', socket.room.id, '. Deleting it');
                if (!socket.room.p2p) {
                    if (socket.room.initMixerTimer !== undefined) {
                        clearInterval(socket.room.initMixerTimer);
                        socket.room.initMixerTimer = undefined;
                    }

                    if (socket.room.mixer === undefined) {
                        return;
                    }

                    if (socket.room.controller && typeof socket.room.controller.removePublisher === 'function') {
                        socket.room.controller.removePublisher(socket.room.mixer);
                    }
                    if (socket.room.streams[socket.room.mixer]) {
                        delete socket.room.streams[socket.room.mixer];
                    }
                    socket.room.mixer = undefined;
                }
                if (socket.room.agent !== undefined) {
                    amqper.callRpc('nuve', 'freeErizoAgent', socket.room.agent);
                }
                delete rooms[socket.room.id];
                updateMyState();
            } else if (socket.room !== undefined) {
                sendMsgToOthersInRoom(socket.room, 'onUserLeave', {user: socket.user});
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
        room.controller.removeSubscriptions(sock.id);
    });

    var streams = room.streams;
    for (var j in streams) {
        if (streams[j].hasAudio() || streams[j].hasVideo()) {
            if (!room.p2p) {
                room.controller.removePublisher(j);
            }
        }
    }

    if (room.agent !== undefined) {
        amqper.callRpc('nuve', 'freeErizoAgent', room.agent);
    }
    delete rooms[roomId];
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
        defaultVideoBW: GLOBAL.config.erizoController.defaultVideoBW,
        maxVideoBW: GLOBAL.config.erizoController.maxVideoBW,
        turnServer: GLOBAL.config.erizoController.turnServer,
        stunServerUrl: GLOBAL.config.erizoController.stunServerUrl,
        warning_n_rooms: GLOBAL.config.erizoController.warning_n_rooms,
        limit_n_rooms: GLOBAL.config.erizoController.limit_n_rooms
    });
};

exports.handleEventReport = function (event, room, spec) {
    log.debug(event, room, spec);
    if (typeof eventReportHandlers[event] === 'function') {
        eventReportHandlers[event](room, spec);
    }
};

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, function () {
        log.warn('Exiting on', sig);
        process.exit();
    });
});

(function validateLocalConfig () {
    if (LIMIT_N_ROOMS === 0) {
        log.info('Invalid config: limit_n_rooms == 0');
    } else if (WARNING_N_ROOMS >= LIMIT_N_ROOMS) {
        log.info('Invalid config: warning_n_rooms >= limit_n_rooms');
    } else {
        amqper.connect(function () {
            try {
                amqper.setPublicRPC(rpcPublic);
                addToCloudHandler(function () {
                    var rpcID = 'erizoController_' + myId;
                    amqper.bind(rpcID, listen);
                    controller.myId = 'erizoController_' + myId;
                });
            } catch (error) {
                log.info('Error in Erizo Controller:', error);
            }
        });
    }
})();
