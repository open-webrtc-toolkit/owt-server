/*global require, setTimeout, setInterval, clearInterval, exports*/
'use strict';
var logger = require('./logger').logger;
var Scheduler = require('./scheduler').Scheduler;

// Logger
var log = logger.getLogger('ClusterManager');

var ClusterManager = function (spec) {
    var that = {};

    /*initializing | in-service*/
    var state = 'initializing',

        initial_time = spec.initialTime,
        check_alive_period = spec.checkAlivePeriod,
        check_alive_count = spec.checkAliveCount;

    /* {Purpose: Scheduler}*/
    var schedulers = {};

    /*Id : {scheduler: Scheduler, alive_count: Number, info: Info}*/
    var workers = {};

    var init = function () {
        setTimeout(function () {state = 'in-service';}, initial_time);
        setInterval(checkAlive, check_alive_period);
    };

    var createScheduler = function (purpose) {
        var strategy = spec.hasOwnProperty(purpose + 'Strategy') ? spec[purpose + 'Strategy'] : spec.generalStrategy;
        return new Scheduler({strategy: strategy, scheduleReserveTime: spec.scheduleReserveTime});
    };

    var checkAlive = function () {
        for (var worker in workers) {
            workers[worker].alive_count += 1;
            if (workers[worker].alive_count > check_alive_count) {
                log.info('Worker', worker, 'is not alive any longer, Deleting it.');
                that.workerQuit(worker);
            }
        }
    };

    that.workerJoin = function (purpose, worker, info, on_ok, on_error) {
        schedulers[purpose] = schedulers[purpose] || createScheduler(purpose);
        schedulers[purpose].add(worker, info.state, info.max_load, function () {
            workers[worker] = {purpose: purpose,
                               info: info,
                               alive_count: 0};
            on_ok();
        }, on_error);
    };

    that.workerQuit = function (worker) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].remove(worker);
    };

    that.keepAlive = function (worker, on_result) {
        if (workers[worker]) {
            workers[worker].alive_count = 0;
            on_result('ok');
        } else {
            on_result('whoareyou');
        }
    };

    that.reportState = function (worker, state) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].updateState(worker, state);
    };

    that.reportLoad = function (worker, load) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].updateLoad(worker, load);
    };

    that.pickUpTasks = function (worker, tasks) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].pickUpTasks(worker, tasks);
    };

    that.layDownTask = function (worker, task) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].layDownTask(worker, task);
    };

    that.schedule = function (purpose, task, reserveTime, on_ok, on_error) {
        if (state === 'in-service') {
            if (schedulers[purpose]) {
                schedulers[purpose].schedule(task, reserveTime, function(worker) {
                    on_ok(worker, workers[worker].info);
                }, on_error);
            } else {
                log.warn('No scheduler for purpose:', purpose);
                on_error('No scheduler for purpose: ' + purpose);
            }
        } else {
           log.warn('cluster manager is not ready.');
           on_error('cluster manager is not ready.');
        }
    };

    that.unschedule = function (worker, task) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].unschedule(worker, task);
    };

    that.getWorkerAttr = function (worker, on_ok, on_error) {
        if (workers[worker]) {
            // FIXME: the following attr items are for purpose of compaticity with legacy oam client, should be refined later.
            if (workers[worker].purpose === 'portal') {
                var scheduling_info = schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].getInfo(worker);
                scheduling_info = scheduling_info || {state: 0, load: 0, max_load: 1.0, tasks: []};
                on_ok({id: worker,
                       purpose: workers[worker].purpose,
                       ip: workers[worker].info.ip,
                       rpcID: worker,
                       state: scheduling_info.state,
                       load: scheduling_info.load,
                       hostname: workers[worker].info.hostname || '',
                       port: workers[worker].info.port || 0,
                       keepAlive: workers[worker].alive_count});
            } else {
                on_ok(workers[worker].info);
            }
        } else {
            on_error('Worker [' + worker + '] does NOT exist.');
        }
    };

    that.getWorkers = function (purpose, on_ok) {
        if (purpose === 'all') {
            on_ok(Object.keys(workers));
        } else {
            result = [];
            for (var worker in workers) {
                if (workers[worker].purpose === purpose) {
                    result.push(worker);
                }
            }
            on_ok(result);
        }
    };

    that.getTasks = function (worker, on_ok) {
        return workers[worker] && schedulers[workers[worker].purpose] ? schedulers[workers[worker].purpose].getTasks(worker) : [];
    };

    that.getScheduled = function (purpose, task, on_ok, on_error) {
        if (schedulers[purpose]) {
            schedulers[purpose].getScheduled(task, on_ok, on_error);
        } else {
            on_error('Invalid purpose.');
        }
    };

    init();
    return that;
};

module.exports.API = function (spec) {
    var manager = new ClusterManager(spec);
    var that = {};

    that.join = function (purpose, worker, info, callback) {
        manager.workerJoin(purpose, worker, info, function () {
            callback('callback', 'ok');
        }, function (error_reason) {
            callback('callback', 'error', error_reason);
        });
    };

    that.quit = function (worker) {
        manager.workerQuit(worker);
    };

    that.keepAlive = function (worker, callback) {
        manager.keepAlive(worker, function (result) {
            callback('callback', result);
        });
    };

    that.reportState = function (worker, state) {
        manager.reportState(worker, state);
    };

    that.reportLoad = function (worker, load) {
        manager.reportLoad(worker, load);
    };

    that.pickUpTasks = function (worker, tasks) {
        manager.pickUpTasks(worker, tasks);
    };

    that.layDownTask = function (worker, task) {
        manager.layDownTask(worker, task);
    };

    that.schedule = function (purpose, task, reserveTime, callback) {
        manager.schedule(purpose, task, reserveTime, function(worker, info) {
            callback('callback', {id: worker, info: info});
        }, function (error_reason) {
            callback('callback', 'error', error_reason);
        });
    };

    that.unschedule = function (worker, task) {
        manager.unschedule(worker, task);
    };

    that.getWorkerAttr = function (worker, callback) {
        manager.getWorkerAttr(worker, function (attr) {
            callback('callback', attr);
        }, function (error_reason) {
            callback('callback', 'error', error_reason);
        });
    };

    that.getWorkers = function (purpose, callback) {
        manager.getWorkers(purpose, function (workerList) {
            callback('callback', workerList);
        });
    };

    that.getTasks = function (worker, callback) {
        manager.getTasks(worker, function (taskList) {
            callback('callback', taskList);
        });
    };

    that.getScheduled = function (purpose, task, callback) {
        manager.getScheduled(purpose, task, function (worker) {
            callback('callback', worker);
        }, function (error_reason) {
            callback('callback', 'error', error_reason);
        });
    };

    return that;
};
















