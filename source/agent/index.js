/*global require, GLOBAL, process*/
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

// Configuration default values
GLOBAL.config = config || {};

GLOBAL.config.agent = GLOBAL.config.agent || {};
GLOBAL.config.agent.maxProcesses = GLOBAL.config.agent.maxProcesses || 13;
GLOBAL.config.agent.prerunProcesses = GLOBAL.config.agent.prerunProcesses || 2;

GLOBAL.config.cluster = GLOBAL.config.cluster || {};
GLOBAL.config.cluster.name = GLOBAL.config.cluster.name || 'woogeen-cluster';
GLOBAL.config.cluster.join_retry = GLOBAL.config.cluster.join_retry || 60;
GLOBAL.config.cluster.keep_alive_interval = GLOBAL.config.cluster.keep_alive_interval || 800;
GLOBAL.config.cluster.report_load_interval = GLOBAL.config.cluster.report_load_interval || 1000;
GLOBAL.config.cluster.max_load = GLOBAL.config.cluster.max_load || 0.85;
GLOBAL.config.cluster.network_max_scale = GLOBAL.config.cluster.network_max_scale || 1000;
GLOBAL.config.cluster.ip_address = GLOBAL.config.cluster.ip_address || '';
GLOBAL.config.cluster.network_interface = GLOBAL.config.cluster.network_interface || undefined;

GLOBAL.config.webrtc = GLOBAL.config.webrtc || {};
GLOBAL.config.webrtc.ip_address = GLOBAL.config.webrtc.ip_address || '';
GLOBAL.config.webrtc.network_interface = GLOBAL.config.webrtc.network_interface || undefined;

GLOBAL.config.recording = GLOBAL.config.recording || {};
GLOBAL.config.recording.path = GLOBAL.config.recording.path || '/tmp';

GLOBAL.config.video = GLOBAL.config.video || {};
GLOBAL.config.video.hardwareAccelerated = !!GLOBAL.config.video.hardwareAccelerated;
GLOBAL.config.video.yamiEnabled = !!GLOBAL.config.video.yamiEnabled;


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
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.host = value;
                break;
            case 'rabbit-port':
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.port = value;
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
                GLOBAL.config.agent[prop] = value;
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
var tasks = {}; // {erizo_id: {RoomID: [ConsumerID]}}
var load_collection = {period: GLOBAL.config.cluster.report_load_interval};

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
    var out = fs.openSync('../logs/' + id + '.log', 'a');
    var err = fs.openSync('../logs/' + id + '.log', 'a');
    var child = spawn('node', ['./erizoJS.js', id, myPurpose, privateIP, publicIP, clusterIP, myId], {
        detached: true,
        stdio: [ 'ignore', out, err, 'ipc' ]
    });
    child.unref();
    child.on('close', function (code) {
        log.info(id, 'exit with', code);
        //if (code !== 0) {
        if (processes[id]) {
            monitoringTarget && monitoringTarget.notify('abnormal', {purpose: myPurpose, id: id, type: 'node'});
        }
        cleanupErizoJS(id);
        fillErizos();
    });
    child.on('error', function (error) {
        log.error('failed to launch worker:', error);
        child.READY = false;
        monitoringTarget && monitoringTarget.notify('error', {purpose: myPurpose, id: id, type: 'node'});
        cleanupErizoJS(id);
        fillErizos(); // FIXME: distinguish between deterministic errors and recoverable ones.
    });
    child.on('message', function (message) { // currently only used for sending ready message from worker to agent;
        log.info('message from worker', id, ':', message);
        if (message === 'READY') {
            child.READY = true;
        } else {
            child.READY = false;
            child.kill();
            cleanupErizoJS(id);
            fillErizos(); // FIXME: distinguish between deterministic errors and recoverable ones.
        }
    });
    processes[id] = child;
    tasks[id] = {};
    idle_erizos.push(id);
};

var dropErizoJS = function(id, callback) {
    if (processes.hasOwnProperty(id)) {
        processes[id].kill();
        cleanupErizoJS(id);
        callback('callback', 'ok');
    }
};

var dropAll = function() {
    Object.keys(processes).map(function (k) {
        dropErizoJS(k, function (unused, status) {
            log.info('Terminate ErizoJS', k, status);
        });
    });
};

var fillErizos = function() {
    for (var i = idle_erizos.length; i < GLOBAL.config.agent.prerunProcesses; i++) {
        launchErizoJS();
    }
};

var isInUse = function(session) {
    for (var eid in tasks) {
        if (tasks[eid][session] !== undefined) {
            return true;
        }
    }
    return false;
};

var addConsumer = function(worker, nodeId, consumer) {
    tasks[nodeId] = tasks[nodeId] || {};
    if (tasks[nodeId][consumer.session] === undefined) {
        if (!isInUse(consumer.session)) {
            worker.addTask(consumer.session);
        }
        tasks[nodeId][consumer.session] = [];
    }
    tasks[nodeId][consumer.session].push(consumer.consumer);
};

