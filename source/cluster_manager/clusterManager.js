// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var logger = require('./logger').logger;
var Scheduler = require('./scheduler').Scheduler;
var util = require('util');

// Logger
var log = logger.getLogger('ClusterManager');
var role = null;
var Url = require("url");

var ClusterManager = function (clusterName, selfId, spec) {
    var that = {name: clusterName, id: selfId , totalNode: spec.totalNode, electTimeout: spec.electTimeout};

    /*initializing | in-service*/
    var state = 'initializing',
        is_freshman = true,
        monitoringTarget,

        initial_time = spec.initialTime,
        check_alive_period = spec.checkAlivePeriod,
        check_alive_count = spec.checkAliveCount;

    /* {Purpose: Scheduler}*/
    var schedulers = {};

    /*Id : {purpose: Purpose, alive_count: Number}*/
    var workers = {};
    var clusterInfo = {};

    var data_synchronizer;

    function validateUrl(url) {
        try {
            if (spec.enableCascading) {
                new Url.URL(url);
                return true;
            }
            return false;
        } catch {
            return false;
        }
    }

    var sendRequest = validateUrl(spec.url);

    var send = function (method, resource, body) {
      log.info("send info to url:", spec.url);
      const data = JSON.stringify(body);
      var url = Url.parse(spec.url + "/" + resource);
      var ssl = (url.protocol === 'https:' ? true : false);

      const options = {
        hostname: url.hostname,
        port: url.port,
        path: url.pathname + (url.search ? url.search : ''),
        method: method,
        headers: {
          'Content-Type': 'application/json',
          'Content-Length': data.length,
        },
      };
      ssl && (options.rejectUnauthorized = false);
      log.info("send options:", options);
      const http = (ssl ? require('https') : require('http'));
      const req = http.request(options, res => {
        console.log(`statusCode: ${res.statusCode}`);

        res.on('data', d => {
          process.stdout.write(d);
        });
      });

      req.on('error', error => {
        console.error(error);
      });

      req.write(data);
      req.end();

    }

    var createScheduler = function (purpose) {
        var strategy = spec.strategy[purpose] ? spec.strategy[purpose] : spec.strategy.general;
        return new Scheduler({purpose: purpose, strategy: strategy, scheduleReserveTime: spec.scheduleReserveTime});
    };

    var checkAlive = function () {
        for (var worker in workers) {
            workers[worker].alive_count += 1;
            if (workers[worker].alive_count > check_alive_count) {
                log.info('Worker', worker, 'is not alive any longer, Deleting it.');
                workerQuit(worker);
            }
        }
    };

    var workerJoin = function (purpose, worker, info) {
        log.debug('workerJoin, purpose:', purpose, 'worker:', worker, 'info:', info);
        schedulers[purpose] = schedulers[purpose] || createScheduler(purpose);
        schedulers[purpose].add(worker, info);
        workers[worker] = {purpose: purpose,
                           alive_count: 0};
        if (!clusterInfo[purpose]) {
            clusterInfo[purpose] = new Set()
            var data = {
                clusterID: spec.clusterID,
                region: spec.region,
                info: {
                    action: "add",
                    capacity: purpose
                }
            }
            if (sendRequest) {
                log.info("Send updateCapacity event add to cloud with data:", data);
                send('POST', 'updateCapacity', data);
            }
        }
        clusterInfo[purpose].add(worker);
        data_synchronizer && data_synchronizer({type: 'worker_join', payload: {purpose: purpose, worker: worker, info: info}});
        return state;
    };

    var workerQuit = function (worker) {
        log.debug('workerQuit, worker:', worker);
        var purpose = workers[worker].purpose;
        if (workers[worker] && schedulers[workers[worker].purpose]) {
            schedulers[workers[worker].purpose].remove(worker);
            monitoringTarget && monitoringTarget.notify('quit', {purpose: workers[worker].purpose, id: worker, type: 'worker'});
            delete workers[worker];
            data_synchronizer && data_synchronizer({type: 'worker_quit', payload: {worker: worker}});
        }

        if (purpose) {
            clusterInfo[purpose].delete(worker);
            if(!clusterInfo[purpose]) {
               var data = {
                    clusterID: spec.clusterID,
                    region: spec.region,
                    info: {
                        action: "remove",
                        capacity: purpose
                    }
                }
                if (sendRequest) {
                    log.info("Send updateCapacity event remove to cloud with data:", data);
                    send('POST', 'updateCapacity', data);
                }
            }
        }
    };

    var keepAlive = function (worker, on_result) {
        if (workers[worker]) {
            workers[worker].alive_count = 0;
            on_result('ok');
        } else {
            on_result('whoareyou');
        }
    };

    var reportState = function (worker, state) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].updateState(worker, state);
        data_synchronizer && data_synchronizer({type: 'worker_state', payload: {worker: worker, state: state}});
    };

    var reportLoad = function (worker, load) {
        log.debug('reportLoad, worker:', worker, 'load:', load);
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].updateLoad(worker, load);
        data_synchronizer && data_synchronizer({type: 'worker_load', payload: {worker: worker, load: load}});
    };

    var pickUpTasks = function (worker, tasks) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].pickUpTasks(worker, tasks);
        data_synchronizer && data_synchronizer({type: 'worker_pickup', payload: {worker: worker, tasks: tasks}});
    };

    var layDownTask = function (worker, task) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].layDownTask(worker, task);
        data_synchronizer && data_synchronizer({type: 'worker_laydown', payload: {worker: worker, task: task}});
    };

    var schedule = function (purpose, task, preference, reserveTime, on_ok, on_error) {
        log.debug('schedule, purpose:', purpose, 'task:', task, ', preference:', preference, 'reserveTime:', reserveTime, 'while state:', state);
        if (state === 'in-service') {
            if (schedulers[purpose]) {
                schedulers[purpose].schedule(task, preference, reserveTime, function(worker, info) {
                    log.debug('schedule OK, got  worker', worker);
                    on_ok(worker, info);
                    data_synchronizer && data_synchronizer({type: 'scheduled', payload: {purpose: purpose, task: task, worker: worker, reserve_time: reserveTime}});
                }, function (reason) {
                    log.warn('schedule failed, purpose:', purpose, 'task:', task, 'reason:', reason);
                    on_error('Failed in scheduling ' + purpose + ' worker, reason: ' + reason);
                });
            } else {
                log.warn('No scheduler for purpose:', purpose);
                on_error('No scheduler for purpose: ' + purpose);
            }
        } else {
           log.warn('cluster manager is not ready.');
           on_error('cluster manager is not ready.');
        }
    };

    var unschedule = function (worker, task) {
        workers[worker] && schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].unschedule(worker, task);
        data_synchronizer && data_synchronizer({type: 'unscheduled', payload: {worker: worker, task: task}});
    };

    var getClusterInfo = function (on_ok) {
        on_ok(Object.keys(clusterInfo));
    };

    var getWorkerAttr = function (worker, on_ok, on_error) {
        if (workers[worker]) {
            var worker_info = schedulers[workers[worker].purpose] && schedulers[workers[worker].purpose].getInfo(worker);
            worker_info = worker_info || {state: 0, load: 0, info: {}, tasks: []};
            // FIXME: the following attr items are for purpose of compaticity with legacy oam client, should be refined later.
            if (workers[worker].purpose === 'portal') {
                on_ok({id: worker,
                       purpose: workers[worker].purpose,
                       ip: worker_info.info.ip,
                       rpcID: worker,
                       state: worker_info.state,
                       load: worker_info.load,
                       hostname: worker_info.info.hostname || '',
                       port: worker_info.info.port || 0,
                       keepAlive: workers[worker].alive_count});
            } else {
                on_ok(worker_info);
            }
        } else {
            on_error('Worker [' + worker + '] does NOT exist.');
        }
    };

    var getWorkers = function (purpose, on_ok) {
        if (purpose === 'all') {
            on_ok(Object.keys(workers));
        } else {
            var result = [];
            for (var worker in workers) {
                if (workers[worker].purpose === purpose) {
                    result.push(worker);
                }
            }
            on_ok(result);
        }
    };

    var getTasks = function (worker, on_ok) {
        return workers[worker] && schedulers[workers[worker].purpose] ? schedulers[workers[worker].purpose].getTasks(worker) : [];
    };

    var getScheduled = function (purpose, task, on_ok, on_error) {
        if (schedulers[purpose]) {
            schedulers[purpose].getScheduled(task, on_ok, on_error);
        } else {
            on_error('Invalid purpose.');
        }
    };

    var getClusterID = function (room_id, roomToken, on_ok) {
        on_ok(spec.clusterID);

        if (sendRequest) {
            var data = {
                clusterID: spec.clusterID,
                region: spec.region,
                info: {
                    room: room_id,
                    token: roomToken
                }
            }

            log.info("Send room info to cloud with data:", data);
            send('POST', 'updateConference', data);
        }
    };

    var registerInfo = function (info, on_ok) {
        on_ok('ok');
        if (sendRequest) {
            var data = {
                clusterID: spec.clusterID,
                region: spec.region,
                info: {
                    resturl: info.resturl,
                    servicekey: info.servicekey,
                    serviceid: info.serviceid
                }
            }

            log.info("Send registerCluster event to cloud with data:", data);
            send('POST', 'registerCluster', data);
        }
    };

    var leaveConference = function (info, on_ok) {
        on_ok('ok');
        var data = {
            clusterID: spec.clusterID,
            region: spec.region,
            conferenceId: info
        }
        if (sendRequest) {
            log.info("Send conference leave event to cloud with data:", data);
            send('POST', 'leaveConference', data);
        }
    };

    var unregisterCluster = function () {
        var data = {
            clusterID: spec.clusterID,
            region: spec.region
        }
        if (sendRequest) {
            log.info("Send unregister cluster event to cloud with data:", data);
            send('POST', 'unregisterCluster', data);
        }
    };

    that.stopCluster = function () {
        unregisterCluster();
    }

    that.getRuntimeData = function (on_data) {
        var data = {schedulers: {}, workers: workers};
        for (var purpose in schedulers) {
            data.schedulers[purpose] = schedulers[purpose].getData();
        }
        on_data(data);
    };

    that.registerDataUpdate = function (on_updated_data) {
        data_synchronizer = on_updated_data;
    };

    that.setRuntimeData = function (data) {
         log.debug('onRuntimeData, data:', data);
         if (is_freshman) {
             is_freshman = false;
         }
         workers = data.workers;
         for (var purpose in data.schedulers) {
             schedulers[purpose] = createScheduler(purpose);
             schedulers[purpose].setData(data.schedulers[purpose]);
         }
    };

    that.setUpdatedData = function (data) {
        if (is_freshman) {
            return;
        }
        log.debug('onUpdatedData, data:', data);
        switch (data.type) {
        case 'worker_join':
            workerJoin(data.payload.purpose, data.payload.worker, data.payload.info);
            break;
        case 'worker_quit':
            workerQuit(data.payload.worker);
            break;
        case 'worker_state':
            reportState(data.payload.worker, data.payload.state);
            break;
        case 'worker_load':
            reportLoad(data.payload.worker, data.payload.load);
            break;
        case 'worker_pickup':
            pickUpTasks(data.payload.worker, data.payload.tasks);
            break;
        case 'worker_laydown':
            layDownTask(data.payload.worker, data.payload.task);
            break;
        case 'scheduled':
            schedulers[data.payload.purpose] && schedulers[data.payload.purpose].setScheduled(data.payload.task, data.payload.worker, data.payload.reserve_time);
            break;
        case 'unscheduled':
            unschedule(data.payload.worker, data.payload.task);
            break;
        default:
            log.warn('unknown updated data type:', data.type);
        }
    };

    let interval = undefined;
    that.serve = function (monitoringTgt) {
        if (is_freshman) {
            setTimeout(function () {
                state = 'in-service';
            }, initial_time);
        } else {
            state = 'in-service';
        }
        is_freshman = false;
        monitoringTarget = monitoringTgt;
        interval = setInterval(checkAlive, check_alive_period);
        for (var purpose in schedulers) {
            schedulers[purpose].serve();
        }
    };

    that.unserve = function () {
        state = 'initializing'
        monitoringTarget = undefined;
        interval && clearInterval(interval);
        is_freshman = true;
        //schedulers = {};
        //workers = {};
        data_synchronizer = undefined;
    }

    that.rpcAPI = {
        join: function (purpose, worker, info, callback) {
            var result = workerJoin(purpose, worker, info);
            callback('callback', result);
        },
        quit: function (worker) {
            workerQuit(worker);
        },
        keepAlive: function (worker, callback) {
            keepAlive(worker, function (result) {
                callback('callback', result);
            });
        },
        reportState: function (worker, state) {
            reportState(worker, state);
        },
        reportLoad: function (worker, load) {
            reportLoad(worker, load);
        },
        pickUpTasks: function (worker, tasks) {
            pickUpTasks(worker, tasks);
        },
        layDownTask: function (worker, task) {
            layDownTask(worker, task);
        },
        schedule: function (purpose, task, preference, reserveTime, callback) {
            schedule(purpose, task, preference, reserveTime, function(worker, info) {
                callback('callback', {id: worker, info: info});
            }, function (error_reason) {
                callback('callback', 'error', error_reason);
            });
        },
        unschedule: function (worker, task) {
            unschedule(worker, task);
        },
        getWorkerAttr: function (worker, callback) {
            getWorkerAttr(worker, function (attr) {
                callback('callback', attr);
            }, function (error_reason) {
                callback('callback', 'error', error_reason);
            });
        },
        getWorkers: function (purpose, callback) {
            getWorkers(purpose, function (workerList) {
                callback('callback', workerList);
            });
        },
        getTasks: function (worker, callback) {
            getTasks(worker, function (taskList) {
                callback('callback', taskList);
            });
        },
        getScheduled: function (purpose, task, callback) {
            getScheduled(purpose, task, function (worker) {
                callback('callback', worker);
            }, function (error_reason) {
                callback('callback', 'error', error_reason);
            });
        },
        getClusterID: function (room_id, roomToken, callback) {
            getClusterID(room_id, roomToken, function (cluster) {
                callback('callback', cluster);
            });
        },
        registerInfo: function (info, callback) {
            registerInfo(info, function (worker) {
                callback('callback', 'ok');
            });
        },
        leaveConference: function (info, callback) {
            leaveConference(info, function (worker) {
                callback('callback', 'ok');
            });
        }
    };

    // API for grpc
    that.grpcInterface = {
        join: function (call, callback) {
            const req = call.request;
            const result = workerJoin(req.purpose, req.id, req.info);
            callback(null, {state: result});
        },
        quit: function(call, callback) {
            workerQuit(call.request.id);
            callback(null, {});
        },
        keepAlive: function(call, callback) {
            keepAlive(call.request.id, function (result) {
                callback(null, {message: result});
            });
        },
        reportState: function (call, callback) {
            reportState(call.request.id, call.request.state);
            callback(null, {});
        },
        reportLoad: function (call, callback) {
            reportLoad(call.request.id, call.request.load);
            callback(null, {});
        },
        pickUpTasks: function (call, callback) {
            pickUpTasks(call.request.id, call.request.tasks);
            callback(null, {});
        },
        layDownTask: function (call, callback) {
            layDownTask(call.request.id, call.request.task);
            callback(null, {});
        },
        schedule: function (call, callback) {
            const purpose = call.request.purpose;
            const task = call.request.task;
            const preference = call.request.preference;
            const reserveTime = call.request.reserveTime;
            schedule(purpose, task, preference, reserveTime, function(worker, info) {
                callback(null, {id: worker, info: info});
            }, function (reason) {
                callback(new Error(reason), null);
            });
        },
        unschedule: function (call, callback) {
            unschedule(call.request.id, call.request.task);
            callback(null, {});
        },
        getWorkerAttr: function (call, callback) {
            getWorkerAttr(call.request.id, function (attr) {
                callback(null, attr);
            }, function (reason) {
                callback(new Error(reason), null);
            });
        },
        getWorkers: function (call, callback) {
            getWorkers(call.request.purpose, function (workerList) {
                callback(null, {list: workerList});
            });
        },
        getTasks: function (call, callback) {
            getTasks(call.request.id, function (taskList) {
                callback(null, {list: taskList});
            });
        },
        getScheduled: function (call, callback) {
            const req = call.request;
            getScheduled(req.purpose, req.task, function (worker) {
                callback(null, {message: worker});
            }, function (reason) {
                callback(new Error(reason), null);
            });
        }
    };

    return that;
};

