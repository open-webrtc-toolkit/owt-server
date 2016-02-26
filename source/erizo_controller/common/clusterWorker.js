/*global require, setInterval, module*/
'use strict';

var logger = require('./logger').logger;
var makeRPC = require('./makeRPC').makeRPC;

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
               //FIXME: uncomment the following segments if stronger randomicity is required.
               //+ '-' + s4()
               //+ '-' + s4()
               //+ '-' + s4()
               + '-' + s4() + s4() + s4();
    };
})();

module.exports.ClusterWorker = function (spec) {
    var that = {};

    /*'unregistered' | 'registered' | 'recovering'*/
    var state = 'unregistered',
        tasks = [];

    var amqper = spec.amqper,
        id = spec.purpose + '-' + genID(),
        purpose = spec.purpose,
        info = spec.info,
        cluster_name = spec.clusterName || 'woogeenCluster',
        join_retry = spec.joinRetry || 5,
        join_period = spec.joinPeriod || 3000/*MS*/,
        recovery_period = spec.recoveryPeriod || 1000/*MS*/,
        keep_alive_period = spec.keepAlivePeriod || 5000/*MS*/,
        keep_alive_interval = undefined,
        on_join_ok = spec.onJoinOK || function () {log.debug('Join cluster successfully.');},
        on_join_failed = spec.onJoinFailed || function (reason) {log.debug('Join cluster failed. reason:', reason);},
        on_loss = spec.onLoss || function () {log.debug('Lost connection with cluster manager');},
        on_recovery = spec.onRecovery || function () {log.debug('Rejoin cluster successfully.');};

    var join = function (on_ok, on_failed) {
        makeRPC(
            amqper,
            cluster_name,
            'join',
            [purpose, id, info],
            function () {
                state = 'registered';
                on_ok();
                keepAlive();
            }, on_failed);
    };

    var joinCluster = function (attempt, period) {
        var error = 'Unknown reason';

        var tryJoin = function (countDown) {
            if (countDown <= 0) {
                on_join_failed(error);
                return;
            }

            log.debug('Try joining cluster:', cluster_name);
            join(function () {
                    on_join_ok(id);
                    log.info('Join cluster', cluster_name, 'OK.');
                }, function (error_reason) {
                    error = error_reason;
                    state === 'unregistered' && log.info('Join cluster', cluster_name, 'failed. try again.');
                    setTimeout(function () {tryJoin(countDown - 1);}, period);
                });
        };
        tryJoin(attempt);
    };

    var keepAlive = function () {
        keep_alive_interval && clearInterval(keep_alive_interval);

        var tryRecovery = function () {
            clearInterval(keep_alive_interval);
            keep_alive_interval = undefined;
            state = 'recovering';

            on_loss();

            log.info('Lost connection with cluster', cluster_name, '. Try recovering.');
            var interval = setInterval(function () {
                join(function () {
                    clearInterval(interval);
                    tasks.length > 0 && pickUpTasks(tasks);
                    log.info('Rejoin cluster', cluster_name, 'OK.');
                    on_recovery(id);
                }, function (reason) {
                    if (state === 'recovering') {
                        log.debug('Rejoin cluster', cluster_name, 'failed. reason:', reason);
                    }
                });
            }, recovery_period);
        };

        keep_alive_interval = setInterval(function () {
            makeRPC(
                amqper,
                cluster_name,
                'keepAlive',
                [id],
                function (result) {
                    if (result === 'whoareyou') {
                        log.info('Unknown by cluster manager.');
                        (state !== 'recovering') && tryRecovery();
                    }
                }, function (error_reason) {
                    log.info('keep alive error:', error_reason);
                    (state !== 'recovering') && tryRecovery();
                });
        }, keep_alive_period);
    };

    var pickUpTasks = function (taskList) {
        makeRPC(
            amqper,
            cluster_name,
            'pickUpTasks',
            [id, taskList]);
    };

    var layDownTask = function (task) {
        makeRPC(
            amqper,
            cluster_name,
            'layDownTask',
            [id, task]);
    };

    var doRejectTask = function (task) {
        makeRPC(
            amqper,
            cluster_name,
            'unschedule',
            [id, task]);
    };

    that.quit = function () {
        if (state === 'registerd') {
            if (keep_alive_interval) {
                clearInterval(keep_alive_interval);
                keep_alive_interval = undefined;
            }

            makeRPC(
                amqper,
                cluster_name,
                'quit',
                [id]);
        } else if (state === 'recovering') {
            keep_alive_interval && clearInterval(keep_alive_interval);
        }
    };

    that.reportState = function (state) {
        if (state === 'registered') {
            makeRPC(
                amqper,
                cluster_name,
                'reportState',
                [id, state]);
        }
    };

    that.reportLoad = function (load) {
        if (state === 'registered') {
            makeRPC(
                amqper,
                cluster_name,
                'reportLoad',
                [id, load]);
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

    joinCluster(join_retry, join_period);
    return that;
};

