/*global require, exports, GLOBAL*/
'use strict';
var rpc = require('./rpc/rpc');
var log = require('./logger').logger.getLogger('CloudHandler');
var cluster_name = ((GLOBAL.config || {}).cluster || {}).name || 'woogeen-cluster';

exports.getErizoControllerForRoom = function (room, callback) {
    var roomId = room._id,
        keepTrying = true;

    var tryFetchingPortal = function (attempts) {
        if (attempts <= 0) {
            return callback('timeout');
        }

        rpc.callRpc(cluster_name, 'schedule', ['portal', roomId, 60 * 1000], {callback: function (result) {
            if (result === 'timeout' || result === 'error') {
                if (keepTrying) {
                    log.info('Faild in scheduling portal for', roomId, ', keep trying.');
                    setTimeout(function () {tryFetchingPortal(attempts - (result === 'timeout' ? 4 : 1));}, 1000);
                }
            } else {
                callback(result.info);
                keepTrying = false;
            }
        }});
    };

    tryFetchingPortal(60);
};

exports.getUsersInRoom = function (roomId, callback) {
    rpc.callRpc(cluster_name, 'getScheduled', ['portal', roomId], {callback: function (r) {
        if (r === 'timeout' || r === 'error') {
            callback([]);
        } else {
            rpc.callRpc(r, 'getUsersInRoom', [roomId], {callback: function (users) {
                if (users === 'timeout' || users === 'error') {
                    callback([]);
                } else {
                    callback(users);
                }
            }});
        }
    }});
};

exports.deleteRoom = function (roomId, callback) {
    rpc.callRpc(cluster_name, 'getScheduled', ['portal', roomId], {callback: function (r) {
        if (r === 'timeout' || r === 'error') {
            callback('Success');
        } else {
            rpc.callRpc(r, 'deleteRoom', [roomId], {callback: function (result) {
                callback(result);
            }});
        }
    }});
};

exports.deleteUser = function (user, roomId, callback) {
    rpc.callRpc(cluster_name, 'getScheduled', ['portal', roomId], {callback: function (r) {
        if (r === 'timeout' || r === 'error') {
            callback('Success');
        } else {
            rpc.callRpc(r, 'deleteUser', [{user: user, roomId: roomId}], {callback: function (result) {
                callback(result);
            }});
        }
    }});
};

exports.getPortals = function (callback) {
    rpc.callRpc(cluster_name, 'getWorkers', ['portal'], {callback: function (result) {
        if (result === 'timeout' || result === 'error') {
            callback([]);
        } else {
            var res = [];
            result.forEach(function (worker) {
                rpc.callRpc(cluster_name, 'getWorkerAttr', [worker], {callback: function (r) {
                    if (r === 'timeout' || r === 'error') {
                        res.push('error');
                    } else {
                        res.push(r);
                        if (res.length === result.length) {
                            callback(res.filter(function (e) { return e !== 'error';}));
                        }
                    }
                }});
            });
        }
    }});
};

exports.getPortal = function (node, callback) {
    rpc.callRpc(cluster_name, 'getWorkerAttr', [worker], {callback: function (r) {
        callback(r);
    }});
};

exports.getEcConfig = function getEcConfig (rpcID, callback) {
    rpc.callRpc(rpcID, 'getConfig', [], {callback: function (result) {
        callback(result);
    }});
};

exports.getHostedRooms = function getHostedRooms (callback) {
    rpc.callRpc(cluster_name, 'getWorkers', ['portal'], {callback: function (result) {
        if (result === 'timeout' || result === 'error') {
            callback([]);
        } else {
            var res = [], async_back = 0;
            result.forEach(function (worker) {
                rpc.callRpc(cluster_name, 'getTasks', [worker], {callback: function (r) {
                    async_back += 1;
                    if (!(r === 'timeout' || r === 'error')) {
                        r.forEach(function (t) {
                             res.push({id: t, ec: worker});
                        });
                    }
                    if (async_back === result.length) {
                        callback(res);
                    }
                }});
            });
        }
    }});
};