var getTimerCount = function(sec, timerStep) {
    if (timerStep <= 0) {
        timerStep = 100;
    }
    if (timerStep > sec) {
        sec = timerStep;
    }
    return parseInt((sec + timerStep - 1) / timerStep);
}

class HeartbeatObserver {
    constructor(onLoss) {
        this.interval = undefined;
        this.lastContact = new Date();
        this.state = state;
        this.onLoss = onLoss;
        this.minStepCount = getTimerCount(parseInt(state.electTimeout), state.minHeartTicket);
        this.maxStepCount = getTimerCount(state.electTimeout * 2, state.minHeartTicket);
        this.observerTimes = 0;
    }

    resetTimer() {
        let self = this;
        self.observerTimes++;
        self.interval && clearInterval(self.interval);
        let loseTime = undefined;
        self.lastContact = new Date();
        let lossCount = 0;
        let maxCount = self.maxStepCount;
        //if it was run first let it elect to max avoid it's network lately term plus to large
        if (self.observerTimes > 1) {
            maxCount = Math.random() * (self.maxStepCount - self.minStepCount) + self.minStepCount;
        }
        self.interval = setInterval(function() {
            loseTime = new Date();
            lossCount++;
            if (lossCount > maxCount) {
                self.interval && clearInterval(self.interval);
                self.interval = undefined;
                self.onLoss(loseTime, self.lastContact);
            }
        }, state.minHeartTicket);
    };

