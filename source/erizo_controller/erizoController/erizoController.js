/*global require, logger. setInterval, clearInterval, Buffer, exports*/
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
GLOBAL.config.erizoController.warning_n_rooms = (GLOBAL.config.erizoController.warning_n_rooms != undefined ? GLOBAL.config.erizoController.warning_n_rooms : 15);
GLOBAL.config.erizoController.limit_n_rooms = (GLOBAL.config.erizoController.limit_n_rooms != undefined ? GLOBAL.config.erizoController.limit_n_rooms : 20);
GLOBAL.config.erizoController.interval_time_keepAlive = GLOBAL.config.erizoController.interval_time_keepAlive || 1000;
GLOBAL.config.erizoController.sendStats = GLOBAL.config.erizoController.sendStats || false;
GLOBAL.config.erizoController.recording_path = GLOBAL.config.erizoController.recording_path || undefined;
GLOBAL.config.erizoController.roles = GLOBAL.config.erizoController.roles || {"presenter":{"publish": true, "subscribe":true, "record":true}, "viewer":{"subscribe":true}, "viewerWithData":{"subscribe":true, "publish":{"audio":false,"video":false,"screen":false,"data":true}}};
GLOBAL.config.erizoController.mixer = GLOBAL.config.erizoController.mixer || false;

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

opt = getopt.parse(process.argv.slice(2));

