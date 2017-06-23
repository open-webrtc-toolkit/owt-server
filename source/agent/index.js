/*global require, global, process*/
'use strict';
var Getopt = require('node-getopt');
var spawn = require('child_process').spawn;
var fs = require('fs');
var toml = require('toml');
var log = require('./logger').logger.getLogger('ErizoAgent');

var config;
try {
  config = toml.parse(fs.readFileSync('./agent.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

var loadConfig = {};
try {
  loadConfig = require('./loader.json');
} catch (e) {
  log.info('No loader.json found.');
}

// Configuration default values
global.config = config || {};

global.config.agent = global.config.agent || {};
global.config.agent.maxProcesses = global.config.agent.maxProcesses || 13;
global.config.agent.prerunProcesses = global.config.agent.prerunProcesses || 2;

global.config.cluster = global.config.cluster || {};
global.config.cluster.name = global.config.cluster.name || 'woogeen-cluster';
global.config.cluster.join_retry = global.config.cluster.join_retry || 60;
global.config.cluster.report_load_interval = global.config.cluster.report_load_interval || 1000;
global.config.cluster.max_load = global.config.cluster.max_load || 0.85;
global.config.cluster.network_max_scale = global.config.cluster.network_max_scale || 1000;

global.config.internal.ip_address = global.config.internal.ip_address || '';
global.config.internal.network_interface = global.config.internal.network_interface || undefined;

global.config.capacity = global.config.capacity || {};

global.config.webrtc = global.config.webrtc || {};
global.config.webrtc.ip_address = global.config.webrtc.ip_address || '';
global.config.webrtc.network_interface = global.config.webrtc.network_interface || undefined;

global.config.recording = global.config.recording || {};
global.config.recording.path = global.config.recording.path || '/tmp';

global.config.video = global.config.video || {};
global.config.video.hardwareAccelerated = !!global.config.video.hardwareAccelerated;
global.config.video.yamiEnabled = !!global.config.video.yamiEnabled;
global.config.video.enableBetterHEVCQuality = !!global.config.video.enableBetterHEVCQuality;

// Parse command line arguments
var getopt = new Getopt([
  ['r' , 'rabbit-host=ARG'            , 'RabbitMQ Host'],
  ['g' , 'rabbit-port=ARG'            , 'RabbitMQ Port'],
  ['l' , 'logging-config-file=ARG'    , 'Logging Config File'],
  ['M' , 'maxProcesses=ARG'           , 'Stun Server URL'],
  ['P' , 'prerunProcesses=ARG'        , 'Default video Bandwidth'],
  ['U' , 'my-purpose=ARG'             , 'Purpose of this agent'],
  ['h' , 'help'                       , 'display this help']
]);

var myId = '';
var myPurpose = 'webrtc';
var myState = 2;
var reuse = true;

var opt = getopt.parse(process.argv.slice(2));

for (var prop in opt.options) {
    if (opt.options.hasOwnProperty(prop)) {
        var value = opt.options[prop];
        switch (prop) {
            case 'help':
                getopt.showHelp();
                process.exit(0);
                break;
            case 'rabbit-host':
                global.config.rabbit = global.config.rabbit || {};
                global.config.rabbit.host = value;
                break;
            case 'rabbit-port':
                global.config.rabbit = global.config.rabbit || {};
                global.config.rabbit.port = value;
                break;
            case 'my-purpose':
                if (value === 'session' ||
                    value === 'webrtc' ||
                    value === 'sip' ||
                    value === 'avstream' ||
                    value === 'recording' ||
                    value === 'audio' ||
                    value === 'video') {
                    myPurpose = value;
                } else {
                    process.exit(0);
                }
                break;
            default:
                global.config.agent[prop] = value;
                break;
        }
    }
}

var clusterWorker = require('./clusterWorker');
var amqper = require('./amqp_client')();
var rpcClient;
var monitoringTarget;

var erizo_index = 0;
var idle_erizos = [];
var erizos = [];
var processes = {};
var tasks = {}; // {erizo_id: {RoomID: [TaskID]}}
var load_collection = {period: global.config.cluster.report_load_interval};
var capacity = global.config.capacity;
var worker;

function cleanupErizoJS (id) {
    delete processes[id];
    delete tasks[id];
    var index = erizos.indexOf(id);
    if (index !== -1) {
        erizos.splice(index, 1);
    }
    index = idle_erizos.indexOf(id);
    if (index !== -1) {
        idle_erizos.splice(index, 1);
    }
}

var launchErizoJS = function() {
    var id = myId + '.' + erizo_index++;
    if (!fs.existsSync('../logs')){
        fs.mkdirSync('../logs');
    }
    var out = fs.openSync('../logs/' + id + '.log', 'a');
    var err = fs.openSync('../logs/' + id + '.log', 'a');
    var execName = 'node';
    if (!process.env.NODE_DEBUG_ERIZO && loadConfig.bin) {
        execName = './' + loadConfig.bin;
    }
    var child = spawn(execName, ['./erizoJS.js', id, myPurpose, JSON.stringify(webrtcInterfaces), clusterIP, myId], {
        detached: true,
        stdio: [ 'ignore', out, err, 'ipc' ]
    });
    child.unref();
    child.out_log_fd = out;
    child.err_log_fd = err;

    log.debug('launchErizoJS, id:', id);
    child.on('close', function (code) {
        log.debug('Node ', id, 'exited with', code);
        //if (code !== 0) {
        if (processes[id]) {
            monitoringTarget && monitoringTarget.notify('abnormal', {purpose: myPurpose, id: id, type: 'node'});
            fs.closeSync(child.out_log_fd);
            fs.closeSync(child.err_log_fd);
            cleanupErizoJS(id);
        }
        var is_recorverable = !(code === -2);//FIXME: there might be other unrecoverable reasons.
        if (is_recorverable) {
            fillErizos();
        } else {
            log.error('Node(', id, ') exited with an unrecoverable code(', code, '), and will no longer try to launch new ones.');
        }
    });
    child.on('error', function (error) {
        log.error('failed to launch worker', id, 'error:', error.code);
        child.READY = false;
        monitoringTarget && monitoringTarget.notify('error', {purpose: myPurpose, id: id, type: 'node'});
        fs.closeSync(child.out_log_fd);
        fs.closeSync(child.err_log_fd);
        cleanupErizoJS(id);
    });
    child.on('message', function (message) { // currently only used for sending ready message from worker to agent;
        log.debug('message from node', id, ':', message);
        if (message === 'READY') {
            child.READY = true;
        } else {
            child.READY = false;
            child.kill();
            fs.closeSync(child.out_log_fd);
            fs.closeSync(child.err_log_fd);
            cleanupErizoJS(id);
        }
    });
    processes[id] = child;
    tasks[id] = {};
    idle_erizos.push(id);
};

var dropErizoJS = function(id) {
    log.debug('dropErizoJS, id:', id);
    if (processes.hasOwnProperty(id)) {
        processes[id].kill();
        cleanupErizoJS(id);
    }
};

var dropAll = function(quietly) {
    Object.keys(processes).map(function (k) {
        dropErizoJS(k);
        !quietly && monitoringTarget && monitoringTarget.notify('abnormal', {purpose: myPurpose, id: k, type: 'node'});
    });
};

var fillErizos = function() {
    for (var i = idle_erizos.length; i < global.config.agent.prerunProcesses; i++) {
        launchErizoJS();
    }
};

var addTask = function(worker, nodeId, task) {
    tasks[nodeId] = tasks[nodeId] || {};
    tasks[nodeId][task.session] = tasks[nodeId][task.session] || [];

    if (tasks[nodeId][task.session].indexOf(task.task) === -1) {
        worker && worker.addTask(task.task);
    }

    tasks[nodeId][task.session].push(task.task);
};

var removeTask = function(worker, nodeId, task, on_last_task_leave) {
    if (tasks[nodeId]) {
        if (tasks[nodeId][task.session]) {
            var i = tasks[nodeId][task.session].indexOf(task.task);
            if (i > -1) {
                tasks[nodeId][task.session].splice(i, 1);
            }

            if (tasks[nodeId][task.session].indexOf(task.task) === -1) {
                worker && worker.removeTask(task.task);
            }

            if (tasks[nodeId][task.session].length === 0) {
                delete tasks[nodeId][task.session];
                if (Object.keys(tasks[nodeId]).length === 0) {
                    on_last_task_leave();
                }
            }
        }
    }
};

var api = function (worker) {
    return {
        getNode: function(task, callback) {
            log.debug('getNode, task:', task);
            var reportCallback = function (id, timeout) {
                var waitForInitialization = function () {
                    if (!processes[id]) {
                        log.warn('process', id, 'not found');
                        return;
                    }
                    if (processes[id].READY === undefined) {
                        log.debug(id, 'not ready');
                        setTimeout(waitForInitialization, timeout);
                        return;
                    }
                    if (processes[id].READY === true) {
                        log.debug(id, 'ready');
                        callback('callback', id);
                        addTask(worker, id, task);
                    }
                };
                waitForInitialization();
            };
            try {
                var room_id = task.session;
                var erizo_id;
                if (reuse) {
                    var i;
                    for (i in erizos) {
                        erizo_id = erizos[i];
                        if (tasks[erizo_id] !== undefined && tasks[erizo_id][room_id] !== undefined) {
                            reportCallback(erizo_id, 100);
                            return;
                        }
                    }

                    for (i in idle_erizos) {
                        erizo_id = idle_erizos[i];
                        if (tasks[erizo_id] !== undefined && tasks[erizo_id][room_id] !== undefined) {
                            reportCallback(erizo_id, 100);
                            return;
                        }
                    }
                }

                erizo_id = idle_erizos.shift();
                reportCallback(erizo_id, 100);

                erizos.push(erizo_id);

                if (reuse && myPurpose !== 'session' && myPurpose !== 'sip' && ((erizos.length + idle_erizos.length + 1) >= global.config.agent.maxProcesses)) {
                    // We re-use Erizos
                    idle_erizos.push(erizos.shift());
                } else {
                    // We launch more processes
                    fillErizos();
                }
            } catch (error) {
                log.error('getNode error:', error);
            }
        },

        recycleNode: function(id, task, callback) {
            log.debug('recycleNode, id:', id, 'task:', task);
            try {
                removeTask(worker, id, task, function() {
                    dropErizoJS(id);
                });
                callback('callback', 'ok');
            } catch(error) {
                log.error('recycleNode error:', error);
            }
        },

        queryNode: function(task, callback) {
            for (var eid in tasks) {
                if (tasks[eid][task] !== undefined) {
                    return callback('callback', eid);
                }
            }
            return callback('callback', 'error');
        }
    };
};

var clusterIP, clusterInterface;
// It has three properties: name for interface's name, replaced_ip_address for replaced IP address, and ip_address for interface's IP address.
var webrtcInterfaces = global.config.webrtc.network_interfaces || [];
function collectIPs () {
    var interfaces = require('os').networkInterfaces(),
        clusterAddress,
        k,
        k2,
        address;

    for (k in interfaces) {
        if (interfaces.hasOwnProperty(k)) {
            for (k2 in interfaces[k]) {
                if (interfaces[k].hasOwnProperty(k2)) {
                    address = interfaces[k][k2];
                    if (address.family === 'IPv4' && !address.internal) {
                        var webrtcInterface = webrtcInterfaces.find((i) => {return i.name === k;});
                        if (webrtcInterface) {
                            webrtcInterface.ip_address = address.address;
                        }
                        if (!clusterInterface && (k === global.config.internal.network_interface || !global.config.internal.network_interface)) {
                            clusterInterface = k;
                            clusterAddress = address.address;
                        }
                    }
                }
            }
        }
    }

    if (global.config.internal.ip_address === '' || global.config.internal.ip_address === undefined) {
        clusterIP = clusterAddress;
    } else {
        clusterIP = global.config.internal.ip_address;
    }
}

var joinCluster = function (on_ok) {
    var joinOK = function (id) {
        myId = id;
        myState = 2;
        log.info(myPurpose, 'agent join cluster ok.');
        on_ok(id);
    };

    var joinFailed = function (reason) {
        log.error(myPurpose, 'agent join cluster failed. reason:', reason);
        process.exit();
    };

    var loss = function () {
        log.info(myPurpose, 'agent lost.');
        dropAll(false);
        fillErizos();
    };

    var recovery = function () {
        log.info(myPurpose, 'agent recovered.');
    };

    var overload = function () {
        log.warn(myPurpose, 'agent overloaded!');
        if (myPurpose === 'recording') {
            dropAll(false);
        }
    };

    worker = clusterWorker({
        rpcClient: rpcClient,
        purpose: myPurpose,
        clusterName: global.config.cluster.name,
        joinRetry: global.config.cluster.join_retry,
        info: {
            ip: clusterIP,
            purpose: myPurpose,
            state: 2,
            max_load: global.config.cluster.max_load,
            capacity: global.config.capacity
        },
        onJoinOK: joinOK,
        onJoinFailed: joinFailed,
        onLoss: loss,
        onRecovery: recovery,
        onOverload: overload,
        loadCollection: load_collection
    });
};

(function init_env() {
    reuse = (myPurpose === 'audio' || myPurpose === 'video') ? false : true;

    collectIPs();

    switch (myPurpose) {
        case 'webrtc':
            global.config.capacity.isps = global.config.capacity.isps || [];
            global.config.capacity.regions = global.config.capacity.regions || [];
        case 'avstream':
            var concernedInterface = clusterInterface;
            if (!concernedInterface) {
                var interfaces = require('os').networkInterfaces();
                for (var i in interfaces) {
                    for (var j in interfaces[i]) {
                        if (!interfaces[i][j].internal) {
                            concernedInterface = i;
                            break;
                        }
                    }
                    if (concernedInterface) {
                        break;
                    }
                }
            }

            load_collection.item = {name: 'network',
                                    interf: concernedInterface || 'lo',
                                    max_scale: global.config.cluster.network_max_scale};
            break;
        case 'recording':
            try {
                fs.accessSync(global.config.recording.path, fs.F_OK);
            } catch (e) {
                log.error('The configured recording path is not accessible.');
                process.exit(1);
            }

            load_collection.item = {name: 'disk',
                                    drive: global.config.recording.path};
            break;
        case 'sip':
            load_collection.item = {name: 'cpu'};
            break;
        case 'audio':
        case 'session':
            load_collection.item = {name: 'cpu'};
            break;
        case 'video':
            /*FIXME: should be double checked whether hardware acceleration is actually running*/
        load_collection.item = {name: (global.config.video.hardwareAccelerated ? 'gpu' : 'cpu')};
        if (global.config.video.hardwareAccelerated) {
            // Query the hardware capability only if we want to try it.
            if (global.config.video.yamiEnabled) {
                var path = require('path');
                process.env.LD_LIBRARY_PATH = [
                    path.resolve(process.cwd(), './lib/va'),
                    process.env.LD_LIBRARY_PATH,
                    path.resolve(process.cwd(), './lib'),
                    ].join(':');
                process.env.LIBVA_DRIVERS_PATH = path.resolve(process.cwd(), './lib/dri');
                process.env.LIBVA_DRIVER_NAME = 'i965';
            }
        }
        break;
        default:
            log.error('Ambiguous purpose:', purpose);
            process.exit();
            break;
    }
})();

amqper.connect(global.config.rabbit, function () {
    log.debug('Adding agent to cloudhandler, purpose:', myPurpose);
    amqper.asRpcClient(function(rpcClnt) {
        rpcClient = rpcClnt;
        joinCluster(function (rpcID) {
            amqper.asRpcServer(rpcID, api(worker), function(rpcServer) {
                log.info('as rpc server ok.');
                amqper.asMonitoringTarget(function (monitoringTgt) {
                    monitoringTarget = monitoringTgt;
                    log.info('as monitoring target ok.');
                }, function(reason) {
                    log.error(rpcID + 'as monitoring target failed, reason:', reason);
                    process.exit();
                });
            }, function(reason) {
                log.error(rpcID + ' as rpc server failed, reason:', reason);
                process.exit();
            });
            fillErizos();
        });
    }, function(reason) {
        log.error('Agent as rpc client failed, reason:', reason);
        process.exit();
    });
}, function(reason) {
    log.error('Agent connect to rabbitMQ server failed, reason:', reason);
    process.exit();
});

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, function () {
        log.warn('Exiting on', sig);
        process.exit();
    });
});

process.on('exit', function () {
    dropAll(true);
    worker && worker.quit();
    amqper.disconnect();
});