    stop() {
        let self = this;
        self.interval && clearInterval(self.interval);
        self.interval = undefined;
    }
}

class Mutex {
    constructor() {
        this._locked = false;
        this.lockKey = "";
        this.lockIndex = 0;
        this.currentMessage = "";

        this.preLockKey = "";
        this.preMessage = "";
        this.preBegin = (new Date()).getTime();
        this.preEnd = this.preBegin;
        this.preDelay = 0;
        this._waitingPromises = [];
    }

    lock(lockKey, message) {
        if (!this._locked) {
            this._locked = true;
            this.lockKey = lockKey;
            this.currentMessage = message;
            this.begin = (new Date()).getTime();
            return Promise.resolve();
        }
        let self = this;
        return new Promise((resolve, reject) => {
            self.begin = (new Date()).getTime();
            self.lockKey = lockKey;
            self.currentMessage = message;
            self._waitingPromises.push(() => {
                let end = (new Date()).getTime();
                let delay = end - self.begin;
                if (delay > state.maxDelay) {
                    log.debug(state.id, `${self.lockKey}.${self.lockIndex++} handler delay:`, `${delay}ms`, "leaderId:", state.leaderId, "self:", state, "currentMessage:", self.currentMessage, "preBegin:", self.preBegin, "preEnd:", self.preEnd, "preDelay:", `${self.preDelay}ms`, "preMessage:", self.preMessage);
                }
                self.preLockKey = self.lockKey;
                self.preMessage = self.currentMessage;
                self.preBegin = self.begin;
                self.preEnd = end;
                self.preDelay = delay;
                resolve();
            });
        });
    }

