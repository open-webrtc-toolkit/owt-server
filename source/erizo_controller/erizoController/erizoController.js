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
                if (room.streams[i].getVideoRecorder() === recorderId+'') {
                    room.streams[i].setVideoRecorder('');
                }

                if (room.streams[i].getAudioRecorder() === recorderId+'') {
                    room.streams[i].setAudioRecorder('');
                }
            }
        }
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
                            sendMsgToOthersInRoom(socket.room, 'user_join', {user: user});
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
                                            room.controller = controller.RoomController(
                                                {amqper: amqper, room:roomID, config: resp}, 
                                                function (resolutions) {
                                                    room.enableMixing = resp.enableMixing;
                                                    on_ok();
                                                    if (resp.enableMixing) {
                                                        //var st = new ST.Stream({id: roomID, socket: '', audio: true, video: {category: 'mix'}, data: true, from: ''});
                                                        var st = new ST.Stream({id: roomID, socket: '', audio: true, video: {device: 'mcu', resolutions: resolutions}, from: '', attributes: null});
                                                        room.streams[roomID] = st;
                                                        room.mixer = roomID;
                                                        sendMsgToRoom(room, 'add_stream', st.getPublicStream());
                                                        if (room.config && room.config.publishLimit > 0) {
                                                            room.config.publishLimit++;
                                                        }
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
                            streamId = msg.payload.streamId + '';
                        socket.room.controller.onTrackControl(
                            socket.id,
                            streamId,
                            cmdOpts[0],
                            cmdOpts[1],
                            cmdOpts[2],
                            function () {
                                safeCall(callback, 'success');
                            }, function (error_reason) {
                                safeCall(callback, 'error', error_reason);
                            }
                        );
                    }
                } else {
                    safeCall(callback, 'error', "Invalid control message.");
                }
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
                socket.room.controller.onConnectionSignalling(socket.id, msg.streamId, msg.msg);
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
                    var recordingId = sdp;
                    if (GLOBAL.config.erizoController.recording_path) {
                        url = GLOBAL.config.erizoController.recording_path + recordingId + '.mkv';
                    } else {
                        url = '/tmp/' + recordingId + '.mkv';
                    }
                    stream_type = 'file';
                }
                if (!options.audio && !options.video) {
                    return safeCall(callback, 'error', 'no media input is specified to publish');
                }

                socket.room.controller.publish(
                    socket.id,
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

                            st = new ST.Stream({id: id, socket: socket.id, audio: options.audio, video: options.video, attributes: options.attributes, from: url});
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

                socket.room.controller.publish(
                    socket.id,
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
                            st = new ST.Stream({id: id, audio: options.audio, video: options.video, attributes: options.attributes, from: socket.id});
                            return;
                        case 'failed': // If the connection failed we remove the stream
                            log.info(signMess.reason, "removing " , id);
                            socket.emit('connection_failed',{});

                            socket.state = 'sleeping';
                            if (!socket.room.p2p) {
                                socket.room.controller.unpublish(id);
                                if (GLOBAL.config.erizoController.report.session_events) {
                                    var timeStamp = new Date();
                                    amqper.broadcast('event', {room: socket.room.id, user: socket.id, type: 'failed', stream: id, sdp: signMess.sdp, timestamp: timeStamp.getTime()});
                                }
                            }

                            var index = socket.streams.indexOf(id);
                            if (index !== -1) {
                                socket.streams.splice(index, 1);
                            }
                            safeCall(callback, 'error', signMess.reason);
                            return;
<<<<<<< HEAD
                        case 'ready':
                            // Double check if this socket is still in the room.
                            // It can be removed from the room if the socket is disconnected
                            // before the publish succeeds.
                            var index = socket.room.sockets.indexOf(socket);
                            if (index === -1) {
                                socket.room.controller.unpublish(id);
                                return;
                            }

                            socket.room.streams[id] = st;
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
                    if (GLOBAL.config.erizoController.report.session_events) {
                        var timeStamp = new Date();
                        amqper.broadcast('event', {room: socket.room.id, user: socket.id, name: socket.user.name, type: 'subscribe', stream: options.streamId, timestamp: timeStamp.getTime()});
                    }
                    if (typeof options.video === 'object' && options.video !== null) {
                        if (!stream.hasResolution(options.video.resolution)) {
                            options.video = true;
                        }
                    }
                    socket.room.controller.subscribe(
                        socket.id,
                        'webrtc',
                        options.streamId,
                        {require_audio: !!options.audio, require_video: !!options.video, video_resolution: (options.video && options.video.resolution) ? options.video.resolution : undefined},
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
                                // signMess.candidate = signMess.candidate.replace(privateRegexp, publicIP);
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
            var preferredVideoCodec = options.videoCodec || '';
            var preferredAudioCodec = options.audioCodec || '';;

            var videoRecorder = videoStream.getVideoRecorder();
            var audioRecorder = audioStream.getAudioRecorder();
            if ((videoRecorder && videoRecorder !== recorderId) || (audioRecorder && audioRecorder !== recorderId)) {
                return safeCall(callback, 'error', 'Media recording is going on.');
            }

            // Make sure the recording context clean for this 'startRecorder'
            socket.room.controller.unsubscribe(socket.room.id, recorderId);
            var isContinuous = false;
            for (var i in socket.room.streams) {
                if (socket.room.streams.hasOwnProperty(i)) {
                    if (socket.room.streams[i].getVideoRecorder() === recorderId+'') {
                        socket.room.streams[i].setVideoRecorder('');
                        isContinuous = true;
                    }

                    if (socket.room.streams[i].getAudioRecorder() === recorderId+'') {
                        socket.room.streams[i].setAudioRecorder('');
                        isContinuous = true;
                    }
                }
            }

            socket.room.controller.subscribeSelectively(
                socket.room.id,
                recorderId,
                'file',
                audioStreamId,
                videoStreamId,
                {require_audio: !!options.audioStreamId,
                 audio_codec: preferredAudioCodec,
                 require_video: !!options.videoStreamId,
                 video_codec: preferredVideoCodec,
                 interval: interval,
                 observer: 'erizoController_' + myId,
                 room_id: socket.room.id},
                function (response) {
                    if (response.type === 'ready') {
                        videoStream.setVideoRecorder(recorderId);
                        audioStream.setAudioRecorder(recorderId);

                        log.info('Media recording to ', url);

                        if (isContinuous) {
                            sendMsgToRoom(socket.room, 'reuse_recorder', {id: recorderId});
                        } else {
                            sendMsgToRoom(socket.room, 'add_recorder', {id: recorderId});
                        }

                        safeCall(callback, 'success', {
                            recorderId : recorderId,
                            host: publicIP,
                            path: url
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

            // Stop recorder
            if (options.recorderId) {
                socket.room.controller.unsubscribe(socket.room.id, options.recorderId);
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

                sendMsgToRoom(socket.room, 'remove_recorder', {id: options.recorderId});

                safeCall(callback, 'success', {
                            host: publicIP,
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
                    log.info('Region for ', options.id, ': ', result.text);
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
                if (GLOBAL.config.erizoController.report.session_events) {
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
                    socket.room.controller.unsubscribe(socket.id, to);
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
                    socket.room.controller.unsubscribeAll(socket.id);
                }

                for (i in socket.streams) {
                    if (socket.streams.hasOwnProperty(i)) {
                        id = socket.streams[i];
                        if (!socket.room.p2p) {
                            socket.room.controller.unpublish(id);
                            if (GLOBAL.config.erizoController.report.session_events) {
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

            if (socket.room !== undefined && !socket.room.p2p && GLOBAL.config.erizoController.report.session_events) {
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
