// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;
var makeRPC = require('./makeRPC').makeRPC;

// Logger
var log = logger.getLogger('SipErizoHelper');

module.exports = function (spec) {

    var that = {},
        /*{ErizoId: {agent: AgentId, kaCount: number()}*/
        erizos = {};

    var cluster = spec.cluster,
        rpcClient = spec.rpcClient,
        on_erizo_broken = spec.on_broken;

    const enableGrpc = global.config.sip_portal?.enable_grpc || false;
    if (enableGrpc) {
        const { startClient } = require('./grpcTools');
        const grpcNode = {}; // remote => client
        cluster = global.config.cluster?.grpc_host || 'localhost:10080';
        // Reload makeRPC
        makeRPC = function (_, node, method, args, onOk, onError) {
            const startSipClientIfNeeded = function (node) {
                if (!grpcNode[node]) {
                    grpcNode[node] = startClient('sip', node);
                }
            };
            if (method === 'schedule') {
                if (!grpcNode[node]) {
                    grpcNode[node] = startClient('clusterManager', node);
                }
                const req = {
                    purpose: args[0],
                    task: args[1].task,
                    preference: {}, // Change data for some preference
                    reserveTime: 30 * 1000
                };
                grpcNode[node].schedule(req, (err, result) => {
                    if (err) {
                        log.debug('Schedule node error:', err);
                        onError(err);
                    } else {
                        const addr = result.info.ip + ':' + result.info.grpcPort;
                        onOk(addr);
                    }
                });
            } else if (method === 'getNode') {
                if (!grpcNode[node]) {
                    grpcNode[node] = startClient('nodeManager', node);
                }
                grpcNode[node].getNode({info: args[0]}, (err, result) => {
                    if (!err) {
                        onOk(result.message);
                    } else {
                        log.info('getNode error:', err);
                        onError(err);
                    }
                });
            } else if (method === 'recycleNode') {
                if (!grpcNode[node]) {
                    grpcNode[node] = startClient('nodeManager', node);
                }
                const req = {id: args[0], info: args[1]};
                grpcNode[node].recycleNode(req, (err, result) => {
                    if (!err) {
                        onOk(result.message);
                    } else {
                        onError(err);
                    }
                });
            } else if (method === 'init') {
                startSipClientIfNeeded(node);
                const req = {
                    room: args[0].room_id,
                    sipServer: args[0].sip_server,
                    sipUser: args[0].sip_user,
                    sipPassword: args[0].sip_passwd,
                };
                grpcNode[node].init(req, (err, result) => {
                    if (!err) {
                        onOk(result.message);
                    } else {
                        onError(err);
                    }
                });
            } else if (method === 'clean') {
                startSipClientIfNeeded(node);
                grpcNode[node].clean({}, (err, result) => {
                    if (!err) {
                        onOk(result.message);
                    } else {
                        onError(err);
                    }
                });
            } else if (method === 'keepAlive') {
                startSipClientIfNeeded(node);
                grpcNode[node].keepAlive({}, (err, result) => {
                    if (!err) {
                        onOk(true);
                    } else {
                        onError(err);
                    }
                });
            } else {
                log.warn('Unknown method:', method);
            }
        };
    }

    var callbackFor = function(erizo_id) {
        return function(ok) {
            if (erizos[erizo_id] !== undefined) {
                if (ok === true) {
                    erizos[erizo_id].kaCount = 0;
                } else {
                    log.debug('ErizoJS[', erizo_id, '] check alive timeout!');
                    erizos[erizo_id].kaCount += 1;
                }

                if (erizos[erizo_id].kaCount === 5) {
                    log.info('ErizoJS[', erizo_id, '] is no longer alive!');

                    var afterRecycle = function() {
                        if (erizos[erizo_id]) {
                            delete erizos[erizo_id];
                        }
                    };

                    makeRPC(rpcClient,
                            erizos[erizo_id].agent,
                            'recycleNode',
                            [erizo_id, erizos[erizo_id].for_whom],
                            afterRecycle,
                            afterRecycle);

                    on_erizo_broken(erizo_id);
                }
            }
        };
    };

    var KEEPALIVE_INTERVAL = 2*1000;
    var sendKeepAlive = function() {
        for (var erizo_id in erizos) {
            makeRPC(rpcClient,
                    erizo_id,
                    'keepAlive',
                    [],
                    callbackFor(erizo_id),
                    callbackFor(erizo_id));
        }
    };

    setInterval(sendKeepAlive, KEEPALIVE_INTERVAL);

    var getSipAgent = function (for_whom, on_ok, on_failed) {
        var keepTrying = true;

        var tryFetchingAgent = function (attempt) {
            if (attempt <= 0) {
                return on_failed('Failed in scheduling a sip agent.');
            }

            makeRPC(
                rpcClient,
                cluster,
                'schedule',
                ['sip', for_whom/*FIXME: use room_id as taskId temporarily, should use for_whom instead later.*/, 'preference'/*FIXME: should fill-in actual preference*/, 10 * 1000],
                function (result) {
                    on_ok({id: result});
                    keepTrying = false;
                }, function (reason) {
                    if (keepTrying) {
                        log.debug('Failed in scheduling a sip agent, keep trying.');
                        setTimeout(function () {tryFetchingAgent(attempt - ((typeof reason === 'string') && reason.startsWith('Timeout') ? 6 : 1));}, 200);
                    }
                });
        };

        tryFetchingAgent(25);
    };

    that.allocateSipErizo = function (for_whom, on_ok, on_failed) {
        getSipAgent(for_whom, function(result) {
            makeRPC(rpcClient,
                    result.id,
                    'getNode',
                    [for_whom],
                    function(erizo_id) {
                        erizos[erizo_id] = {agent: result.id, for_whom: for_whom, kaCount: 0};
                        log.info("Successully schedule sip node ", erizo_id, " for ", for_whom);
                        on_ok({id: erizo_id});
                    },
                    on_failed);
        }, function (reason) {
            on_failed('Failed to get sip agent:', reason);
        });
    };

    that.deallocateSipErizo = function (erizo_id) {
        if (!erizos[erizo_id]) {
            log.warn('Try to deallocate a non-existing sip node');
            return;
        }
        var for_whom = erizos[erizo_id].for_whom;
        makeRPC(rpcClient,
                erizos[erizo_id].agent,
                'recycleNode',
                [erizo_id, for_whom],
                function() {
                    delete erizos[erizo_id];
                }, function() {
                    delete erizos[erizo_id]
                });
    };

    that.initSipErizo = function (id, room, sip, on_ok, on_failed) {
        makeRPC(rpcClient, id, 'init',
            [{
                room_id: room,
                sip_server: sip.sip_server,
                sip_user: sip.sip_user,
                sip_passwd: sip.sip_passwd
            }],
            on_ok, on_failed);
    }

    that.cleanSipErizo = function (room) {
        makeRPC(rpcClient, erizos[room], 'clean');
    }

    return that;
};