    unlock() {
        let self = this;
        if (self._waitingPromises.length > 0) {
            const next = self._waitingPromises.shift();
            next();
        } else {
            let end = (new Date()).getTime();
            let delay = end - self.begin;
            if (delay > state.maxDelay) {
                log.debug(state.id, `${self.lockKey}.${self.lockIndex++} handler delay:`, `${delay}ms`, "leaderId:", state.leaderId, "self:", state, "currentMessage:", self.currentMessage, "preBegin:", self.preBegin, "preEnd:", self.preEnd, "preDelay:", `${self.preDelay}ms`, "preMessage:", self.preMessage);
            }
            self.preLockKey = self.lockKey;
            self.preMessage = self.currentMessage;
            self.preBegin = self.begin;
            self.preEnd = end;
            self.preDelay = delay;
            self._locked = false;
        }
    }
}
const mutex = new Mutex();
const DEFAULT_DATA = -1; //Number.MIN_SAFE_INTEGER;

var state = {
    term: DEFAULT_DATA, // current term
    leaderId: 0, // candidate and follower found leaderId
    commitId: DEFAULT_DATA, //last sync data index
    lastVoteTerm: DEFAULT_DATA, // candidate last vote for other candidate term
    lastVoteFor: DEFAULT_DATA, // candidate last vote for other candidate.id
    voteNum: 0, //candidate laste got vote from other candidate
    totalNode: 0,
    prevLogIndex: DEFAULT_DATA, // last log entry index
    prevLogTerm: DEFAULT_DATA, // last log entry term
    voters: new Set(), //who vote for me
    role: "follower",
    timestamp: (new Date()).getTime(),
    maxDelay: 100
}

var broadcastOnTopicMsg = null;

