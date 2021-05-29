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
        join_retry = spec.joinRetry || 60,
        keep_alive_period = 800/*MS*/,
        keep_alive_interval = undefined,
        on_join_ok = spec.onJoinOK || function () {log.debug('Join cluster successfully.');},
        on_join_failed = spec.onJoinFailed || function (reason) {log.debug('Join cluster failed. reason:', reason);},
        on_loss = spec.onLoss || function () {log.debug('Lost connection with cluster manager');},
        on_recovery = spec.onRecovery || function () {log.debug('Rejoin cluster successfully.');},
        on_overload = spec.onOverload || function () {log.debug('Overloaded!!');};;

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
                    if (result === 'initializing') {
                        tasks.length > 0 && pickUpTasks(tasks);
                    } else {
                        on_loss();
                        tasks = [];
                    }
                    on_success();
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
                    loss_count = 0;
                    if (result === 'whoareyou') {
                        if (state !== 'recovering') {
                            log.info('Unknown by cluster manager', cluster_name);
                            tryRecovery(function () {
                                log.info('Rejoin cluster', cluster_name, 'OK.');
                            });
                        }
                    }
                }, function (error_reason) {
                    loss_count += 1;
                    if (loss_count > 3) {
                        if (state !== 'recovering') {
                            log.info('Lost connection with cluster', cluster_name);
                            tryRecovery(function () {
                                log.info('Rejoin cluster', cluster_name, 'OK.');
                                on_recovery(id);
                            });
                        }
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
        if (state === 'registered') {
            if (keep_alive_interval) {
                clearInterval(keep_alive_interval);
                keep_alive_interval = undefined;
            }

            rpcClient.remoteCast(
                cluster_name,
                'quit',
                [id]);
        } else if (state === 'recovering') {
            keep_alive_interval && clearInterval(keep_alive_interval);
        }

        load_collector && load_collector.stop();
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