var removeConsumer = function(worker, nodeId, consumer, on_last_consumer_leave) {
    if (tasks[nodeId]) {
        if (tasks[nodeId][consumer.session]) {
            var i = tasks[nodeId][consumer.session].indexOf(consumer.consumer);
            if (i > -1) {
                tasks[nodeId][consumer.session].splice(i, 1);
            }

            if (tasks[nodeId][consumer.session].length === 0) {
                delete tasks[nodeId][consumer.session];
                if (Object.keys(tasks[nodeId]).length === 0) {
                    on_last_consumer_leave();
                }
            }
        }
    }

    if (!isInUse) {
        worker.removeTask(consumer.session);
    }
};

var api = function (worker) {
    return {
        getNode: function(consumer, callback) {
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
                        addConsumer(worker, id, consumer);
                    }
                };
                waitForInitialization();
            };
            try {
                var room_id = consumer.session;
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

                if (reuse && myPurpose !== 'session' && myPurpose !== 'sip' && ((erizos.length + idle_erizos.length + 1) >= GLOBAL.config.agent.maxProcesses)) {
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

        recycleNode: function(id, consumer, callback) {
            try {
                removeConsumer(worker, id, consumer, function() {
                    dropErizoJS(id, callback);
                });
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

var privateIP, publicIP, clusterIP;
var externalInterface, clusterInterface;
function collectIPs () {
    var interfaces = require('os').networkInterfaces(),
        externalAddress,
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
                        if (!externalInterface && (k === GLOBAL.config.webrtc.network_interface || !GLOBAL.config.webrtc.network_interface)) {
                            externalInterface = k;
                            externalAddress = address.address;
                        }
                        if (!clusterInterface && (k === GLOBAL.config.cluster.network_interface || !GLOBAL.config.cluster.network_interface)) {
                            clusterInterface = k;
                            clusterAddress = address.address;
                        }
                    }
                }
            }
        }
    }

    privateIP = externalAddress;

    if (GLOBAL.config.webrtc.ip_address === '' || GLOBAL.config.webrtc.ip_address === undefined){
        publicIP = externalAddress;
    } else {
        publicIP = GLOBAL.config.webrtc.ip_address;
    }

    if (GLOBAL.config.cluster.ip_address === '' || GLOBAL.config.cluster.ip_address === undefined) {
        clusterIP = clusterAddress;
    } else {
        clusterIP = GLOBAL.config.cluster.ip_address;
    }
}

var joinCluster = function (on_ok) {
    var worker;
    var joinOK = function (id) {
        myId = id;
        myState = 2;
        log.info(myPurpose, 'agent join cluster ok.');
        on_ok(id);
        process.on('exit', function () {
            worker.quit();
        });
    };

    var joinFailed = function (reason) {
        log.error(myPurpose, 'agent join cluster failed. reason:', reason);
        process.exit();
    };

    var loss = function () {
        log.info(myPurpose, 'agent lost.');
        dropAll();
        fillErizos();
    };

    var recovery = function () {
        log.info(myPurpose, 'agent recovered.');
    };

    worker = clusterWorker({
        rpcClient: rpcClient,
        purpose: myPurpose,
        clusterName: GLOBAL.config.cluster.name,
        joinRetry: GLOBAL.config.cluster.join_retry,
        keepAlivePeriod: GLOBAL.config.cluster.keep_alive_interval,
        info: {
            ip: clusterIP,
            purpose: myPurpose,
            state: 2,
            max_load: GLOBAL.config.cluster.max_load
        },
        onJoinOK: joinOK,
        onJoinFailed: joinFailed,
        onLoss: loss,
        onRecovery: recovery,
        loadCollection: load_collection
    });
    return worker;
};

(function init_env() {
    reuse = (myPurpose === 'audio' || myPurpose === 'video') ? false : true;

    collectIPs();

    switch (myPurpose) {
        case 'webrtc':
        case 'avstream':
            var concernedInterface = externalInterface || clusterInterface;
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
                                    max_scale: GLOBAL.config.cluster.network_max_scale};
            break;
        case 'recording':
            try {
                fs.accessSync(GLOBAL.config.recording.path, fs.F_OK);
            } catch (e) {
                log.error('The configured recording path is not accessible.');
                process.exit(1);
            }

            load_collection.item = {name: 'disk',
                                    drive: GLOBAL.config.recording.path};
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
        load_collection.item = {name: (GLOBAL.config.video.hardwareAccelerated ? 'gpu' : 'cpu')};
        if (GLOBAL.config.video.hardwareAccelerated) {
            // Query the hardware capability only if we want to try it.
            if (GLOBAL.config.video.yamiEnabled) {
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
            load_collection.item = {name: 'cpu'};
            break;
    }
})();

amqper.connect(GLOBAL.config.rabbit, function () {
    log.info('Adding agent to cloudhandler, purpose:', myPurpose);
    amqper.asRpcClient(function(rpcClnt) {
        rpcClient = rpcClnt;
        var worker = joinCluster(function (rpcID) {
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
    dropAll();
    amqper.disconnect();
});
