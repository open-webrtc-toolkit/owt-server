/*global require, console, setInterval, clearInterval, exports*/
var rpc = require('./rpc/rpc');
var config = require('./../../etc/woogeen_config');
var logger = require('./logger').logger;
var nuveKey = require('./mdb/dataBase').nuveKey;

// Logger
var log = logger.getLogger("CloudHandler");

var ec2;

var INTERVAL_TIME_EC_READY = 100;
var INTERVAL_TIME_CHECK_KA = 1000;
var MAX_KA_COUNT = 10;

var erizoNodes = {};
var rooms = {}; // roomId: erizoControllerId
var ecQueue = [];
var eaTop = undefined;
var idIndex = 0;

var recalculateEcPriority = function () {
    "use strict";

    //*******************************************************************
    // States: 
    //  0: Not available
    //  1: Warning
    //  2: Available 
    //*******************************************************************

    var newQueue = [],
        available = 0,
        warnings = 0,
        ecea;

    for (ecea in erizoNodes) {
        if (erizoNodes.hasOwnProperty(ecea)) {
            if (erizoNodes[ecea].state === 2 && erizoNodes[ecea].purpose === 'reception') {
                newQueue.push(ecea);
                available += 1;
            }
        }
    }

    for (ecea in erizoNodes) {
        if (erizoNodes.hasOwnProperty(ecea)) {
            if (erizoNodes[ecea].state === 1 && erizoNodes[ecea].purpose === 'reception') {
                newQueue.push(ecea);
                warnings += 1;
            }
        }
    }

    ecQueue = newQueue;

    if (ecQueue.length === 0 || (available === 0 && warnings < 2)) {
        log.info('[CLOUD HANDLER]: Warning! No erizoController is available.');
    }
},

    recalculateEaPriority = function () {
        eaTop = undefined;

        for (ecea in erizoNodes) {
            if (erizoNodes.hasOwnProperty(ecea)) {
                if (erizoNodes[ecea].purpose === 'general-use') {
                    if (eaTop === undefined || (erizoNodes[ecea].load < erizoNodes[eaTop].load)) {
                        eaTop = ecea;
                    }
                }
            }
        }

        if (eaTop === undefined) {
            log.info('[CLOUD HANDLER]: Warning! No erizoAgent is available.');
        }
    },

    checkKA = function () {
        "use strict";
        var ecea, room;

        for (ecea in erizoNodes) {
            if (erizoNodes.hasOwnProperty(ecea)) {
                erizoNodes[ecea].keepAlive += 1;
                if (erizoNodes[ecea].keepAlive > MAX_KA_COUNT) {
                    log.info((erizoNodes[ecea].purpose === 'reception' ? 'ErizoController' : 'ErizoAgent'), ecea, ' in ', erizoNodes[ecea].ip, 'does not respond. Deleting it.');
                    delete erizoNodes[ecea];
                    for (room in rooms) {
                        if (rooms.hasOwnProperty(room)) {
                            if (rooms[room].ec === ecea) {
                                rooms[room].ec = undefined;
                            }
                            if (rooms[room].ea === ecea) {
                                rooms[room].ea = undefined;
                            }

                            if (rooms[room].ec === undefined && rooms[room].ea === undefined) {
                                delete rooms[room];
                            }
                        }
                    }
                    recalculateEcPriority();
                    recalculateEaPriority();
                }
            }
        }
    },

    checkKAInterval = setInterval(checkKA, INTERVAL_TIME_CHECK_KA);

exports.addNewErizoController = function (msg, callback) {
    "use strict";

    if (msg.cloudProvider === '') {
        addNewPrivateErizoController(msg.ip, msg.hostname, msg.port, msg.ssl, callback);
    } else if (msg.cloudProvider === 'amazon') {
        addNewAmazonErizoController(msg.ip, msg.hostname, msg.port, msg.ssl, callback);
    }
};

var addNewAmazonErizoController = function(privateIP, callback) {

    var publicIP;

    if (ec2 === undefined) {
        var opt = {version: '2012-12-01'};
        if (config.cloudProvider.host !== '') {
            opt.host = config.cloudProvider.host;
        }
        ec2 = require('aws-lib').createEC2Client(config.cloudProvider.accessKey, config.cloudProvider.secretAccessKey, opt);
    }
    log.info('private ip ', privateIP);

    ec2.call('DescribeInstances', {'Filter.1.Name':'private-ip-address', 'Filter.1.Value':privateIP}, function (err, response) {

        if (err) {
            log.info('Error: ', err);
            callback('error');
        } else if (response) {
            publicIP = response.reservationSet.item.instancesSet.item.ipAddress;
            log.info('public IP: ', publicIP);
            addNewPrivateErizoController(publicIP, hostname, port, ssl, callback);
        }
    });
}

var addNewPrivateErizoController = function (ip, hostname, port, ssl, callback) {
    "use strict";
    idIndex += 1;
    var id = idIndex,
        rpcID = 'erizoController_' + id;
    erizoNodes[id] = {
        ip: ip,
        purpose: 'reception',
        rpcID: rpcID,
        state: 2,
        keepAlive: 0,
        hostname: hostname,
        port: port,
        ssl: ssl
    };
    log.info('New erizocontroller (', id, ') in: ', erizoNodes[id].ip);
    recalculateEcPriority();
    callback({id: id, publicIP: ip, hostname: hostname, port: port, ssl: ssl});
};

