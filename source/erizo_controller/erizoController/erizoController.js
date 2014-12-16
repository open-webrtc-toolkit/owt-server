/*global require, logger. setInterval, clearInterval, Buffer, exports*/
var crypto = require('crypto');
var rpcPublic = require('./rpc/rpcPublic');
var ST = require('./Stream');
var http = require('http');
var server = http.createServer();
var io = require('socket.io').listen(server, {log:false});
var config = require('./../../etc/licode_config');
var Permission = require('./permission');
var Getopt = require('node-getopt');

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
GLOBAL.config.erizoController.warning_n_rooms = GLOBAL.config.erizoController.warning_n_rooms || 15;
GLOBAL.config.erizoController.limit_n_rooms = GLOBAL.config.erizoController.limit_n_rooms || 20;
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

server.listen(8080);

io.set('log level', 0);

var nuveKey = GLOBAL.config.nuve.superserviceKey;

var WARNING_N_ROOMS = GLOBAL.config.erizoController.warning_n_rooms;
var LIMIT_N_ROOMS = GLOBAL.config.erizoController.limit_n_rooms;

var INTERVAL_TIME_KEEPALIVE = GLOBAL.config.erizoController.interval_time_keepAlive;

var BINDED_INTERFACE_NAME = GLOBAL.config.erizoController.networkInterface;

var myId;
var rooms = {};
var myState;

var calculateSignature = function (token, key) {
    "use strict";

    var toSign = token.tokenId + ',' + token.host,
        signed = crypto.createHmac('sha1', key).update(toSign).digest('hex');
    return (new Buffer(signed)).toString('base64');
};