var runAsFollower = function(topicChannel, manager) {
    var isInstallSnapshot = false;
    var installSnapshotMutex = new Mutex();
    state.role = "follower";
    state.timestamp = (new Date()).getTime();

    var routerKeys = ["clusterManager.follower", `clusterManager.follower.${state.id}`];
    var observer = new HeartbeatObserver(async(loseTime, lastContact) => {
        await mutex.lock("follower.HeartbeatObserver");
        try {
            if (state.role !== "follower") {
                log.warn('follower cycle in HeartbeatObserver', "self:", state);
                return;
            }
            log.fatal("Lose heart-beat from leader:", state.leaderId, "loseTime:", loseTime, 'lastContact:', lastContact, "timeout:", `${loseTime.getTime()-lastContact.getTime()}ms`);
            await changeRole("candidate", state.term);
        } catch (e) {
            log.fatal(util.inspect(e));
        } finally {
            mutex.unlock();
        }
    });

    var changeRole = async function(newRole, newTerm) {
        if (state.role != "follower") {
            log.warn('follower cycle in changeRole', "self:", state);
            return;
        }

        state.leaderId = 0;
        state.term = newTerm;
        state.role = newRole;
        log.info(`follower->${newRole}`, "self:", state);
        await topicChannel.unsubscribe(routerKeys);
        switch (newRole) {
            case "candidate":
                await runAsCandidate(topicChannel, manager);
                break;
            case "follower":
                await runAsFollower(topicChannel, manager);
                break;

        }
    }

    var installSnapshot = async function(data) {
        await installSnapshotMutex.lock("follower.installSnapshot");
        if (!isInstallSnapshot) {
            isInstallSnapshot = true;
            log.info("start installSnapshot last commitId:", state.commitId, "data.commitId:", data.commitId, "from:", data.id);
            topicChannel.publish(`clusterManager.leader.${state.leaderId}`, { type: 'installSnapshot', data: state });
            setTimeout(() => {
                if (isInstallSnapshot) {
                    isInstallSnapshot = false;
                    installSnapshotMutex.unlock();
                }
            }, 500);
            return;
        }
        installSnapshotMutex.unlock();
    };

    var responseVote = function(term, id, commitId, voteForTerm, voteGranted) {
        log.debug('follower Send responseVote..', "term:", term, "id:", id, "commitId:", commitId, "voteForTerm:", voteForTerm, "voteGranted:", voteGranted, "self:", state);
        topicChannel.publish(`clusterManager.candidate.${id}`, { type: 'responseVote', data: { voteGranted, term, id, commitId, voteForTerm, voter: state.id, voterRole: "follower" } });
    }

    var needHandle = function(message) {
        if (message.type === 'heartbeart' || message.type === 'installSnapshot' || message.type === 'appendEntry') {
            let data = message.data;
            if (state.leaderId != 0 && state.leaderId != data.id) {
                return false;
            }
            //statistics delay
            let current = (new Date()).getTime();
            let delay = current - data.timestamp;
            if (delay >= state.maxDelay) {
                log.fatal(`[follower.${state.id}] got hear beat from [leader:${data.id}]: ${new Date(data.timestamp)}, delay: ${delay}ms`);
            } else {
                log.debug(`[follower.${state.id}] got hear beat from [leader:${data.id}]: ${new Date(data.timestamp)}, delay: ${delay}ms`);
            }
            return true
        }
        return true;
    }

    var onTopicMessage = async function(message, routingKey) {
        if (!needHandle(message)) {
            return;
        }

        await mutex.lock(`follower.onTopicMessage.${message.type}`, message);
        try {
            if (state.role != "follower") {
                log.warn('follower cycle in onTopicMessage', "self:", state);
                return;
            }

            let data = message.data;
            if (message.type === 'installSnapshot') {
                //no my data throw it
                if (data.followerId !== state.id) {
                    log.fatal('installSnapshot failed followerId:', data.followerId, "my id:", state.id);
                    return;
                }

                // Ignore an older term
                if (data.term < state.term) {
                    log.warn("ignoring installSnapshot request with older term than current term", "request-term", data.term, "current-term", state.term);
                    return
                }

                //leader is older then me might be it was the older leader
                if (data.commitId < state.commitId) {
                    log.fatal('installSnapshot failed commitId:', data.commitId, "my commitId:", state.commitId);
                    return;
                }

                //the last data id
                state.commitId = data.commitId;
                //the last term
                state.term = data.term;
                //the pre data id
                state.prevLogIndex = data.commitId;
                //the pre data term
                state.prevLogTerm = data.term;
                log.info("installSnapshot last commitId:", state.commitId, "data.commitId:", data.commitId, "from:", data.id, "self:", state, "data:", data);
                observer.resetTimer();
                manager.setRuntimeData(data.data);
                if (isInstallSnapshot) {
                    isInstallSnapshot = false;
                    installSnapshotMutex.unlock();
                }
            } else if (message.type === 'appendEntry') {
                //if i not found the leader was loss ignore
                if (state.leaderId != 0 && state.leaderId != data.id) {
                    log.warn("follower got appendEntry no my leaderId", "data:", data, "self:", state);
                    return
                }

                // Ignore an older term
                if (state.term > data.term) {
                    log.warn("appendEntry ignore an older term:", data.term, "commitId:", data.commitId, "self:", state);
                    return;
                }

                observer.resetTimer();
                state.leaderId = data.id;
                state.term = data.term;
                if (state.prevLogIndex != DEFAULT_DATA) {
                    if (state.prevLogTerm == data.prevLogTerm && state.prevLogIndex == data.prevLogIndex) {
                        state.commitId++;
                        state.prevLogIndex = state.commitId;
                        state.prevLogTerm = data.term;
                        manager.setUpdatedData(data.data);
                        delete data.data;
                        log.debug("appendEntry last commitId:", state.commitId, "data.commitId:", data.commitId, "from:", data.id, "self:", JSON.stringify(state), "data:", JSON.stringify(data));
                        return;
                    }
                }
                log.warn("appendEntry warn commitId:", state.commitId, "data.commitId:", data.commitId, "from:", data.id, "self:", JSON.stringify(state), "data:", JSON.stringify(data));
                installSnapshot(data);
            } else if (message.type === 'heartbeart') {
                //if i not found the leader was loss ignore
                if (state.leaderId != 0 && state.leaderId != data.id) {
                    log.warn("follower got heartbeart no my leaderId", "data:", data, "self:", state);
                    return
                }

                // Ignore an older term
                if (state.term > data.term) {
                    log.warn("follower heartbeart ignore an older term:", data.term, "commitId:", data.commitId, "self:", state);
                    return;
                }

                if (state.prevLogIndex != DEFAULT_DATA) {
                    let prevLogTerm = state.prevLogTerm;
                    if (state.prevLogIndex == data.prevLogIndex) {
                        prevLogTerm = data.prevLogTerm
                    }
                    if (prevLogTerm > data.prevLogTerm) {
                        log.warn("follower heartbeart ignore revious log term mis-match prevLogTerm:", data.prevLogTerm, "commitId:", data.commitId, "self:", state);
                        return;
                    }
                }

                observer.resetTimer();
                let oldLeaderId = state.leaderId;
                state.leaderId = data.id;
                state.term = data.term;
                if (oldLeaderId != state.leaderId) {
                    log.debug("follower found new leader last commitId:", state.commitId, "data.commitId:", data.commitId, "from:", data.id, "data:", JSON.stringify(data));
                    return;
                }
            } else if (message.type === 'requestVote') {
                //if i not found the leader was loss ignore
                if (state.leaderId != 0 && state.leaderId != data.id) {
                    log.warn("follower rejecting vote request since we have a leader", "data:", data, "self:", state);
                    responseVote(state.term, data.id, state.commitId, data.term, false);
                    return
                }

                // ignore an older term
                if (data.term < state.term) {
                    log.warn("follower ignore an older term", "data:", data, "self:", state);
                    responseVote(state.term, data.id, state.commitId, data.term, false);
                    return;
                }

                //found a new term change myself term next hearbeat time i will choice you
                if (data.term > state.term) {
                    log.warn("follower receive a newer term data:", data, "self:", state);
                    state.term = data.term;
                }

                //check if we've voted in this election before
                if (state.lastVoteFor != DEFAULT_DATA && state.lastVoteTerm === data.term) {
                    log.warn("follower duplicate requestVote for same term:", data.term, "data:", data, "self:", state);
                    if (state.lastVoteFor == data.id) {
                        responseVote(state.term, data.id, state.commitId, data.term, true);
                    }
                    return
                }

                //ignore the older node to become leader
                if (state.prevLogIndex != DEFAULT_DATA) {
                    let prevLogTerm = state.prevLogTerm;
                    if (state.prevLogIndex == data.prevLogIndex) {
                        prevLogTerm = data.prevLogTerm
                    }
                    if (prevLogTerm > data.prevLogTerm) {
                        log.warn("follower rejecting vote request since our prevLogTerm is greater", "data:", data, "self:", state);
                        responseVote(state.term, data.id, state.commitId, data.term, false);
                        return;
                    }

                    if (state.prevLogTerm <= data.prevLogTerm && state.commitId > data.commitId) {
                        log.warn("follower rejecting vote request since our commitId is greater", "data:", data, "self:", state);
                        responseVote(state.term, data.id, state.commitId, data.term, false);
                        return;
                    }
                }

                state.lastVoteTerm = data.term;
                state.lastVoteFor = data.id;
                state.term = data.term;
                log.warn(`follower vote for other data:`, data, "self:", state);
                responseVote(state.term, data.id, state.commitId, data.term, true);
                observer.resetTimer();
            } else {
                log.debug('follower, not concerned message:', message, "routingKey:", routingKey, "self:", state);
            }
        } catch (e) {
            log.fatal(util.inspect(e));
        } finally {
            mutex.unlock();
        }
    };

    log.info(state.id, 'Run as follower leaderId:', state.leaderId, "self:", state);
    return new Promise((resolve) => {
        topicChannel.subscribe(routerKeys, onTopicMessage, function() {
            broadcastOnTopicMsg = onTopicMessage;
            observer.resetTimer();
            log.info(state.id, 'Run as follower success leaderId:', state.leaderId, "self:", state);
            resolve();
        });
    });
};

