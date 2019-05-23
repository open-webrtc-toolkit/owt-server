// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var Getopt = require('node-getopt');
var config = require('./configLoader').load();

var logger = require('./logger').logger;
var log = logger.getLogger('WorkingAgent');

// Parse command line arguments
var getopt = new Getopt([
  ['U' , 'my-purpose=ARG'             , 'Purpose of this agent'],
  ['h' , 'help'                       , 'display this help']
]);

var myId = '';
var myPurpose = 'webrtc';
var myState = 2;

var opt = getopt.parse(process.argv.slice(2));

for (var prop in opt.options) {
    if (opt.options.hasOwnProperty(prop)) {
        var value = opt.options[prop];
        switch (prop) {
            case 'help':
                getopt.showHelp();
                process.exit(0);
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
                break;
        }
    }
}

var clusterWorker = require('./clusterWorker');
var nodeManager = require('./nodeManager');
var amqper = require('./amqp_client')();
var rpcClient;
var monitoringTarget;

var worker;
var manager;

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
        clusterName: config.cluster.name,
        joinRetry: config.cluster.worker.join_retry,
        info: {
            ip: config.cluster.worker.ip,
            purpose: myPurpose,
            state: 2,
            max_load: config.cluster.worker.load.max,
            capacity: config.capacity
        },
        onJoinOK: joinOK,
        onJoinFailed: joinFailed,
        onLoss: loss,
        onRecovery: recovery,
        onOverload: overload,
        loadCollection: config.cluster.worker.load
    });
};

var init_manager = () => {
  var reuseNode = !(myPurpose === 'audio' || myPurpose === 'video' || myPurpose === 'conference' || myPurpose === 'sip');
  var consumeNodeByRoom = !(myPurpose === 'audio' || myPurpose === 'video');

  var spawnOptions = {
    cmd: 'node',
    config: Object.assign({}, config)
  };

  // WebRTC-QUIC module links BoringSSL. We start it by a node without OpenSSL.
  if (myPurpose === 'webrtc') {
    spawnOptions.cmd = '/mnt/downloads/quic-ok/webrtc_agent/snode';
  }

  spawnOptions.config.purpose = myPurpose;

  manager = nodeManager(
    {
      parentId: myId,
      prerunNodeNum: config.agent.prerunProcesses,
      maxNodeNum: config.agent.maxProcesses,
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

amqper.connect(config.rabbit, function () {
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