exports.addNewErizoAgent = function (msg, callback) {
    "use strict";
    idIndex += 1;
    var id = idIndex,
        rpcID = 'ErizoAgent_' + id;
    erizoNodes[id] = {
        id: id,
        purpose: 'general-use',
        ip: msg.ip,
        rpcID: rpcID,
        state: 2,
        load: 0,
        keepAlive: 0
    };
    log.info('New erizoAgent (', id, ') in: ', erizoNodes[id].ip);
    recalculateEaPriority();
    callback({id: id, privateIP: msg.ip});
};

exports.keepAlive = function (id, callback) {
    "use strict";
    var result;

    if (erizoNodes[id] === undefined) {
        result = 'whoareyou';
        log.info('I received a keepAlive mess from a removed erizoController');
    } else {
        erizoNodes[id].keepAlive = 0;
        result = 'ok';
        //log.info('KA: ', id);
    }
    callback(result);
};

exports.setInfo = function (params) {
    "use strict";

    log.info('Received info ', params,    '.Recalculating erizoNodes priority');
    erizoNodes[params.id].state = params.state;
    recalculateEcPriority();
};

exports.killMe = function (ip) {
    "use strict";

    log.info('[CLOUD HANDLER]: ErizoController in host ', ip, 'does not respond.');

};

exports.getErizoControllerForRoom = function (roomId, callback) {
    "use strict";

    if (rooms[roomId] !== undefined && rooms[roomId].ec !== undefined) {
        callback(erizoNodes[rooms[roomId].ec]);
        return;
    }

    var id,
        intervarId = setInterval(function () {

            id = ecQueue[0];

            if (id !== undefined) {

                if (rooms[roomId] === undefined) {
                    rooms[roomId] = {};
                }

                rooms[roomId].ec = id;
                callback(erizoNodes[id]);

                recalculateEcPriority();
                clearInterval(intervarId);
            }

        }, INTERVAL_TIME_EC_READY);

};

exports.allocErizoAgent = function(msg, callback) {
    "use strict";

    var roomId = msg;
    if (rooms[roomId] !== undefined && rooms[roomId].ea !== undefined) {
        callback(erizoNodes[rooms[roomId].ea]);
        return;
    }

    var id,
        intervarId = setInterval(function () {

            id = eaTop;

            if (id !== undefined) {

                if (rooms[roomId] === undefined) {
                    rooms[roomId] = {};
                }

                rooms[roomId].ea = id;
                callback(erizoNodes[id]);

                erizoNodes[id].load += 1;

                recalculateEaPriority();
                clearInterval(intervarId);
            }

        }, INTERVAL_TIME_EC_READY);
};

exports.freeErizoAgent = function(msg, callback) {
    "use strict";

    var ea = msg;
    if (rooms[ea] !== undefined) {
        erizoNodes[ea].load -= 1;
        recalculateEaPriority();
    }

};

exports.getUsersInRoom = function (roomId, callback) {
    "use strict";

    if (rooms[roomId] === undefined || rooms[roomId].ec === undefined) {
        callback('error');
        return;
    }

    var rpcID = erizoNodes[rooms[roomId].ec].rpcID;
    rpc.callRpc(rpcID, 'getUsersInRoom', [roomId], {"callback": function (users) {
        if (users === 'timeout') {
            users = '?';
        }
        callback(users);
    }});
};

exports.deleteRoom = function (roomId, callback) {
    "use strict";

    if (rooms[roomId] === undefined || rooms[roomId].ec === undefined) {
        callback('Success');
        return;
    }

    var rpcID = erizoNodes[rooms[roomId].ec].rpcID;
    rpc.callRpc(rpcID, 'deleteRoom', [roomId], {"callback": function (result) {
        callback(result);
    }});
};

exports.deleteUser = function (user, roomId, callback) {
    "use strict";

    if (rooms[roomId] === undefined || rooms[roomId].ec === undefined) {
        callback('Success');
        return;
    }

    var rpcID = erizoNodes[rooms[roomId].ec].rpcID;
    rpc.callRpc(rpcID, 'deleteUser', [{user: user, roomId:roomId}], {"callback": function (result) {
        callback(result);
    }});
};

exports.getEcQueue = function getEcQueue (idx) {
    'use strict';
    if (arguments.length > 0) {
        return erizoNodes[idx];
    }
    return ecQueue.map(function (id) {
        return erizoNodes[id];
    });
};

exports.getEcConfig = function getEcConfig (rpcID, callback) {
    'use strict';
    rpc.callRpc(rpcID, 'getConfig', [], {callback: function (result) {
        callback(result);
    }});
};

exports.getHostedRooms = function getHostedRooms () {
    'use strict';
    return Object.keys(rooms).map(function (rid) {
        return {
            id: rid,
            ec: rooms[rid].ec,
            ea: rooms[rid].ea
        };
    });
};

exports.reschedule = function reschedule (id) {
    'use strict';
    delete rooms[id];
};

exports.getKey = function getKey (id) {
    'use strict';
    if (erizoNodes[id]) {
        return nuveKey;
    }
    return 'error';
}