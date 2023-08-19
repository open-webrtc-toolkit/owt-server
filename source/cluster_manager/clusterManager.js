// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
const util = require('util');
var logger = require('./logger').logger;
var Scheduler = require('./scheduler').Scheduler;

// Logger
var log = logger.getLogger('ClusterManager');
var Url = require("url");

var ClusterManager = function (clusterName, selfId, spec) {
    var that = {name: clusterName, id: selfId, totalNode:spec.totalNode};

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
            /**
             * keycoding 20230811
             * 1、when find the agent was loss just set it's state to 0
             * 2、notify to follower which agent was loss
             * 3、the agent which was loss may be connect in the loop time
             */
            if (workers[worker].alive_count >= 3) reportState(worker, 0);
            if (workers[worker].alive_count === 3 || workers[worker].alive_count % 300 === 0) {
                log.info(`Agent ${worker} is not alive any longer(${workers[worker].alive_count})`);
            }
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
        var purpose = workers[worker] ? workers[worker].purpose : undefined;
        if (workers[worker] && schedulers[workers[worker].purpose]) {
            schedulers[workers[worker].purpose].remove(worker);
            monitoringTarget && monitoringTarget.notify('quit', {purpose: workers[worker].purpose, id: worker, type: 'worker'});
            delete workers[worker];
            data_synchronizer && data_synchronizer({type: 'worker_quit', payload: {worker: worker}});
        }

        if (purpose) {
            clusterInfo[purpose] && clusterInfo[purpose].delete(worker);
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
            /**
             * keycoding 20230811
             * 1、recover the agent which was loss before just set it's state to 2
             * 2、notify to follower agent was found
             */
            if (workers[worker].alive_count >= 3) {
                log.info(`Agent ${worker} is recover(${workers[worker].alive_count})`);
            }
            let scheduler = schedulers[workers[worker].purpose];
            if (scheduler && !scheduler.isAlive(worker)) {
                reportState(worker, 2);
            }
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
                schedulers[purpose].schedule(task, preference, reserveTime, function (worker, info) {
                    log.debug('schedule OK, got  worker', worker);
                    on_ok(worker, info);
                    data_synchronizer && data_synchronizer({type: 'scheduled', payload: {purpose: purpose, task: task, worker: worker, reserve_time: reserveTime}});
                }, function (reason) {
                    log.warn('schedule failed, purpose:', purpose, 'task:', task, 'reason:', reason);
                    on_error(`Failed in scheduling purpose: ${purpose} for task: ${task}, reason: ${reason}`);
                });
            } else {
                log.warn(`No scheduler for purpose: ${purpose} for task: ${task}`);
                on_error(`No scheduler for purpose: ${purpose} for task: ${task}`);
            }
        } else {
            log.warn(`Failed in scheduling purpose: ${purpose} for task ${task}, reason: cluster manager is not ready.`);
            on_error(`Failed in scheduling purpose: ${purpose} for task ${task}, reason: cluster manager is not ready.`);
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
        /**
         * keycoding 20230819
         * 1、return the worker when it's state==2
         */
        let on_ok_check = (worker)=>{
            let scheduler = schedulers[purpose];
            if(!scheduler || !scheduler.isAlive(worker)){
                log.error("work:",worker,"for task:",task,"is no available");
                on_error(`NO_AVAILABLE_WORKER[${worker}]_TASK[${task}]`);
            }else{
                on_ok(worker);
            }
        }
        if (schedulers[purpose]) {
            schedulers[purpose].getScheduled(task, on_ok_check, on_error);
        } else {
            log.error(`Invalid purpose: ${purpose} for task: ${task}`);
            on_error(`Invalid purpose: ${purpose} for task: ${task}`);
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

    var interval = undefined;
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
        interval && clearInterval(interval);
        interval = setInterval(checkAlive, check_alive_period);
        for (var purpose in schedulers) {
            schedulers[purpose].serve();
        }
    };

    /**
     * keycoding 20230811
     * 1、stop service when it was become follower or candidate
     */
    that.unserve = function () {
        state = 'initializing'
        monitoringTarget = undefined;
        interval && clearInterval(interval);
        is_freshman = true;
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

/**
 * keycoding 20230811
 * 1、avoid double master by raft
 * 2、force to sync runtime data from the new leader
 * 3、it might be the leader own network failure,so the leader should supervise itself to found itself was loss for others
 */
class HeartbeatObserver {
    constructor(onLoss) {
        this.interval = undefined;
        this.lastContact = new Date();
        this.state = state;
        this.onLoss = onLoss;
    }

    resetTimer () {
        let self = this;
        let loseTime = undefined;
        this.lastContact = new Date();
        self.interval && clearInterval(self.interval);
        self.interval = setInterval( function () {
            loseTime = new Date();
            if (loseTime-self.lastContact > 100  && self.interval) {
                self.interval && clearInterval(self.interval);
                self.interval = undefined;
                self.onLoss(loseTime,self.lastContact);
            }
        }, 30);
    };

    stop(){
        let self = this;
        self.interval && clearInterval(self.interval);
        self.interval = undefined;
    }
}

var state = {
    term:0, // current term
    leaderId:0, // candidate and follower found leaderId
    commitId:0,
    lastVoteTerm:-1, // candidate last vote for other candidate term
    lastVoteFor: -1, // candidate last vote for other candidate.id
    voteNum:0, //candidate laste got vote from other candidate
    totalNode:0,
    prevLogIndex:-1,// last log entry index
    prevLogTerm:-1, // last log entry term
    voters:new Set(),
    role:"follower"
};

var leaderMsgHandler = null;

var runAsFollower = function (topicChannel, manager) {
    state.role = "follower";

    var foundLeader,isRunning;
    var routerKeys = [`clusterManager.follower.${manager.id}`];
    var observer = new HeartbeatObserver(async (loseTime,lastContact)=>{
        if(state.role  !== "follower"){
            log.warn('cycle in runAsFollower.HeartbeatObserver:', state.role );
            return;
        }

        log.info("Lose heart-beat from leader:",state.leaderId,"loseTime:", loseTime, 'lastContact:', lastContact);
        state.role = "candidate";
        await topicChannel.unsubscribe(routerKeys);
        log.info("follower->candidate");
        await runAsCandidate(topicChannel, manager);
    });

    var syncRuntimeData = async function () {
        if(!isRunning){
            isRunning = true;
            log.info("syncRuntimeData","self:",state);
            topicChannel.publish(`clusterManager.leader.${state.leaderId}`, {type: 'syncRuntimeData', data: state});
            setTimeout(()=>{
                isRunning = false;
            },200);
        }
    };

    var onTopicMessage = async function (message, routingKey) {
        let data = message.data;
        if (message.type === 'syncRuntimeData') {
            if(data.id !== state.leaderId){
                log.error('syncRuntimeData failed leaderId:',data.id,"my leaderId:",state.leaderId);
                return;
            }

            if(data.followerId !== manager.id){
                log.error('syncRuntimeData failed followerId:',data.followerId,"my id:",manager.id);
                return;
            }

            if(data.commitId < state.commitId){
                log.error('syncRuntimeData failed commitId:',data.commitId,"my commitId:",state.commitId);
                return;
            }

            log.info("syncRuntimeData success", "term:", data.term, "commitId:", data.commitId, "self:", state);
            observer.resetTimer();
            state.commitId = data.commitId;
            state.term = data.term;
            state.prevLogIndex = data.commitId;
            state.prevLogTerm = data.term;
            manager.setRuntimeData(data.data);
            isRunning = false;
        } else if (message.type === 'appendEntry') {
            if(data.id !== state.leaderId){
                log.error('appendEntry failed leaderId:',data.id,"my leaderId:",state.leaderId);
                return;
            }

            observer.resetTimer();
            if(state.prevLogIndex == data.prevLogIndex && (state.prevLogTerm === data.prevLogTerm || state.prevLogTerm == data.term)){
                state.commitId++;
                state.prevLogIndex = state.commitId;
                state.prevLogTerm = data.term;
                manager.setUpdatedData(data.data);
            }else{
                log.warn('appendEntry warn, prevLogIndex:',data.prevLogIndex,"prevLogTerm:",data.prevLogTerm,'my prevLogIndex:',state.prevLogIndex,"my prevLogTerm:",state.prevLogTerm);
                await syncRuntimeData();
            }
        } else if (message.type === 'heartbeart'){
            if((data.id !== state.leaderId)){
                log.error('heartbeart failed leaderId:',data.id,"my leaderId:",state.leaderId);
                return;
            }

            observer.resetTimer();
            state.leaderId = data.id;
            state.term = data.term;
            if(!foundLeader){
                foundLeader = true;
                (data.id != manager.id) && await syncRuntimeData();
            }
        }
        else {
            log.info('follower, not concerned message:', message, "routingKey:", routingKey, "self:",state);
        }
    };

    log.info(manager.id,'Run as follower leaderId:',state.leaderId);
    return new Promise((resolve) => {
        topicChannel.subscribe(routerKeys, onTopicMessage, function () {
            leaderMsgHandler = onTopicMessage;
            observer.resetTimer();
            log.info(manager.id,`Run as follower success leaderId:`,state.leaderId);
            resolve();
        });
    });
};

var runAsLeader = function (topicChannel, manager) {
    state.role = "leader"
    state.leaderId = manager.id;
    state.lastVoteFor = state.leaderId;
    state.lastVoteTerm = state.term;

    log.info('Run as leader id:',manager.id);
    var interval;
    var routerKeys = [`clusterManager.leader.${manager.id}`];
    var observer = new HeartbeatObserver(async (loseTime,lastContact)=>{
        log.info("Lose heart-beat from leader:",state.leaderId,"loseTime:", loseTime, 'lastContact:', lastContact);
        await changeRole('candidate');
    });

    var changeRole = async function(newRole){
        if(state.role !== "leader"){
            log.warn('cycle in runAsLeader', state.role);
            return;
        }
        state.role = newRole;
        observer.stop();
        interval && clearInterval(interval);
        manager.unserve();
        await topicChannel.unsubscribe(routerKeys);
        await topicChannel.bus.removeRpcServer();
        log.info(`leader->${state.role}`);
        switch(newRole){
            case "candidate":
                await runAsCandidate(topicChannel, manager);
                break;
            case "follower":
                await runAsFollower(topicChannel, manager);
        }
    }

    return new Promise((resolve) => {
        topicChannel.bus.asRpcServer(manager.name, manager.rpcAPI, (rpcSvr) => {
            log.info('Run as asRpcServer.');
            topicChannel.bus.asMonitoringTarget(function (monitoringTgt) {
                manager.serve(monitoringTgt);
                interval && clearInterval(interval);
                interval = setInterval(function () {
                    topicChannel.publish('clusterManager.broadcast', {type:'heartbeart', data:state});
                }, 20);

                var onTopicMessage = async function (message) {
                    if (state.role != "leader") {
                        log.warn('cycle in master change role:', state.role);
                        return;
                    }
                    if (message.type === 'syncRuntimeData') {
                        let followerId = message.data.id;
                        if (followerId === manager.id) {
                            log.error('syncRuntimeData follower:', followerId, "is yourself");
                            return;
                        }

                        let leaderId = message.data.leaderId;
                        if (leaderId !== manager.id) {
                            log.error('syncRuntimeData follower:', followerId, "leaderId:",leaderId, "no yourself:",manager.id);
                            return;
                        }

                        manager.getRuntimeData(function (data) {
                            log.info('syncRuntimeData follower:', followerId);
                            data = Object.assign({data,followerId}, state);
                            topicChannel.publish(`clusterManager.follower.${followerId}` , {type: 'syncRuntimeData', data: data});
                        });
                    } else if (message.type === 'heartbeart') {
                        if(message.data.id !== manager.id){
                            log.error('!!Double Leader!! ',"data:",JSON.stringify(message.data),"self:", state);
                            if (message.data.term > state.term && message.data.commitId > state.commitId) {
                                log.error('Another leader is more senior than me, I quit.');
                                state.term = message.data.term;
                                state.leaderId = message.data.id;
                                await changeRole("follower");
                            }
                        }else{
                            observer.resetTimer();
                        }
                    }
                };

                topicChannel.subscribe(routerKeys, onTopicMessage, function () {
                    log.info('Cluster leader is in service as leader!');
                    leaderMsgHandler = onTopicMessage;
                    manager.registerDataUpdate(function (data) {
                        state.commitId++;
                        data = Object.assign({},{data}, state);
                        topicChannel.publish('clusterManager.broadcast', {type: 'appendEntry', data: data});
                        state.prevLogIndex = state.commitId;
                        state.prevLogTerm = state.term;
                    });
                    log.info('Run as leader success id:',manager.id);
                    observer.resetTimer();
                    resolve();
                });

            }, function (reason) {
                log.error('Cluster leader running as monitoring target failed, reason:', reason);
                process.kill(process.pid, 'SIGINT');
            });
        }, function (reason) {
            log.error('Cluster leader running as RPC server failed, reason:', util.inspect(reason));
            process.kill(process.pid, 'SIGINT');
        });
    });
};

var runAsCandidate = function (topicChannel, manager) {
    state.role = "candidate";
    state.term++;
    state.lastVoteTerm = -1 ;
    state.lastVoteFor = -1;
    state.voteNum = 0;
    state.voters = new Set();
    let mixElecTimeout = 150;
    let maxElecTimeout = 300;

    var timeout = Math.random() * (maxElecTimeout-mixElecTimeout) + mixElecTimeout,timer;
    var routerKeys = ['clusterManager.candidate.#'];

    var requestVote = function () {
        log.debug('Send requestVote..', "self:",state);
        topicChannel.publish('clusterManager.candidate', {type: 'requestVote', data: state});
    };

    var responseVote = function(term, id,commitId){
        log.debug('Send responseVote..',"term:",term,"id:",id,"commitId:",commitId, "self:",state);
        topicChannel.publish('clusterManager.candidate', {type: 'responseVote', data: {term,  id, commitId, voter:manager.id}});
    }

    var electTimeout = async function () {
        return changeRole('candidate',state.term);
    };

    var changeRole = async function(newRole,newTerm){
        timer && clearTimeout(timer);
        timer = undefined;
        if (state.role != "candidate") {
            log.warn('cycle in changeRole:', state.role, "self:",state);
            return;
        }

        state.term = newTerm;
        state.role = newRole;
        log.info(`candidate->${newRole}`,"self:",state);
        await topicChannel.unsubscribe(routerKeys);
        switch(newRole){
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
        return;
    }

    var onTopicMessage = async function (message) {
        if (state.role != "candidate") {
            log.warn('cycle in onTopicMessage:', state.role, "self:",state);
            return;
        }

        let data = message.data;
        if (message.type === 'requestVote') {
            // ignore an older term
            if(data.term < state.term){
                log.warn("ignore an older term",  "data:",data,"self:",state);
                return;
            }

            // ignore the older follower to become leader
            if(state.prevLogTerm > data.prevLogTerm){
                log.warn("rejecting vote request since our last term is greater", "data:", data, "self:", state)
                return;
            }

            // ignore the older follower to become leader
            if(state.prevLogTerm == data.prevLogTerm && state.commitId > data.commitId){
                log.warn("rejecting vote request since our last index is greater", "data:", data, "self:", state);
                return;
            }

            //ignore to vote twice
            if(state.lastVoteFor == data.id && state.lastVoteTerm == data.term){
                log.warn("duplicate requestVote for",  "data:", data, "self:", state);
                return
            }

            //vote for self
            if(data.id == manager.id && state.term == data.term){
                log.warn("vote for yourself",  "self:", state);
                state.lastVoteTerm = data.term;
                state.lastVoteFor = manager.id;
                responseVote(data.term, data.id, data.commitId);
                return
            }

            //vote for other who is newer and data is new than me
            if(data.term > state.term && data.commitId >= state.commitId){
                state.lastVoteTerm = data.term;
                state.lastVoteFor = data.id;
                log.warn("vote for another",  "data:",data, "self:", state);
                responseVote(data.term, data.id, data.commitId);
                return;
            }


            responseVote( state.term, manager.id, state.commitId);
            // only left the candidate to vote,it means many node found leader is loss
            //if(data.term > term){
            //    log.info(`found other candidate term:${data.term} bigger then me:${term}`)
            //    await changeRole("follower", data.term);
            //}
        }else if(message.type === 'responseVote'){
            if(data.term > state.term && data.commitId >= state.commitId){
                log.warn("some one term is bigger then me i run as follower",  "data:", data, "self:", state);
                state.leaderId = data.id;
                await changeRole("follower", data.term);
                return;
            }

            //got some one vote for me
            if(data.id == manager.id && !state.voters.has(data.voter)){
                state.voters.add(data.voter);
                if( (++state.voteNum) > parseInt(state.totalNode/2)){
                    log.info(`got vote:${state.voteNum} run as leader`,"self:", state);
                    await changeRole("leader", state.term);
                }
            }
        } else if (message.type === 'heartbeart') {
            //found some one is run as leader, whatever i run as follower
            state.leaderId = data.id;
            log.info(`found leader`,"data:",data,"self:", state);
            await changeRole("follower", data.term);
        }
    };

    log.info(manager.id,'Run as candidate.');
    return new Promise((resolve) => {
        topicChannel.subscribe(routerKeys, onTopicMessage, function () {
            leaderMsgHandler = onTopicMessage;
            requestVote();
            timer && clearTimeout(timer);
            timer = setTimeout(electTimeout, timeout);
            log.info(manager.id,"Run as candidate success");
            resolve();
        });
    });
};

var broadcastHandler = function(message){
    leaderMsgHandler ? leaderMsgHandler(message) :null;
}

exports.manager = function (topicChannel, clusterName, id, spec) {
    var that = {};
    var manager;

    that.run = function (topicChannel){
        manager = new ClusterManager(clusterName, id, spec);
        topicChannel.subscribe(['clusterManager.broadcast.#'], broadcastHandler, () => {
            state.id = manager.id;
            state.totalNode = manager.totalNode;
            runAsCandidate(topicChannel, manager);
        });
    }

    that.leave = function () {
        manager.stopCluster();
    }

    return that;
};

exports.ClusterManager = ClusterManager;
