// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var Getopt = require('node-getopt');
var spawn = require('child_process').spawn;
var fs = require('fs');
var toml = require('toml');
var logger = require('./logger').logger;
var log = logger.getLogger('WorkingAgent');

var config;
try {
  config = toml.parse(fs.readFileSync('./agent.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

// Configuration default values
global.config = config || {};

global.config.agent = global.config.agent || {};
global.config.agent.maxProcesses = global.config.agent.maxProcesses || 13;
global.config.agent.prerunProcesses = global.config.agent.prerunProcesses || 2;

global.config.cluster = global.config.cluster || {};
global.config.cluster.name = global.config.cluster.name || 'owt-cluster';
global.config.cluster.join_retry = global.config.cluster.join_retry || 60;
global.config.cluster.report_load_interval = global.config.cluster.report_load_interval || 1000;
global.config.cluster.max_load = global.config.cluster.max_load || 0.85;
global.config.cluster.network_max_scale = global.config.cluster.network_max_scale || 1000;

global.config.internal.ip_address = global.config.internal.ip_address || '';
global.config.internal.network_interface = global.config.internal.network_interface || undefined;

global.config.capacity = global.config.capacity || {};

global.config.webrtc = global.config.webrtc || {};
global.config.webrtc.ip_address = global.config.webrtc.ip_address || '';
global.config.webrtc.network_interfaces = global.config.webrtc.network_interfaces || undefined;

global.config.recording = global.config.recording || {};
global.config.recording.path = global.config.recording.path || '/tmp';

global.config.video = global.config.video || {};
global.config.video.hardwareAccelerated = !!global.config.video.hardwareAccelerated;
global.config.video.enableBetterHEVCQuality = !!global.config.video.enableBetterHEVCQuality;

// Parse command line arguments
var getopt = new Getopt([
  ['r' , 'rabbit-host=ARG'            , 'RabbitMQ Host'],
  ['g' , 'rabbit-port=ARG'            , 'RabbitMQ Port'],
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
                if (value === 'conference' ||
                    value === 'webrtc' ||
                    value === 'sip' ||
                    value === 'streaming' ||
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
var nodeManager = require('./nodeManager');
var amqper = require('./amqp_client')();
var rpcClient;
var monitoringTarget;

var load_collection = {period: global.config.cluster.report_load_interval};
var capacity = global.config.capacity;
var worker;
var manager;

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
        manager && manager.dropAllNodes(false);
    };

    var recovery = function () {
        log.info(myPurpose, 'agent recovered.');
    };

    var overload = function () {
        log.warn(myPurpose, 'agent overloaded!');
        if (myPurpose === 'recording') {
            manager && manager.dropAllNodes(false);
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

var init_manager = () => {
  var reuseNode = !(myPurpose === 'audio' || myPurpose === 'video' || myPurpose === 'conference' || myPurpose === 'sip');
  var consumeNodeByRoom = !(myPurpose === 'audio' || myPurpose === 'video');

  var spawnOptions = {
    cmd: 'node',
    config: Object.assign({}, global.config)
  };

  spawnOptions.config.purpose = myPurpose;
  spawnOptions.config.clusterIP = clusterIP;

  manager = nodeManager(
    {
      parentId: myId,
      prerunNodeNum: global.config.agent.prerunProcesses,
      maxNodeNum: global.config.agent.maxProcesses,
      reuseNode: reuseNode,
      consumeNodeByRoom: consumeNodeByRoom
    },
    spawnOptions,
    (nodeId, tasks) => {
      monitoringTarget && monitoringTarget.notify('abnormal', {purpose: myPurpose, id: nodeId, type: 'node'});
      tasks.forEach(() => {
        worker && worker.removeTask(task);
      });
    },
    (task) => {
      worker && worker.addTask(task);
    },
    (task) => {
      worker && worker.removeTask(task);
    }
  );
};

var rpcAPI = function (worker) {
    return {
        getNode: function(task, callback) {
          if (manager) {
            return manager.getNode(task).then((nodeId) => {
              callback('callback', nodeId);
            }).catch((err) => {
              callback('callback', 'error', err);
            });
          } else {
            callback('callback', 'error', 'agent not ready');
          }
        },

        recycleNode: function(id, task, callback) {
          if (manager) {
            return manager.recycleNode(id, task).then(() => {
              callback('callback', 'ok');
            }).catch((err) => {
              callback('callback', 'error', err);
            });
          } else {
            callback('callback', 'error', 'agent not ready');
          }
        },

        queryNode: function(task, callback) {
          if (manager) {
            return manager.queryNode(task).then((nodeId) => {
              callback('callback', nodeId);
            }).catch((err) => {
              callback('callback', 'error', err);
            });
          } else {
            callback('callback', 'error', 'agent not ready');
          }
        }
    };
};

(function init_env() {
    reuse = (myPurpose === 'audio' || myPurpose === 'video') ? false : true;

    collectIPs();

    switch (myPurpose) {
        case 'webrtc':
            global.config.capacity.isps = global.config.capacity.isps || [];
            global.config.capacity.regions = global.config.capacity.regions || [];
        case 'streaming':
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
        case 'conference':
            load_collection.item = {name: 'cpu'};
            break;
        case 'video':
            /*FIXME: should be double checked whether hardware acceleration is actually running*/
            var videoCapability = require('./videoCapability');
            log.debug('Video Capability:', JSON.stringify(videoCapability));
            capacity.video = videoCapability;
            global.config.video.codecs = videoCapability;
            load_collection.item = {name: (global.config.video.hardwareAccelerated ? 'gpu' : 'cpu')};
            break;
        default:
            log.error('Ambiguous purpose:', purpose);
            process.exit();
            break;
    }
})();

amqper.connect(global.config.rabbit, function () {
    log.debug('Initializing RPC facilities, purpose:', myPurpose);
    amqper.asRpcClient(function(rpcClnt) {
        rpcClient = rpcClnt;
        joinCluster(function (rpcID) {
            amqper.asRpcServer(rpcID, rpcAPI(worker), function(rpcServer) {
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
            init_manager();
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
    manager && manager.dropAllNodes(true);
    worker && worker.quit();
    amqper.disconnect();
});

process.on('unhandledRejection', (reason) => {
    log.info('Reason: ' + reason);
});

process.on('SIGUSR2', function() {
    logger.reconfigure();
});