var runAsLeader = function(topicChannel, manager) {
    state.role = "leader";
    state.timestamp = (new Date()).getTime();

    log.info('Run as leader id:', state.id, "self:", state);
    var interval;
    var routerKeys = ["clusterManager.leader", `clusterManager.leader.${state.id}`];
    var observer = new HeartbeatObserver(async(loseTime, lastContact) => {
        await mutex.lock('leader.HeartbeatObserver');
        try {
            if (state.role !== "leader") {
                log.warn('leader cycle in HeartbeatObserver', "self:", state);
                return;
            }
            log.fatal("leader Lose heart-beat from self:", state.id, "loseTime:", loseTime, 'lastContact:', lastContact, "timeout:", `${loseTime.getTime() - lastContact.getTime()}ms`);
            await changeRole('follower');
        } catch (e) {
            log.fatal(util.inspect(e));
        } finally {
            mutex.unlock();
        }
    });

    var changeRole = async function(newRole) {
        if (state.role !== "leader") {
            log.warn('leader cycle in changeRole', "self:", state);
            return;
        }

        state.leaderId = 0;
        state.role = newRole;
        observer.stop();
        interval && clearInterval(interval);
        manager.unserve();
        await topicChannel.unsubscribe(routerKeys);
        await topicChannel.bus.removeRpcServer();
        log.info(`leader->${state.role}`);
        switch (newRole) {
            case "candidate":
                await runAsCandidate(topicChannel, manager);
                break;
            case "follower":
                await runAsFollower(topicChannel, manager);
        }
    }

    var needHandle = function(message) {
        if (message.type === 'heartbeart') {
            let data = message.data;
            //statistics delay
            if (data.id === state.id) {
                let current = (new Date()).getTime();
                let delay = current - data.timestamp;
                if (delay >= state.maxDelay) {
                    log.fatal(`[leader.${state.id}] got hear beat from [leader:${data.id}]: ${new Date(data.timestamp)}, delay: ${delay}ms`);
                } else {
                    log.debug(`[leader.${state.id}] got hear beat from [leader:${data.id}]: ${new Date(data.timestamp)}, delay: ${delay}ms`);
                }
            } else if (data.term < state.term) {
                return false;
            }
        }
        return true;
    }

    var onTopicMessage = async function(message) {
        if (!needHandle(message)) {
            return false;
        }
        await mutex.lock(`leader.onTopicMessage.${message.type}`, message);
        try {
            if (state.role != "leader") {
                log.warn('leader cycle in onTopicMessage', "self:", state);
                return;
            }

            if (message.type === 'installSnapshot') {
                let followerId = message.data.id;
                if (followerId === state.id) {
                    log.error('installSnapshot follower:', followerId, "is yourself");
                    return;
                }

                let leaderId = message.data.leaderId;
                if (leaderId !== state.id) {
                    log.error('installSnapshot follower:', followerId, "leaderId:", leaderId, "no yourself:", state.id);
                    return;
                }

                manager.getRuntimeData(function(data) {
                    log.info('installSnapshot follower:', followerId);
                    data = Object.assign({ data, followerId }, state);
                    topicChannel.publish(`clusterManager.follower.${followerId}`, { type: 'installSnapshot', data: data });
                });
            } else if (message.type === 'heartbeart') {
                if (message.data.id !== state.id) {
                    log.fatal('!!Double Leader!! ', "data:", message.data, "self:", state);
                    if (message.data.term > state.term) {
                        log.fatal('Another leader is more senior than me, I quit.');
                        state.term = message.data.term;
                        await changeRole("follower");
                    }
                } else {
                    observer.resetTimer();
                }
            }
        } catch (e) {
            log.fatal(util.inspect(e));
        } finally {
            mutex.unlock();
        }
    };

    var sendHeartbeat = function() {
        state.timestamp = (new Date()).getTime();
        topicChannel.publish('clusterManager.leader', { type: 'heartbeart', data: state });
        topicChannel.publish('clusterManager.follower', { type: 'heartbeart', data: state });
        topicChannel.publish('clusterManager.candidate', { type: 'heartbeart', data: state });
    }

    var leaderLoop = function() {
        sendHeartbeat();
        //start heartbeat
        interval && clearInterval(interval);
        interval = setInterval(function() {
            sendHeartbeat();
        }, state.minHeartTicket);
    }

    leaderLoop();
    return new Promise((resolve) => {
        topicChannel.subscribe(routerKeys, onTopicMessage, function() {
            broadcastOnTopicMsg = onTopicMessage;
            topicChannel.bus.asRpcServer(manager.name, manager.rpcAPI, (rpcSvr) => {
                log.info('Run as asRpcServer.');
                topicChannel.bus.asMonitoringTarget(function(monitoringTgt) {
                    log.info('Cluster leader is in service as leader!');
                    //start service
                    manager.serve(monitoringTgt);

                    //sync data to others
                    manager.registerDataUpdate(function(data) {
                        state.commitId++;
                        log.debug("registerDataUpdate last commitId:", state.commitId)
                        data = Object.assign({}, { data }, state);
                        topicChannel.publish('clusterManager.follower', { type: 'appendEntry', data: data });
                        state.prevLogIndex = state.commitId;
                        state.prevLogTerm = state.term;
                    });
                    log.info('Run as leader success id:', state.id);
                    observer.resetTimer();
                    resolve();
                }, function(reason) {
                    log.error('Cluster leader running as monitoring target failed, reason:', reason);
                    process.kill(process.pid, 'SIGINT');
                });
            }, function(reason) {
                log.error('Cluster leader running as RPC server failed, reason:', util.inspect(reason));
                process.kill(process.pid, 'SIGINT');
            });
        });
    });
};

