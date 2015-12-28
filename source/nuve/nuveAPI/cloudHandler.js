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
var nodesQueue = [];
var idIndex = 0;

var recalculatePriority = function () {
    "use strict";

    //*******************************************************************
    // States: 
    //  0: Not available
    //  1: Warning
    //  2: Available 
    //*******************************************************************

    var newNodesQueue = [],
        available = 0,
        warnings = 0,
        ec;

    for (ec in erizoNodes) {
        if (erizoNodes.hasOwnProperty(ec)) {
            if (erizoNodes[ec].state === 2) {
                newNodesQueue.push(ec);
                available += 1;
            }
        }
    }

    for (ec in erizoNodes) {
        if (erizoNodes.hasOwnProperty(ec)) {
            if (erizoNodes[ec].state === 1) {
                newNodesQueue.push(ec);
                warnings += 1;
            }
        }
    }

    nodesQueue = newNodesQueue;

    if (nodesQueue.length === 0 || (available === 0 && warnings < 2)) {
        log.info('[CLOUD HANDLER]: Warning! No erizoNode is available.');
    }

},

    checkKA = function () {
        "use strict";
        var ec, room;

        for (ec in erizoNodes) {
            if (erizoNodes.hasOwnProperty(ec)) {
                erizoNodes[ec].keepAlive += 1;
                if (erizoNodes[ec].keepAlive > MAX_KA_COUNT) {
                    log.info('ErizoNode', ec, ' in ', erizoNodes[ec].ip, 'does not respond. Deleting it.');
                    delete erizoNodes[ec];
                    for (room in rooms) {
                        if (rooms.hasOwnProperty(room)) {
                            if (rooms[room] === ec) {
                                delete rooms[room];
                            }
                        }
                    }
                    recalculatePriority();
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
        purpose: 'reception',
        ip: ip,
        rpcID: rpcID,
        state: 2,
        keepAlive: 0,
        hostname: hostname,
        port: port,
        ssl: ssl
    };
    log.info('New erizoController (', id, ') in: ', erizoNodes[id].ip);
    recalculatePriority();
    callback({id: id, publicIP: ip, hostname: hostname, port: port, ssl: ssl});
};

exports.addNewErizoAgent = function (msg, callback) {
    "use strict";
    idIndex += 1;
    var id = idIndex,
        rpcID = 'ErizoAgent_' + id;
    erizoNodes[id] = {
        id: id,
        purpose: msg.purpose,
        ip: msg.ip,
        rpcID: rpcID,
        state: 2,
        load: 0,
        keepAlive: 0
    };
    log.info('New erizoAgent (', id, ') in: ', erizoNodes[id].ip);
    recalculatePriority();
    callback({id: id, privateIP: msg.ip});
};

exports.keepAlive = function (id, callback) {
    "use strict";
    var result;

    if (erizoNodes[id] === undefined) {
        result = 'whoareyou';
        log.info('I received a keepAlive mess from a removed erizoNode');
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
    erizoNodes[params.id].state = params.state || erizoNodes[params.id].state;
    if (params.load !== undefined) {
        erizoNodes[params.id].load = params.load;
    }
    recalculatePriority();
};

exports.killMe = function (ip) {
    "use strict";

    log.info('[CLOUD HANDLER]: ErizoNode in host ', ip, 'does not respond.');

};

exports.getErizoControllerForRoom = function (roomId, callback) {
    "use strict";

    if (rooms[roomId] !== undefined) {
        callback(erizoNodes[rooms[roomId]]);
        return;
    }

    var id,
        intervarId = setInterval(function () {

            id = undefined;
            for (var i in nodesQueue) {
                var temp_id = nodesQueue[i];

                if (erizoNodes[temp_id].purpose === 'reception') {
                    id = temp_id;
                    break;
                }
            }

            if (id !== undefined) {

                rooms[roomId] = id;
                callback(erizoNodes[id]);

                recalculatePriority();
                clearInterval(intervarId);
            }

        }, INTERVAL_TIME_EC_READY);

};

exports.getErizoAgent = function(msg, callback) {
    "use strict";
    var id,
        intervarId = setInterval(function () {

            id = undefined;
            for (var i in nodesQueue) {
                var temp_id = nodesQueue[i];
                if ((erizoNodes[temp_id].purpose === msg.purpose)
                    && ((id === undefined) || (erizoNodes[temp_id].load < erizoNodes[id].load))) {
                        id = temp_id;
                }
            }

            if (id !== undefined) {
                callback(erizoNodes[id]);
                clearInterval(intervarId);
            }

        }, INTERVAL_TIME_EC_READY);
};

exports.getUsersInRoom = function (roomId, callback) {
    "use strict";

    if (rooms[roomId] === undefined) {
        callback('error');
        return;
    }

    var rpcID = erizoNodes[rooms[roomId]].rpcID;
    rpc.callRpc(rpcID, 'getUsersInRoom', [roomId], {"callback": function (users) {
        if (users === 'timeout') {
            users = '?';
        }
        callback(users);
    }});
};

exports.deleteRoom = function (roomId, callback) {
    "use strict";

    if (rooms[roomId] === undefined) {
        callback('Success');
        return;
    }

    var rpcID = erizoNodes[rooms[roomId]].rpcID;
    rpc.callRpc(rpcID, 'deleteRoom', [roomId], {"callback": function (result) {
        callback(result);
    }});
};

exports.deleteUser = function (user, roomId, callback) {
    "use strict";

    var rpcID = erizoNodes[rooms[roomId]].rpcID;
    rpc.callRpc(rpcID, 'deleteUser', [{user: user, roomId:roomId}], {"callback": function (result) {
        callback(result);
    }});
};

exports.getEcQueue = function getEcQueue (idx) {
    'use strict';
    if (arguments.length > 0) {
        return erizoNodes[idx];
    }
    return nodesQueue.map(function (id) {
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
            ec: rooms[rid]
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