var checkSignature = function (token, key) {
    "use strict";

    var calculatedSignature = calculateSignature(token, key);

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

            callback("callback");

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
    } else if (nRooms > LIMIT_N_ROOMS) {
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

var listen = function () {
    "use strict";

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

            if (checkSignature(token, nuveKey)) {

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
                        tokenDB = resp;
                        if (rooms[tokenDB.room] === undefined) {
                            var room = {};

                            room.id = tokenDB.room;
                            room.sockets = [];
                            room.sockets.push(socket.id);
                            room.streams = {}; //streamId: Stream
                            if (tokenDB.p2p) {
                                log.debug('Token of p2p room');
                                room.p2p = true;
                            } else {
                                room.controller = controller.RoomController({rpc: rpc});
                                room.controller.addEventListener(function(type, event) {
                                    // TODO Send message to room? Handle ErizoJS disconnection.
                                    if (type === "unpublish") {
                                        var streamId = parseInt(event); // It's supposed to be an integer.
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
                                    }

                                });

                                if (GLOBAL.config.erizoController.mixer) {
                                    // In case we need to do an RPC call to initialize the mixer when the first
                                    // client is connected, we should wait for a while because the room may still
                                    // be dirty at this moment (due to the fact that there's currently no inter
                                    // process synchronization for room deletion). This could happen when the
                                    // last user in the room refreshes the web page.
                                    // TODO: Revisit here for a better solution.
                                    var tryOut = 10;
                                    room.initMixerTimer = setInterval(function() {
                                        var id = 0;
                                        room.controller.initMixer(id, function (result) {
                                            if (result === 'success') {
                                                var st = new ST.Stream({id: id, socket: socket.id, audio: true, video: {category: 'mix'}, data: false});
                                                room.streams[id] = st;
                                                sendMsgToRoom(room, 'onAddStream', st.getPublicStream());
                                                clearInterval(room.initMixerTimer);
                                            } else if (--tryOut === 0) {
                                                log.info('Mixer initialization failed in Room ', room.id);
                                                clearInterval(room.initMixerTimer);
                                            }
                                        });
                                    }, 100);
                                }

                            }
                            rooms[tokenDB.room] = room;
                            updateMyState();
                        } else {
                            rooms[tokenDB.room].sockets.push(socket.id);
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
                        sendMsgToOthersInRoom(socket.room, 'onPeerJoin', {user: user});

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
                if (receiver === 0) {
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
                return safeCall(callback, 'success');
            }
        });

        //Gets 'sendDataStream' messages on the socket in order to write a message in a dataStream.
        socket.on('sendDataStream', function (msg) {
            var sockets = socket.room.streams[msg.id].getDataSubscribers(), id;
            for (id in sockets) {
                if (sockets.hasOwnProperty(id)) {
                    log.info('Sending dataStream to', sockets[id], 'in stream ', msg.id);
                    io.sockets.socket(sockets[id]).emit('onDataStream', msg);
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
                        st = new ST.Stream({id: id, socket: socket.id, audio: options.audio, video: options.video, data: options.data, attributes: options.attributes});
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
                    if (GLOBAL.config.erizoController.sendStats) {
                        var timeStamp = new Date();
                        rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'publish', stream: id, timestamp: timeStamp.getTime()}]);
                    }
                    socket.room.controller.addPublisher(id, sdp, function (answer) {
                        socket.state = 'waitingOk';
                        answer = answer.replace(privateRegexp, publicIP);
                        safeCall(callback, answer, id);
                    }, function() {
                        // Double check if this socket is still in the room.
                        // It can be removed from the room if the socket is disconnected
                        // before the publish succeeds.
                        var index = socket.room.sockets.indexOf(socket.id);
                        if (index === -1) {
                            socket.room.controller.removePublisher(id);
                            if (GLOBAL.config.erizoController.sendStats) {
                                var timeStamp = new Date();
                                rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'unpublish', stream: id, timestamp: timeStamp.getTime()}]);
                            }
                            return;
                        }

                        var hasScreen = false;
                        if (options.video && options.video.device === 'screen') {
                            hasScreen = true;
                        }
                        st = new ST.Stream({id: id, audio: options.audio, video: options.video, data: options.data, screen: hasScreen, attributes: options.attributes});
                        socket.state = 'sleeping';
                        socket.streams.push(id);
                        socket.room.streams[id] = st;

                        if (socket.room.streams[id] !== undefined) {
                            sendMsgToRoom(socket.room, 'onAddStream', socket.room.streams[id].getPublicStream());
                        }
                    });

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
                st = new ST.Stream({id: id, socket: socket.id, audio: options.audio, video: options.video, data: options.data, screen: hasScreen, attributes: options.attributes});
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
                    socket.room.controller.addSubscriber(socket.id, options.streamId, options.audio, options.video, sdp, function (answer) {
                        answer = answer.replace(privateRegexp, publicIP);
                        safeCall(callback, answer);
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
            if (socket.user === undefined || !socket.user.permissions[Permission.RECORD]) {
                return safeCall(callback, 'error', 'unauthorized');
            }
            var streamId = options.to;
            var recordingId = Math.random() * 1000000000000000000;
            var url;

            if (GLOBAL.config.erizoController.recording_path) {
                url = GLOBAL.config.erizoController.recording_path + recordingId + '.mkv';
            } else {
                url = '/tmp/' + recordingId + '.mkv';
            }

            log.info("erizoController.js: Starting recorder streamID " + streamId + "url ", url);

            if (socket.room.streams[streamId].hasAudio() || socket.room.streams[streamId].hasVideo() || socket.room.streams[streamId].hasScreen()) {
                socket.room.controller.addExternalOutput(streamId, url, function (result) {
                    if (result === 'success') {
                        log.info("erizoController.js: Recorder Started");
                        safeCall(callback, 'success', recordingId);
                    } else {
                        safeCall(callback, 'error', 'This stream is not published in this room');
                    }
                });

            } else {
                safeCall(callback, 'error', 'Stream can not be recorded');
            }
        });

        socket.on('stopRecorder', function (options, callback) {
            if (socket.user === undefined || !socket.user.permissions[Permission.RECORD]) {
                return safeCall(callback, 'error', 'unauthorized');
            }
            var recordingId = options.id;
            var url;

            if (GLOBAL.config.erizoController.recording_path) {
                url = GLOBAL.config.erizoController.recording_path + recordingId + '.mkv';
            } else {
                url = '/tmp/' + recordingId + '.mkv';
            }

            log.info("erizoController.js: Stoping recording  " + recordingId + " url " + url);
            socket.room.controller.removeExternalOutput(url, callback);
        });

        //Gets 'unpublish' messages on the socket in order to remove a stream from the room.
        socket.on('unpublish', function (streamId, callback) {
            if (socket.user === undefined || !socket.user.permissions[Permission.PUBLISH]) {
                return safeCall(callback, 'error', 'unauthorized');
            }

            // Stream has been already deleted or it does not exist
            if (socket.room.streams[streamId] === undefined) {
                return safeCall(callback, 'error', 'stream does not exist');
            }
            var i, index;

            sendMsgToRoom(socket.room, 'onRemoveStream', {id: streamId});

            if (socket.room.streams[streamId].hasAudio() || socket.room.streams[streamId].hasVideo() || socket.room.streams[streamId].hasScreen()) {
                socket.state = 'sleeping';
                if (!socket.room.p2p) {
                    socket.room.controller.removePublisher(streamId);
                    if (GLOBAL.config.erizoController.sendStats) {
                        var timeStamp = new Date();
                        rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'unpublish', stream: streamId, timestamp: timeStamp.getTime()}]);
                    }
                }
            }

            index = socket.streams.indexOf(streamId);
            if (index !== -1) {
                socket.streams.splice(index, 1);
            }
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

                        if (socket.room.streams[id].hasAudio() || socket.room.streams[id].hasVideo() || socket.room.streams[id].hasScreen()) {
                            if (!socket.room.p2p) {
                                socket.room.controller.removePublisher(id);
                                if (GLOBAL.config.erizoController.sendStats) {
                                    var timeStamp = new Date();
                                    rpc.callRpc('stats_handler', 'event', [{room: socket.room.id, user: socket.id, type: 'unpublish', stream: id, timestamp: timeStamp.getTime()}]);
                                }
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
                    clearInterval(socket.room.initMixerTimer);
                    // FIXME: Don't hard code the mixer id.
                    if (socket.room.controller && typeof socket.room.controller.removePublisher === 'function') {
                        socket.room.controller.removePublisher(0);
                    }
                    if (socket.room.streams[0]) {
                        delete socket.room.streams[0];
                    }
                }
                delete rooms[socket.room.id];
                updateMyState();
            } else if (socket.room !== undefined) {
                sendMsgToOthersInRoom(socket.room, 'onPeerLeave', {user: socket.user});
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
        return safeCall(callback, users);
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
exports.deleteRoom = function (room, callback) {
    "use strict";

    var sockets, streams, id, j;

    log.info('Deleting room ', room);

    if (rooms[room] === undefined) {
        return safeCall(callback, 'Success');
    }
    sockets = rooms[room].sockets;

    for (id in sockets) {
        if (sockets.hasOwnProperty(id)) {
            rooms[room].roomController.removeSubscriptions(sockets[id]);
        }
    }

    streams = rooms[room].streams;

    for (j in streams) {
        if (streams[j].hasAudio() || streams[j].hasVideo() || streams[j].hasScreen()) {
            if (!room.p2p) {
                rooms[room].roomController.removePublisher(j);
            }
        }
    }

    delete rooms[room];
    updateMyState();
    log.info('Deleted room ', room, rooms);
    safeCall(callback, 'Success');
};
rpc.connect(function () {
    "use strict";
    try {
        rpc.setPublicRPC(rpcPublic);

        addToCloudHandler(function () {
            var rpcID = 'erizoController_' + myId;

            rpc.bind(rpcID, listen);

        });
    } catch (error) {
        log.info("Error in Erizo Controller: ", error);
    }
});
