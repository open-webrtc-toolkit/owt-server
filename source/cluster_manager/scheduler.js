/*global require, setTimeout, clearTimeout, exports*/
'use strict';
var logger = require('./logger').logger;
var Strategy = require('./strategy');

// Logger
var log = logger.getLogger('Scheduler');


exports.Scheduler = function(spec) {
    var that = {};

    /*State <- [0 | 1 | 2]*/
    /*{WorkerId: {state: State, load: Number, max_load: Number, tasks:[Task]}*/
    var workers = {};

    /*{Task: {reserve_timer: TimerId,
              reserve_time: Number,
              worker: WorkerId}
      }*/
    var tasks = {};

    var strategy = Strategy.create(spec.strategy),
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

    that.add = function (worker, state, max_load, on_ok, on_error) {
        if (workers[worker]) {
            log.warn('Double adding worker:', worker);
            return on_error('Double adding worker.');
        }

        log.info('Add worker:', worker);
        workers[worker] = {state: state,
                           load: 0,
                           max_load: max_load,
                           tasks: []};
        return on_ok();
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

    that.schedule = function (task, reserveTime, on_ok, on_error) {
        if (tasks[task]) {
            var newReserveTime = reserveTime && tasks[task].reserve_time < reserveTime ? reserveTime : tasks[task].reserve_time,
                worker = tasks[task].worker;

            if (workers[worker]) {
                if (isTaskInExecution(task, worker)) {
                    tasks[task].reserve_time = newReserveTime;
                } else {
                    reserveWorkerForTask(task, worker, newReserveTime);
                }
                return on_ok(worker);
            } else {
                repealTask(task);
            }
        }

        strategy.allocate(workers, function (worker) {
            reserveWorkerForTask(task, worker, (reserveTime && reserveTime > 0 ? reserveTime : schedule_reserve_time));
            on_ok(worker);
        }, on_error);
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
        tasks[task] ? on_ok(tasks[task].worker) : on_error('No such a task');
    };

    return that;
};