var runAsCandidate = function(topicChannel, manager) {
    var isStop = false;
    var timer;
    var routerKeys = ['clusterManager.candidate', `clusterManager.candidate.${state.id}`];

    var init = function() {
        state.leaderId = 0;
        state.role = "candidate";
        state.term++;
        state.lastVoteFor = DEFAULT_DATA;
        state.voteNum = 0;
        state.voters = new Set();
        state.timestamp = (new Date()).getTime();
        isStop = false;
    }

    var requestVote = function() {
        log.debug('candidate Send requestVote..', "self:", state);
        topicChannel.publish('clusterManager.follower', { type: 'requestVote', data: state });
        topicChannel.publish('clusterManager.candidate', { type: 'requestVote', data: state });
    };

    var responseVote = function(term, id, commitId, voteForTerm, voteGranted) {
        log.debug('candidate Send responseVote..', "term:", term, "id:", id, "commitId:", commitId, "voteForTerm:", voteForTerm, "voteGranted:", voteGranted, "self:", state);
        topicChannel.publish(`clusterManager.candidate.${id}`, { type: 'responseVote', data: { voteGranted, term, id, commitId, voteForTerm, voter: state.id, voterRole: "candidate" } });
    }

    var electTimeout = async function(msg) {
        await mutex.lock('candidate.electTimeout');
        try {
            if (isStop) {
                return;
            }
            log.debug(`${msg}->electTimeout`);
            await changeRole('candidate', state.term);
        } catch (e) {
            log.fatal(util.inspect(e))
        } finally {
            mutex.unlock();
        }
    };

    var timerIndex = 0;
    var resetTimer = function(msg) {
        if (isStop) {
            return;
        }
        msg = `${msg}->resetTimer:${timerIndex++}`;
        log.debug(msg);
        clearTimer(msg);
        let mixElecTimeout = parseInt(state.electTimeout);
        let timeout = Math.random() * (state.electTimeout * 2 - mixElecTimeout) + mixElecTimeout;
        timer = setTimeout(() => {
            electTimeout(msg);
        }, timeout);
    };

    var clearTimer = function(msg) {
        log.debug(`${msg}->clearTimer`)
        timer && clearTimeout(timer);
        timer = undefined;
    };

    var stopTimer = function(msg) {
        if (!isStop) {
            isStop = true;
        }
        msg = `${msg}->stopTimer`
        clearTimer(msg, isStop);
    }

    var changeRole = async function(newRole, newTerm) {
        if (state.role != "candidate") {
            log.warn('candidate cycle in changeRole', "self:", state);
            return;
        }

        stopTimer(`changeRole candidate->${newRole}`);
        log.info(`candidate->${newRole}`, "self:", state);
        if (newRole == "candidate") {
            init();
            requestVote();
            resetTimer("requestVote");
            return;
        }

        state.leaderId = 0;
        state.term = newTerm;
        state.role = newRole;
        await topicChannel.unsubscribe(routerKeys);
        switch (newRole) {
            case "candidate":
                await runAsCandidate(topicChannel, manager);
                break;
            case "follower":
                await runAsFollower(topicChannel, manager);
                break;
            case "leader":
                await runAsLeader(topicChannel, manager);
                break;
        }
    }

    var onTopicMessage = async function(message) {
        await mutex.lock(`candidate.onTopicMessage.${message.type}`, message);
        try {
            if (isStop) {
                log.warn('candidate cycle in is stop', "self:", state);
                return;
            }

            if (state.role != "candidate") {
                log.warn('candidate cycle in onTopicMessage', "self:", state);
                return;
            }

            let data = message.data;
            if (message.type === 'requestVote') {
                // ignore an older term
                if (data.term < state.term) {
                    log.warn("candidate ignore an older term", "data:", data, "self:", state);
                    responseVote(state.term, data.id, state.commitId, data.term, false);
                    return;
                }

                if (data.term > state.term) {
                    log.warn("candidate receive a newer term i run as follower", "data:", data, "self:", state);
                    await changeRole("follower", data.term);
                }

                //check if we've voted in this election before
                if (state.lastVoteFor != DEFAULT_DATA && state.lastVoteTerm === data.term) {
                    log.warn("candidate duplicate requestVote for same term:", data.term, "data:", data, "self:", state);
                    if (state.lastVoteFor == data.id) {
                        responseVote(state.term, data.id, state.commitId, data.term, true);
                    } else {
                        responseVote(state.term, data.id, state.commitId, data.term, false);
                    }
                    return
                }

                //ignore the older node to become leader
                if (state.prevLogIndex != DEFAULT_DATA) {
                    let prevLogTerm = state.prevLogTerm;
                    if (state.prevLogIndex == data.prevLogIndex) {
                        prevLogTerm = data.prevLogTerm
                    }

                    if (prevLogTerm > data.prevLogTerm) {
                        log.warn("candidate rejecting vote request since our prevLogTerm is greater", "data:", data, "self:", state);
                        responseVote(state.term, data.id, state.commitId, data.term, false);
                        return;
                    }

                    if (state.prevLogTerm <= data.prevLogTerm && state.commitId > data.commitId) {
                        log.warn("candidate rejecting vote request since our commitId is greater", "data:", data, "self:", state);
                        responseVote(state.term, data.id, state.commitId, data.term, false);
                        return;
                    }
                }

                state.lastVoteTerm = data.term;
                state.lastVoteFor = data.id;
                state.term = data.term;
                let voteFor = "other";
                if (data.id == state.id) {
                    voteFor = "self";
                }
                log.warn(`candidate vote for ${voteFor} data:`, data, "self:", state);
                responseVote(state.term, data.id, state.commitId, data.term, true);
                resetTimer("responseVote");
            } else if (message.type === 'responseVote') {
                if (data.term > state.term && data.id == state.id && data.voteForTerm == state.term) {
                    resetTimer("responseVote change as follower");
                    log.warn("candidate newer term discovered, fallback to follower", "data:", data, "self:", state);
                    await changeRole("follower", data.term);
                    return;
                }

                //got some one vote for me
                if (data.voteGranted && data.voteForTerm == state.term && data.id == state.id && !state.voters.has(data.voter)) {
                    state.voters.add(data.voter);
                    if ((++state.voteNum) > parseInt(state.totalNode / 2)) {
                        resetTimer("responseVote got vote");
                        log.info(`candidate got vote:${state.voteNum} run as leader`, "data:", data, "self:", state);
                        await changeRole("leader", state.term);
                    }
                }
            } else if (message.type === 'heartbeart') {
                //ignore the older node to become leader
                if (state.prevLogIndex != DEFAULT_DATA) {
                    let prevLogTerm = state.prevLogTerm;
                    if (state.prevLogIndex == data.prevLogIndex) {
                        prevLogTerm = data.prevLogTerm
                    }

                    if (prevLogTerm > data.prevLogTerm) {
                        log.warn("candidate rejecting run as follower since our prevLogTerm is greater", "data:", data, "self:", state);
                        return;
                    }

                    if (state.prevLogTerm <= data.prevLogTerm && state.commitId > data.commitId) {
                        log.warn("candidate rejecting run as follower commitId is greater", "data:", data, "self:", state);
                        return;
                    }
                }
                await changeRole("follower", data.term);
            }
        } catch (e) {
            log.fatal(util.inspect(e));
        } finally {
            mutex.unlock();
        }
    };

    init();
    log.info(state.id, 'Run as candidate.', "self:", state);
    return new Promise((resolve) => {
        topicChannel.subscribe(routerKeys, onTopicMessage, function() {
            broadcastOnTopicMsg = onTopicMessage;
            requestVote();
            resetTimer("requestVote");
            log.info(state.id, "Run as candidate success", "self:", state);
            resolve();
        });
    });
};

var broadcastHandler = function(message) {
    broadcastOnTopicMsg ? broadcastOnTopicMsg(message) : null;
}

exports.manager = function (topicChannel, clusterName, id, spec) {
    var that = {};
    var manager;

    that.run = function (topicChannel){
        manager = new ClusterManager(clusterName, id, spec);
        topicChannel.subscribe(['clusterManager.broadcast'], broadcastHandler, () => {
            state.id = manager.id;
            state.totalNode = manager.totalNode || 1;
            state.electTimeout = manager.electTimeout || 1000;
            state.minHeartTicket = parseInt(manager.electTimeout / 10);
            if (state.minHeartTicket > 100) {
                state.minHeartTicket = 100;
            }
            runAsFollower(topicChannel, manager);
        });
    }

    that.leave = function () {
        manager.stopCluster();
    }

    return that;
};
exports.ClusterManager = ClusterManager;
