// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;
var makeRPC = require('./makeRPC').makeRPC;
var loadCollector = require('./loadCollector').LoadCollector;

// Logger
var log = logger.getLogger('ClusterWorker');

var genID = (function() {
    function s4() {
        return Math.floor((1 + Math.random()) * 0x10000)
                   .toString(16)
                   .substring(1);
    }
    return function() {
        return s4() + s4()
               /*+ '-' + s4()
               + '-' + s4()
               + '-' + s4()
               + '-'*/ + s4() + s4() + s4();
    };
})();

/*
 * @param {object} rpcClient RPC client with remoteCast method.
 * @param {string} purpose Purpose of this worker.
 * @param {string} clusterName RPC ID of the cluster manager.
 * @param {number} joinRetry retry times for join.
 * @param {object} info Worker info for join.
 * @param {function} onJoinOK join ok callback.
 * @param {function} onJoinFailed join failed callback.
 * @param {function} onLoss loss callback.
 * @param {function} onRecovery recovery callback.
 * @param {function} onOverload overload callback.
 * @param {object} loadCollection load collection for worker.
 */
module.exports = function (spec) {
    var that = {};

    /*'unregistered' | 'registered' | 'recovering'*/
    var state = 'unregistered',
        tasks = [];

    var rpcClient = spec.rpcClient,
        id = spec.purpose + '-' + genID() + '@' + (spec.info.hostname || spec.info.ip),
        purpose = spec.purpose,
        info = spec.info,
        cluster_name = spec.clusterName || 'owt-cluster',
        cluster_host = spec.clusterHost || 'localhost:10080',
        join_retry = spec.joinRetry || 60,
        keep_alive_period = 800/*MS*/,
        keep_alive_interval = undefined,
        on_join_ok = spec.onJoinOK || function () {log.debug('Join cluster successfully.');},
        on_join_failed = spec.onJoinFailed || function (reason) {log.debug('Join cluster failed. reason:', reason);},
        on_loss = spec.onLoss || function () {log.debug('Lost connection with cluster manager');},
        on_recovery = spec.onRecovery || function () {log.debug('Rejoin cluster successfully.');},
        on_overload = spec.onOverload || function () {log.debug('Overloaded!!');};;

    if (spec.info.grpcPort) {
        // Enable gRPC
        log.info('ClusterManager gRPC address:', cluster_host);
        const grpcTools = require('./grpcTools');
        const grpcClient = grpcTools.startClient('clusterManager', cluster_host);
        const GRPC_TIMEOUT = 2000;
        const opt = () => ({deadline: new Date(Date.now() + GRPC_TIMEOUT)});

        // Client for gRPC
        rpcClient = {
            remoteCast: function (name, method, args) {
                if (method === 'reportLoad') {
                    const req = {
                        id: args[0],
                        load: args[1]
                    };
                    grpcClient.reportLoad(req, opt(), (err) => {
                        if (err) {
                            log.info('Failed to reportLoad', err);
                        }
                    });
                } else if (method === 'reportState') {
                    const req = {
                        id: args[0],
                        state: args[1]
                    };
                    grpcClient.reportState(req, opt(), (err) => {
                        if (err) {
                            log.info('Failed to reportState', err);
                        }
                    });
                } else if (method === 'pickUpTasks') {
                    const req = {
                        id: args[0],
                        tasks: args[1]
                    };
                    grpcClient.pickUpTasks(req, opt(), (err) => {
                        if (err) {
                            log.info('Failed to pickUpTasks', err);
                        }
                    });
                } else if (method === 'layDownTask') {
                    const req = {
                        id: args[0],
                        task: args[1]
                    };
                    grpcClient.layDownTask(req, opt(), (err) => {
                        if (err) {
                            log.info('Failed to layDownTask', err);
                        }
                    });
                } else if (method === 'unschedule') {
                    const req = {
                        id: args[0],
                        task: args[1]
                    };
                    grpcClient.unschedule(req, opt(), (err) => {
                        if (err) {
                            log.info('Failed to unschedule', err);
                        }
                    });
                } else if (method === 'quit') {
                    const req = {id: args[0]};
                    grpcClient.quit(req, opt(), (err) => {
                        if (err) {
                            log.info('Failed to quit', err);
                        }
                    });
                }
            }
        };
        // Function makeRPC for gRPC
        makeRPC = function (client, remote, method, args, onOk, onError) {
            if (method === 'join') {
                const req = {
                    purpose: args[0],
                    id: args[1],
                    info: args[2]
                };
                grpcClient.join(req, opt(), (err, result) => {
                    if (err) {
                        onError(err);
                    } else {
                        onOk(result.state)
                    }
                });
            } else if (method === 'quit') {
                const req = {
                    id: args[0]
                };
                grpcClient.quit(req, opt(), (err, result) => {
                    if (err) {
                        onError(err);
                    } else {
                        onOk()
                    }
                });
            } else if (method === 'keepAlive') {
                grpcClient.keepAlive({id: args[0]}, opt(), (err, result) => {
                    if (err) {
                        onError(err);
                    } else {
                        onOk(result.message)
                    }
                });
            } else if (method === 'getScheduled') {
                const req = {
                    purpose: args[0],
                    task: args[1]
                };
                grpcClient.getScheduled(req, opt(), (err, result) => {
                    if (err) {
                        onError(err);
                    } else {
                        const workerId = result.message;
                        grpcClient.getWorkerAttr({id: workerId}, opt(), (err, result) => {
                            log.info('getWorkerAtt:', result);
                            if (err) {
                                log.info('getWorkerAttr error:', err);
                                onError(err);
                            } else {
                                const addr = result.info.ip + ':' + result.info.grpcPort;
                                onOk(addr);
                            }
                        });
                    }
                });
            }
        }
    }

    var previous_load = 0.99;
    var reportLoad = function (load) {
        if (load == previous_load) {
            return;
        }
        previous_load = load;
        if (state === 'registered') {
            rpcClient.remoteCast(
                cluster_name,
                'reportLoad',
                [id, load]);
        }

        if (load > 0.98/*FIXME: Introduce a configuration item to specify the overload threshold here.*/) {
           on_overload();
        }
    };

    var load_collector = loadCollector({period: spec.loadCollection.period,
                                        item: spec.loadCollection.item,
                                        onLoad: reportLoad});

    var join = function (on_ok, on_failed) {
        makeRPC(
            rpcClient,
            cluster_name,
            'join',
            [purpose, id, info],
            function (result) {
                state = 'registered';
                on_ok(result);
                previous_load = 0.99;
                keepAlive();
            }, on_failed);
    };

    var joinCluster = function (attempt) {
        var countDown = attempt;

        var tryJoin = function (countDown) {
            log.debug('Try joining cluster', cluster_name, ', retry count:', attempt - countDown);
            join(function (result) {
                on_join_ok(id);
                log.info('Join cluster', cluster_name, 'OK.');
            }, function (error_reason) {
                if (state === 'unregistered') {
                    log.info('Join cluster', cluster_name, 'failed.');
                    if (countDown <= 0) {
                        log.error('Join cluster', cluster_name, 'failed. reason:', error_reason);
                        on_join_failed(error_reason);
                    } else {
                        setTimeout(function () {
                            tryJoin(countDown - 1);
                        }, keep_alive_period);
                    }
                }
            });
        };

        tryJoin(attempt);
    };

    var keepAlive = function () {
        keep_alive_interval && clearInterval(keep_alive_interval);

        var tryRecovery = function (on_success) {
            clearInterval(keep_alive_interval);
            keep_alive_interval = undefined;
            state = 'recovering';

            var tryJoining = function () {
                log.debug('Try rejoining cluster', cluster_name, '....');
                join(function (result) {
                    log.debug('Rejoining result', result);
                    let rejoinPromise = tasks.map((taskId)=>{
                        return new Promise((resolve)=>{
                            makeRPC(
                                rpcClient,
                                cluster_name,
                                'getScheduled',
                                [purpose, taskId],
                                (result)=>{
                                    //keycoding 20230811: FIXME the task was scheduled to another agent should be delete from this agent
                                    log.error('Rejoin cluster and pickUpTasks', taskId, 'failed. reason: has old scheduled:',result);
                                    resolve();
                                },
                                (reason)=>{
                                    if(reason.indexOf('NO_SUCH_TASK')!==-1){
                                        log.info('Rejoin cluster and pickUpTasks', taskId, 'success.');
                                        pickUpTasks([taskId]);
                                    }
                                    resolve();
                                }
                            );
                        });
                    });
                    Promise.all(rejoinPromise).then(()=>{
                        on_success();
                    });
                }, function (reason) {
                    setTimeout(function() {
                        if (state === 'recovering') {
                            log.debug('Rejoin cluster', cluster_name, 'failed. reason:', reason);
                            tryJoining();
                        }
                    }, keep_alive_period);
                });
            };

            tryJoining();
        };

        var loss_count = 0;
        keep_alive_interval = setInterval(function () {
            makeRPC(
                rpcClient,
                cluster_name,
                'keepAlive',
                [id],
                function (result) {
                    //keycoding 20230811: fix when a newer leader was vote,the agent must pickUp itself
                    if (result === 'whoareyou') {
                        loss_count += 1;
                        if (loss_count % 300 === 0 || loss_count === 3) {
                            log.info(`Agent ${id} is not alive any longer(${loss_count}), error_reason: ${result}`);
                        }
                        if (state !== 'recovering') {
                            log.info('Unknown by cluster manager', cluster_name,"loss_count:",loss_count);
                            tryRecovery(function () {
                                log.info('Rejoin cluster', cluster_name, 'OK.');
                                on_recovery(id);
                            });
                        }
                    }else{
                        if (loss_count >= 3) {
                            log.info(`Agent ${id} is recover(${loss_count})`);
                        }
                        loss_count = 0;
                    }
                }, function (error_reason) {
                    loss_count += 1;
                    if (loss_count % 300 === 0 || loss_count === 3) {
                        log.info(`Agent ${id} is not alive any longer(${loss_count}), error_reason: ${error_reason}`);
                    }
                });
        }, keep_alive_period);
    };

    var pickUpTasks = function (taskList) {
        rpcClient.remoteCast(
            cluster_name,
            'pickUpTasks',
            [id, taskList]);
    };

    var layDownTask = function (task) {
        rpcClient.remoteCast(
            cluster_name,
            'layDownTask',
            [id, task]);
    };

    var doRejectTask = function (task) {
        rpcClient.remoteCast(
            cluster_name,
            'unschedule',
            [id, task]);
    };

    that.quit = function () {
        if (keep_alive_interval) {
            clearInterval(keep_alive_interval);
            keep_alive_interval = undefined;
            load_collector && load_collector.stop();
            load_collector = null;
            return new Promise((resolve)=>{
                makeRPC(
                    rpcClient,
                    cluster_name,
                    'quit',
                    [id],
                    resolve,
                    resolve
                );
            });
        }
    };

    that.reportState = function (st) {
        if (state === 'registered') {
            rpcClient.remoteCast(
                cluster_name,
                'reportState',
                [id, st]);
        }
    };

    that.addTask = function (task) {
        var i = tasks.indexOf(task);
        if (i === -1) {
            tasks.push(task);
            if (state === 'registered') {
                pickUpTasks([task]);
            }
        }
    };

    that.removeTask = function (task) {
        var i = tasks.indexOf(task);
        if (i !== -1) {
            tasks.splice(i, 1);
            if (state === 'registered') {
                layDownTask(task);
            }
        }
    };

    that.rejectTask = function (task) {
        doRejectTask(task);
    };

    joinCluster(join_retry);
    return that;
};

