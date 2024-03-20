// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var logger = require('./logger').logger;
var Matcher = require('./matcher');
var Strategy = require('./strategy');

// Logger
var log = logger.getLogger('Scheduler');

var isWorkerAvailable = function (worker) {
    return worker.load < worker.info.maxLoad && (worker.state === undefined || worker.state === 2);
};


exports.Scheduler = function(spec) {
    var that = {};

    /*State <- [0 | 1 | 2]*/
    /*{WorkerId: {state: State, load: Number, info: info, tasks:[Task]}*/
    var workers = {};

    /*{Task: {reserve_timer: TimerId,
              reserve_time: Number,
              worker: WorkerId}
      }*/
    var tasks = {};

    var matcher = Matcher.create(spec.purpose),
        strategy = Strategy.create(spec.strategy),
        schedule_reserve_time = spec.scheduleReserveTime;

    var reserveWorkerForTask = function (task, worker, time) {
        if (tasks[task]) {
            tasks[task].reserve_timer && clearTimeout(tasks[task].reserve_timer);
        }

        tasks[task] = {worker: worker,
                       reserve_time: time,
                       reserve_timer: setTimeout(function () {repealTask(task);}, time)};
    };

    var repealTask = function (task) {
        if (tasks[task]) {
            tasks[task].reserve_timer && clearTimeout(tasks[task].reserve_timer);
            delete tasks[task];
        }
    };

    var executeTask = function (worker, task) {
        if (workers[worker]) {
            if (workers[worker].tasks.indexOf(task) === -1) {
                workers[worker].tasks.push(task);
            }

            if (tasks[task]) {
                tasks[task].reserve_timer && clearTimeout(tasks[task].reserve_timer);

                if (tasks[task].worker !== worker) {
                    log.warn('Worker conflicts for task:', task, 'and update to worker:', worker);
                    tasks[task].worker = worker;
                }
            } else {
               tasks[task] = {reserve_time: schedule_reserve_time,
                              worker: worker};
            }
        }
    };

    var cancelExecution = function (worker, task) {
        var needToReserve = false;

        if (workers[worker]) {
            var i = workers[worker].tasks.indexOf(task);
            if (i !== -1) {
                workers[worker].tasks.splice(i, 1);
                needToReserve = true;
            }
        }

        if (tasks[task]) {
            if (needToReserve) {
                reserveWorkerForTask(task, worker, tasks[task].reserve_time);
            } else {
                repealTask(task);
            }
        }
    };

    var isTaskInExecution = function (task, worker) {
        return tasks[task] && workers[worker] && worker === tasks[task].worker && workers[worker].tasks.indexOf(task) !== -1;
    };

    that.add = function (worker, info) {
        log.debug('Add worker:', worker, 'info:', info);
        if (workers[worker]) {
            log.warn('Double adding worker:', worker);
        }
        workers[worker] = {state: undefined,
                           load: info.maxLoad || 1.0,
                           info: info,
                           tasks: []};
    };

    that.remove = function (worker) {
        log.debug('Remove worker:', worker);

        if (workers[worker]) {
            workers[worker].tasks.map(function (task) {
                if (tasks[task] && tasks[task].worker === worker) {
                    repealTask(task);
                }
            });
            delete workers[worker];
        }
    };

    that.updateState = function (worker, state) {
        workers[worker] && (workers[worker].state = state);
    };

    that.isAlive = function (worker) {
        return workers[worker] && (worker.state === undefined || worker.state === 2)
    };

    that.updateLoad = function (worker, load) {
        workers[worker] && (workers[worker].load = load);
    };

    that.pickUpTasks = function (worker, tasks) {
        if (workers[worker]) {
            tasks.forEach(function (task) { executeTask(worker, task); });
        }
    };

    that.layDownTask = function (worker, task) {
        if (workers[worker]) {
            cancelExecution(worker, task);
        }
    };

    that.schedule = function (task, preference, reserveTime, on_ok, on_error) {
        if (tasks[task]) {
            var newReserveTime = reserveTime && tasks[task].reserve_time < reserveTime ? reserveTime : tasks[task].reserve_time,
                worker = tasks[task].worker;

            if (workers[worker]) {
                if (isTaskInExecution(task, worker)) {
                    tasks[task].reserve_time = newReserveTime;
                } else {
                    reserveWorkerForTask(task, worker, newReserveTime);
                }
                return on_ok(worker, workers[worker].info);
            } else {
                repealTask(task);
            }
        }

        var candidates = [];
        for (var worker in workers) {
            if (isWorkerAvailable(workers[worker])) {
                candidates.push(worker);
            }
        }

        if (candidates.length < 1) {
            return on_error('No worker available, all in full load.');
        }

        candidates = matcher.match(preference, workers, candidates);

        if (candidates.length < 1) {
            return on_error('No worker matches the preference.');
        } else {
            strategy.allocate(workers, candidates, function (worker) {
                reserveWorkerForTask(task, worker, (reserveTime && reserveTime > 0 ? reserveTime : schedule_reserve_time));
                on_ok(worker, workers[worker].info);
            }, on_error);
        }
    };

    that.unschedule = function (worker, task) {
        if (tasks[task] && tasks[task].worker === worker) {
            repealTask(task);
        }
    };

    that.getInfo = function (worker) {
        return workers[worker];
    };

    that.getScheduled = function (task, on_ok, on_error) {
        /**
         * keycoding 20230811
         * 1、modify the error msg,let the clusterWork get the real error
         */
        tasks[task] ? on_ok(tasks[task].worker) : on_error(`NO_SUCH_TASK:${task}`);
    };

    that.setScheduled = function (task, worker, reserveTime) {
        tasks[task] = {worker: worker,
                       reserve_time: reserveTime};
    };

    that.getData = function () {
        var data = {workers: workers, tasks: {}};
        for (var task in tasks) {
            data.tasks[task] = {reserve_time: tasks[task].reserve_time,
                                worker: tasks[task].worker};
        }
        return data;
    };

    that.setData = function (data) {
        workers = data.workers;
        for (var task in data.tasks) {
            tasks[task] = data.tasks[task];
        }
    };

    that.isWorkerAvailable = function(worker){
        return isWorkerAvailable(workers[worker]);
    }

    that.serve = function () {
        for (var task in tasks) {
            var worker = tasks[task].worker;
            if (workers[worker] && (workers[worker].tasks.indexOf(task) === -1)) {
                reserveWorkerForTask(task, worker, tasks[task].reserve_time);
            }
        }
    };

    return that;
};