for (var prop in opt.options) {
    if (opt.options.hasOwnProperty(prop)) {
        var value = opt.options[prop];
        switch (prop) {
            case "help":
                getopt.showHelp();
                process.exit(0);
                break;
            case "rabbit-host":
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.host = value;
                break;
            case "rabbit-port":
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.port = value;
                break;
            case "logging-config-file":
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
var rpc = require('./../common/rpc');
var controller = require('./roomController');

// Logger
var log = logger.getLogger("ErizoController");

var nuveKey;

var WARNING_N_ROOMS = GLOBAL.config.erizoController.warning_n_rooms;
var LIMIT_N_ROOMS = GLOBAL.config.erizoController.limit_n_rooms;

var INTERVAL_TIME_KEEPALIVE = GLOBAL.config.erizoController.interval_time_keepAlive;

var BINDED_INTERFACE_NAME = GLOBAL.config.erizoController.networkInterface;

var myId;
var rooms = {};
var myState;

var calculateSignature = function (token) {
    "use strict";

    var toSign = token.tokenId + ',' + token.host,
        signed = crypto.createHmac('sha256', nuveKey).update(toSign).digest('hex');
    return (new Buffer(signed)).toString('base64');
};

var checkSignature = function (token) {
    "use strict";

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
    "use strict";

    var sockets = room.sockets,
        id;
    for (id in sockets) {
        if (sockets.hasOwnProperty(id)) {
            log.info('Sending message to', sockets[id], 'in room ', room.id);
            io.sockets.socket(sockets[id]).emit(type, arg);
        }
    }
};

var privateRegexp;
var publicIP;

var addToCloudHandler = function (callback) {
    "use strict";

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
        rpc.callRpc('nuve', 'addNewErizoController', controller, {callback: function (msg) {

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
                    io = require('socket.io').listen(server, {log: false});
                    io.set('log level', 0);
                });
            } else {
                server = require('http').createServer().listen(msg.port);
                io = require('socket.io').listen(server, {log: false});
                io.set('log level', 0);
            }

            var intervarId = setInterval(function () {

                rpc.callRpc('nuve', 'keepAlive', myId, {"callback": function (result) {
                    if (result === 'whoareyou') {

                        // TODO: It should try to register again in Cloud Handler. But taking into account current rooms, users, ...
                        log.info('I don`t exist in cloudHandler. I`m going to be killed');
                        clearInterval(intervarId);
                        rpc.callRpc('nuve', 'killMe', publicIP, {callback: function () {}});
                    }
                }});

            }, INTERVAL_TIME_KEEPALIVE);

            rpc.callRpc('nuve', 'getKey', myId, {
                callback: function (key) {
                    if (key === 'error' || key === 'timeout') {
                        rpc.callRpc('nuve', 'killMe', publicIP, {callback: function () {}});
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
    "use strict";

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
    rpc.callRpc('nuve', 'setInfo', info, {callback: function () {}});
};

function safeCall () {
    var callback = arguments[0];
    if (typeof callback === 'function') {
        var args = Array.prototype.slice.call(arguments, 1);
        callback.apply(null, args);
    }
}

var initMixer = function (room, roomConfig, immediately) {
    if (GLOBAL.config.erizoController.mixer && room.mixer === undefined && room.initMixerTimer === undefined) {
        var id = room.id;
        if (immediately) {
            room.controller.initMixer(id, roomConfig, function (result) {
                if (result === 'success') {
                    var st = new ST.Stream({id: id, socket: '', audio: true, video: {category: 'mix'}, data: true, from: ''});
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

            room.controller.initMixer(id, roomConfig, function (result) {
                if (result === 'success') {
                    var st = new ST.Stream({id: id, socket: '', audio: true, video: {category: 'mix'}, data: true, from: ''});
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
    "use strict";
    log.info('server on');

    io.sockets.on('connection', function (socket) {
        log.info("Socket connect ", socket.id);

        function sendMsgToOthersInRoom (room, type, arg) {
            'use strict';
            room.sockets.map(function (sock) {
                if (sock !== socket.id) {
                    log.info('Sending message to', sock, 'in room ', room.id);
                    io.sockets.socket(sock).emit(type, arg);
                }
            });
        };
        // Gets 'token' messages on the socket. Checks the signature and ask nuve if it is valid.
        // Then registers it in the room and callback to the client.
        socket.on('token', function (token, callback) {

            log.debug("New token", token);

            var tokenDB, user, streamList = [], index;

            if (checkSignature(token)) {

                rpc.callRpc('nuve', 'deleteToken', token.tokenId, {callback: function (resp) {
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

                            if (!tokenDB.p2p && GLOBAL.config.erizoController.sendStats) {
                                var timeStamp = new Date();
                                rpc.callRpc('stats_handler', 'event', [{room: tokenDB.room, user: socket.id, type: 'connection', timestamp:timeStamp.getTime()}]);
                            }

                            for (index in socket.room.streams) {
                                if (socket.room.streams.hasOwnProperty(index)) {
                                    streamList.push(socket.room.streams[index].getPublicStream());
                                }
                            }

                            safeCall(callback, 'success', {streams: streamList,
                                                id: socket.room.id,
                                                clientId: socket.id,
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
                                return rpc.callRpc('nuve', 'reschedule', tokenDB.room, {callback: function () {
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
                                room.sockets.push(socket.id);
                                room.streams = {}; //streamId: Stream
                                rooms[roomID] = room;
                                if (tokenDB.p2p) {
                                    log.debug('Token of p2p room');
                                    room.p2p = true;
                                    on_ok();
                                } else {
                                    rpc.callRpc('nuve', 'getRoomConfig', room.id, {callback: function (resp) {
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
                                            room.controller = controller.RoomController({rpc: rpc});
                                            room.controller.addEventListener(function(type, event) {
                                                // TODO Send message to room? Handle ErizoJS disconnection.
                                                if (type === "unpublish") {
                                                    var streamId = event;
                                                    log.info("ErizoJS stopped", streamId);
                                                    sendMsgToRoom(room, 'onRemoveStream', {id: streamId});
                                                    room.controller.removePublisher(streamId);

                                                    var index = socket.streams.indexOf(streamId);
                                                    if (index !== -1) {
                                                        socket.streams.splice(index, 1);
                                                    }
                                                    if (room.streams[streamId]) {
                                                        delete room.streams[streamId];
                                                    }

                                                    if (room.mixer !== undefined && room.mixer === streamId) {
                                                        room.mixer = undefined;
                                                        // Re-initialize the mixer in the room.
                                                        // Don't do it immediately because we want to wait for 
                                                        // the original mixer being cleaned-up.
                                                        initMixer(room, resp.mediaMixing, false);
                                                    }
                                                }

                                            });

                                            initMixer(room, resp.mediaMixing, true);
                                            on_ok();
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
                            rooms[tokenDB.room].sockets.push(socket.id);
                            validateTokenOK();
                        }
                    } else {
                        log.warn('Invalid host');
                        safeCall(callback, 'error', 'Invalid host');
                        socket.disconnect();
                    }
                }});

            } else {
                log.warn("Authentication error");
                safeCall(callback, 'error', 'Authentication error');
                socket.disconnect();
            }
        });


        socket.on('customMessage', function (msg, callback) {
            switch (msg.type) {
            case 'login': // reserved
                break;
            case 'control': // stream media manipulation
                if (typeof msg.payload === 'object' && msg.payload !== null) {
                    var action = msg.payload.action;
                    var streamId = msg.payload.streamId;
                    if (/^((audio)|(video))-((in)|(out))-((on)|(off))$/.test(action)) {
                    //     action = socket.room.controller[action];
                    //     if (typeof streamId === 'string') {
                    //         streamId = parseInt(streamId, 10);
                    //     }
                    //     if (typeof action === 'function') {
                    //         action(streamId);
                    //         if (typeof callback === 'function') callback('success');
                    //         return;
                    //     }
                    }
                }
                if (typeof callback === 'function') callback('error');
                break;
            case 'data':
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
                    if (socket.room.sockets.indexOf(receiver) === -1) {
                        return safeCall(callback, 'error', 'invalid receiver');
                    }
                    try {
                        io.sockets.socket(receiver).emit('onCustomMessage', data);
                        safeCall(callback, 'success');
                    } catch (err) {
                        safeCall(callback, 'error', err);
                    }
                }
                break;
            default: // do nothing
                return safeCall(callback, 'error', 'invalid message type');
            }
        });

        //Gets 'sendDataStream' messages on the socket in order to write a message in a dataStream.
        socket.on('sendDataStream', function (msg) {
            if (msg.id !== undefined && socket.room.streams[msg.id]) {
                var sockets = socket.room.streams[msg.id].getDataSubscribers(), id;
                for (id in sockets) {
                    if (sockets.hasOwnProperty(id)) {
                        log.info('Sending dataStream to', sockets[id], 'in stream ', msg.id);
                        io.sockets.socket(sockets[id]).emit('onDataStream', msg);
                    }
                }
            }

            // send to mix stream subscribers
            var mixer = socket.room.mixer;
            if (mixer && socket.room.streams[mixer]) {
                var sockets = socket.room.streams[mixer].getDataSubscribers(), id;
                for (id in sockets) {
                    if (sockets.hasOwnProperty(id)) {
                        log.info('Sending dataStream to', sockets[id], 'in stream ', mixer);
                        io.sockets.socket(sockets[id]).emit('onDataStream', msg);
                    }
                }
            }
        });

        //Gets 'updateStreamAttributes' messages on the socket in order to update attributes from the stream.
        socket.on('updateStreamAttributes', function (msg) {
            var sockets = socket.room.streams[msg.id].getDataSubscribers(), id;
            socket.room.streams[msg.id].setAttributes(msg.attrs);
            for (id in sockets) {
                if (sockets.hasOwnProperty(id)) {
                    log.info('Sending new attributes to', sockets[id], 'in stream ', msg.id);
                    io.sockets.socket(sockets[id]).emit('onUpdateAttributeStream', msg);
                }
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
                id = Math.random() * 1000000000000000000;
                var url = sdp;
                if (options.state === 'recording') {
                    var recordingId = sdp;
                    if (GLOBAL.config.erizoController.recording_path) {
                        url = GLOBAL.config.erizoController.recording_path + recordingId + '.mkv';
                    } else {
                        url = '/tmp/' + recordingId + '.mkv';
                    }
                }
                socket.room.controller.addExternalInput(id, url, function (result) {
                    if (result === 'success') {
                        st = new ST.Stream({id: id, socket: socket.id, audio: options.audio, video: options.video, data: options.data, attributes: options.attributes, from: url});
                        socket.streams.push(id);
                        socket.room.streams[id] = st;
                        safeCall(callback, result, id);
                        sendMsgToRoom(socket.room, 'onAddStream', st.getPublicStream());
                    } else {
                        safeCall(callback, result);
                    }
                });
            } else if (options.state !== 'data' && !socket.room.p2p) {
                if (options.state === 'offer' && socket.state === 'sleeping') {
                    // id = Math.random() * 1000000000000000000;
                    id = socket.id;
                    var mixer = socket.room.mixer;
                    var hasScreen = false;
                    if (options.video && options.video.device === 'screen') {
                        hasScreen = true;
                        id = id.slice(0, -8) + '_SCREEN_';
                        mixer = undefined;
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

                    if (GLOBAL.config.erizoController.sendStats) {
                        var timeStamp = new Date();
                        rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'publish', stream: id, timestamp: timeStamp.getTime()}]);
                    }

                    socket.room.controller.addPublisher(id, sdp, mixer, function (answer) {
                        socket.state = 'waitingOk';
                        answer = answer.replace(privateRegexp, publicIP);
                        safeCall(callback, answer, id);
                    }, function() {
                        // Double check if this socket is still in the room.
                        // It can be removed from the room if the socket is disconnected
                        // before the publish succeeds.
                        var index = socket.room.sockets.indexOf(socket.id);
                        if (index === -1) {
                            return;
                        }

                        st = new ST.Stream({id: id, audio: options.audio, video: options.video, data: options.data, screen: hasScreen, attributes: options.attributes, from: socket.id});
                        socket.state = 'sleeping';
                        socket.room.streams[id] = st;

                        if (socket.room.streams[id] !== undefined) {
                            sendMsgToRoom(socket.room, 'onAddStream', socket.room.streams[id].getPublicStream());
                        }
                    });

                    socket.streams.push(id);
                } else if (options.state === 'ok' && socket.state === 'waitingOk') {
                }
            } else if (options.state === 'p2pSignaling') {
                io.sockets.socket(options.subsSocket).emit('onPublishP2P', {sdp: sdp, streamId: options.streamId}, function(answer) {
                    safeCall(callback, answer);
                });
            } else {
                id = Math.random() * 1000000000000000000;
                var hasScreen = false;
                if (options.video && options.video.device === 'screen') {
                    hasScreen = true;
                }
                st = new ST.Stream({id: id, socket: socket.id, audio: options.audio, video: options.video, data: options.data, screen: hasScreen, attributes: options.attributes, from: socket.id});
                socket.streams.push(id);
                socket.room.streams[id] = st;
                safeCall(callback, undefined, id);
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

            if (stream.hasAudio() || stream.hasVideo() || stream.hasScreen()) {

                if (socket.room.p2p) {
                    var s = stream.getSocket();
                    io.sockets.socket(s).emit('onSubscribeP2P', {streamId: options.streamId, subsSocket: socket.id}, function(offer) {
                        safeCall(callback, offer);
                    });

                } else {
                    if (GLOBAL.config.erizoController.sendStats) {
                        var timeStamp = new Date();
                        rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'subscribe', stream: options.streamId, timestamp: timeStamp.getTime()}]);
                    }
                    socket.room.controller.addSubscriber(socket.id, options.streamId, options.audio, options.video, sdp, function (answer, errText) {
                        answer = answer.replace(privateRegexp, publicIP);
                        safeCall(callback, answer, errText);
                    }, function() {
                        log.info("Subscriber added");
                    });
                }
            } else {
                safeCall(callback, undefined);
            }
        });

        //Gets 'startRecorder' messages
        socket.on('startRecorder', function (options, callback) {
            if (typeof options === 'function') {
                callback = options;
            }

            if (socket.user === undefined || !socket.user.permissions[Permission.RECORD]) {
                return safeCall(callback, 'error', 'unauthorized');
            }

            var path = require('path');
            var url = path.join((GLOBAL.config.erizoController.recording_path || '/tmp'),
                'room' + socket.room.id + '_' + (Math.random() * 1000000000000000000) + '.mkv');

            // start recording mix stream
            var mixer = socket.room.mixer;
            if (mixer && socket.room.streams[mixer] &&
                (socket.room.streams[mixer].hasAudio() ||
                    socket.room.streams[mixer].hasVideo() ||
                    socket.room.streams[mixer].hasScreen())) {
                socket.room.controller.startRecorder(mixer, url, function (result) {
                    if (result.success) {
                        log.info('Recorder started:', url);
                        safeCall(callback, 'success', {
                            host: publicIP,
                            path: url
                        });
                    } else {
                        safeCall(callback, 'error', 'Error in start recording: ' + result.text);
                    }
                });
            } else {
                safeCall(callback, 'error', 'No mix stream found in room');
            }
        });

        //Gets 'stopRecorder' messages
        socket.on('stopRecorder', function (options, callback) {
            if (typeof options === 'function') {
                callback = options;
            }

            if (socket.user === undefined || !socket.user.permissions[Permission.RECORD]) {
                return safeCall(callback, 'error', 'unauthorized');
            }

            // stop recording mix stream
            var mixer = socket.room.mixer;
            if (mixer && socket.room.streams[mixer] &&
                (socket.room.streams[mixer].hasAudio() ||
                    socket.room.streams[mixer].hasVideo() ||
                    socket.room.streams[mixer].hasScreen())) {
                socket.room.controller.stopRecorder(mixer, function (result) {
                    if (result.success) {
                        log.info('Recorder stopped:', result.text);
                        safeCall(callback, 'success', {
                            host: publicIP,
                            path: result.text
                        });
                    } else {
                        safeCall(callback, 'error', 'Error in stop recording: ' + result.text);
                    }
                });
            } else {
                safeCall(callback, 'error', 'No mix stream found in room');
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
                if (GLOBAL.config.erizoController.sendStats) {
                    var timeStamp = new Date();
                    rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'unpublish', stream: streamId, timestamp: timeStamp.getTime()}]);
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

            if (socket.room.streams[to].hasAudio() || socket.room.streams[to].hasVideo() || socket.room.streams[to].hasScreen()) {
                if (!socket.room.p2p) {
                    socket.room.controller.removeSubscriber(socket.id, to);
                    if (GLOBAL.config.erizoController.sendStats) {
                        var timeStamp = new Date();
                        rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'unsubscribe', stream: to, timestamp:timeStamp.getTime()}]);
                    }
                };
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

                index = socket.room.sockets.indexOf(socket.id);
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
                            if (GLOBAL.config.erizoController.sendStats) {
                                var timeStamp = new Date();
                                rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'unpublish', stream: id, timestamp: timeStamp.getTime()}]);
                            }
                        }

                        if (socket.room.streams[id]) {
                            delete socket.room.streams[id];
                        }
                    }
                }
            }

            if (socket.room !== undefined && !socket.room.p2p && GLOBAL.config.erizoController.sendStats) {
                var timeStamp = new Date();
                rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'disconnection', timestamp: timeStamp.getTime()}]);
            }

            if (socket.room !== undefined && socket.room.sockets.length === 0) {
                log.info('Empty room ', socket.room.id, '. Deleting it');
                if (GLOBAL.config.erizoController.mixer) {
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
    "use strict";

    var users = [], sockets, id;
    if (rooms[room] === undefined) {
        return safeCall(callback, undefined);
    }

    sockets = rooms[room].sockets;

    for (id in sockets) {
        if (sockets.hasOwnProperty(id)) {
            users.push(io.sockets.socket(sockets[id]).user);
        }
    }

    safeCall(callback, users);
};

/*
 *Gets a list of users in a determined room.
 */
exports.deleteUser = function (user, room, callback) {
    "use strict";

    var users = [], sockets, id;

     if (rooms[room] === undefined) {
         return safeCall(callback, 'Success');
     }

    sockets = rooms[room].sockets;
    var sockets_to_delete = [];

    for (id in sockets) {
        if (sockets.hasOwnProperty(id)) {
            if (io.sockets.socket(sockets[id]).user.name === user){
                sockets_to_delete.push(sockets[id]);
            }
        }
    }

    for (var s in sockets_to_delete) {

        log.info('Deleted user', io.sockets.socket(sockets_to_delete[s]).user.name);
        io.sockets.socket(sockets_to_delete[s]).disconnect();
    }

    if (sockets_to_delete.length !== 0) {
        return safeCall(callback, 'Success');
    }
    else {
        log.error('User', user, 'does not exist');
        return safeCall(callback, 'User does not exist', 404);
    }
};


/*
 * Delete a determined room.
 */
exports.deleteRoom = function (roomId, callback) {
    "use strict";

    var sockets, streams, id, j;

    log.info('Deleting room ', roomId);

    var room = rooms[roomId];
    if (room === undefined) {
        return safeCall(callback, 'Success');
    }

    sockets = room.sockets;
    for (id in sockets) {
        if (sockets.hasOwnProperty(id)) {
            room.controller.removeSubscriptions(sockets[id]);
        }
    }

    streams = room.streams;
    for (j in streams) {
        if (streams[j].hasAudio() || streams[j].hasVideo() || streams[j].hasScreen()) {
            if (!room.p2p) {
                room.controller.removePublisher(j);
            }
        }
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


function spawnAgent (id) { // called once.
    var spawn = require('child_process').spawn;
    log.info('starting erizoAgent', id);
    var agent = spawn('node', ['./../erizoAgent/erizoAgent.js', '-I', id], { detached: true, stdio: 'inherit' });
    agent.unref();
    log.info('erizoAgent pid', agent.pid);
    controller.agentId = 'ErizoAgent_' + id; // save agentId for callRpc() in roomController.
    process.on('exit', function () { // kill agent on exiting.
        log.info('ErizoController closed');
        agent.kill('SIGTERM');
    });
}

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, function () {
        log.warn('Exiting on', sig);
        process.exit();
    });
});

(function validateLocalConfig () {
    if (LIMIT_N_ROOMS == 0) {
        log.info("Invalid config: limit_n_rooms == 0");
    } else if (WARNING_N_ROOMS >= LIMIT_N_ROOMS) {
        log.info("Invalid config: warning_n_rooms >= limit_n_rooms");
    } else {
        rpc.connect(function () {
            "use strict";
            try {
                rpc.setPublicRPC(rpcPublic);
                addToCloudHandler(function () {
                    var rpcID = 'erizoController_' + myId;
                    rpc.bind(rpcID, listen);
                    spawnAgent(myId);
                });
            } catch (error) {
                log.info("Error in Erizo Controller: ", error);
            }
        });
    }
})();

