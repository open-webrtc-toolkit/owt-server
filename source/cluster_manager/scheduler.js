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
    return worker.load < worker.info.max_load && (worker.state === undefined || worker.state === 2);
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
        log.info("reserveWorkerForTask task is:", task, " worker is:", worker);
        if (tasks[task]) {
            if(tasks[task][worker]) {
                tasks[task][worker].reserve_timer && clearTimeout(tasks[task][worker].reserve_timer);
            }
        } else {
            tasks[task] = {};
        }

        tasks[task][worker] = {
                        id: worker,
                        reserve_time: time,
                        reserve_timer: setTimeout(function () {repealTask(task, worker);}, time)};
        log.info("reserveWorkerForTask tasks is:", tasks);
    };

    var repealTask = function (task, worker) {
        if (tasks[task] && tasks[task][worker]) {
	    log.info("repeal task:", task, " worker:", worker, " tasks:", tasks, " tasks[task][worker]:", tasks[task][worker]);
            tasks[task][worker].reserve_timer && clearTimeout(tasks[task][worker].reserve_timer);
	    delete tasks[task][worker];
        }
    };

    var executeTask = function (worker, task) {
        log.info("executeTask task is:", task, " worker is:", worker);
        log.info("executeTask before tasks is:", tasks);
        if (workers[worker]) {
            if (workers[worker].tasks.indexOf(task) === -1) {
                workers[worker].tasks.push(task);
            }

            if (tasks[task]) {
                if(tasks[task][worker]) {
                    tasks[task][worker].reserve_timer && clearTimeout(tasks[task][worker].reserve_timer);
                } else{
                    tasks[task][worker] = {
                        id: worker, 
                        reserve_time: schedule_reserve_time};
                }
            } else {
                tasks[task] = {};
                tasks[task][worker] = {
                    id: worker,
                    reserve_time: schedule_reserve_time};
            }

            log.info("executeTask after tasks is:", tasks);
        }
    };

    var cancelExecution = function (worker, task, time) {
        var needToReserve = false;

        if (workers[worker]) {
            var i = workers[worker].tasks.indexOf(task);
            if (i !== -1) {
                workers[worker].tasks.splice(i, 1);
                needToReserve = true;
            }
        }

        if (tasks[task]) {
	    if (!time) {
	       repealTask(task, worker);
	    } else {
            if (needToReserve) {
		log.info("cancel execution worker:", worker, " task:", task);
                reserveWorkerForTask(task, worker, tasks[task].reserve_time);
            } else {
                repealTask(task, worker);
            }
	    }
        }
    };

    var isTaskInExecution = function (task, worker) {
        return tasks[task] && workers[worker] && tasks[task][worker] && workers[worker].tasks.indexOf(task) !== -1;
    };

    that.add = function (worker, info) {
        log.debug('Add worker:', worker, 'info:', info);
        if (workers[worker]) {
            log.warn('Double adding worker:', worker);
        }
        workers[worker] = {state: undefined,
                           load: info.max_load || 1.0,
                           info: info,
                           tasks: []};
    };

    that.remove = function (worker) {
        log.debug('Remove worker:', worker);

        if (workers[worker]) {
            workers[worker].tasks.map(function (task) {
                if (tasks[task] && tasks[task][worker]) {
                    delete tasks[task][worker];
                }
            });
            delete workers[worker];
        }
    };

    that.updateState = function (worker, state) {
        workers[worker] && (workers[worker].state = state);
    };

    that.updateLoad = function (worker, load) {
        workers[worker] && (workers[worker].load = load);
    };

    that.pickUpTasks = function (worker, tasks) {
        if (workers[worker]) {
            tasks.forEach(function (task) { executeTask(worker, task); });
        }
    };

    that.layDownTask = function (worker, task, time) {
	log.info("lay down task:", worker, " task:", task, " time:", time);
        if (workers[worker]) {
            cancelExecution(worker, task, time);

        }
    };

    that.schedule = function (purpose, task, preference, reserveTime, on_ok, on_error) {
        if(purpose !== 'conference') {
            if (tasks[task]) {
                var newReserveTime = reserveTime && tasks[task].reserve_time < reserveTime ? reserveTime : tasks[task].reserve_time,
                    worker = Object.keys(tasks[task])[0];

                if (workers[worker]) {
                    if (isTaskInExecution(task, worker)) {
                        tasks[task][worker].reserve_time = newReserveTime;
                    } else {
                        reserveWorkerForTask(task, worker, newReserveTime);
                    }
                    return on_ok(worker, workers[worker].info);
                } else {
                    repealTask(task, worker);
                }
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
                if (!isTaskInExecution(task, worker)) {
                    reserveWorkerForTask(task, worker, (reserveTime && reserveTime > 0 ? reserveTime : schedule_reserve_time));
                }
                on_ok(worker, workers[worker].info);
            }, on_error);
        }
    };

    that.unschedule = function (worker, task) {
        if (tasks[task] && tasks[task][worker]) {
            repealTask(task, worker);
        }
    };

    that.getInfo = function (worker) {
        return workers[worker];
    };

    that.getScheduledWorkers = function (task, on_ok) {
        var worker = [];
        if(tasks[task]) {
            worker = Object.keys(tasks[task]);
        }
        log.info("get scheduled workers:", worker, " for task:", task);
        on_ok(worker);
    };

    that.getScheduled = function (task, on_ok, on_error) {
        if(tasks[task]) {
            var worker = Object.keys(tasks[task]);
            if(worker.length >1) {
                var index = Math.floor(Math.random() * worker.length);
                on_ok(tasks[task][worker[index]].id);
            } else {
                var key = worker[0];
		log.info("getScheduled tasks:", tasks, " key:", key);
                key && on_ok(tasks[task][key].id);
            }

        } else {
            on_error('No such a task');
        }
    };

    that.setScheduled = function (task, worker, reserveTime) {
        tasks[task][worker] = {reserve_time: reserveTime};
    };

    that.getData = function () {
        var data = {workers: workers, tasks: {}};
        for (var task in tasks) {
            data.tasks[task] = tasks[task];
        }
        return data;
    };

    that.setData = function (data) {
        workers = data.workers;
        for (var task in data.tasks) {
            tasks[task] = data.tasks[task];
        }
    };

    that.serve = function () {
        for (var task in tasks) {
            var worker = tasks[task].worker;
            if (workers[worker] && (workers[worker].tasks.indexOf(task) === -1)) {
		log.info("serve worker:", worker, " task:", task);
                reserveWorkerForTask(task, worker, tasks[task][worker].reserve_time);
            }
        }
    };

    return that;
};

