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
                    value === 'analytics' ||
                    value === 'audio' ||
                    value === 'video' ||
                    value === 'mediabridge' ||
                    value === 'eventbridge' ||
                    value === 'quic') {
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
var amqper = require('./amqpClient')();
var rpcClient;
var monitoringTarget;

var worker;
var manager;
var grpcPort;

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
        manager && manager.recover();
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
        // Cannot find a defination about |info|. It looks like it will be used by cluster manager, but agents and portal may have different properties of |info|.
        info: {
            ip: config.cluster.worker.ip,
            hostname: config[myPurpose] ? config[myPurpose].hostname : undefined,
            port: config[myPurpose] ? config[myPurpose].port : undefined,
            purpose: myPurpose,
            state: 2,
            maxLoad: config.cluster.worker.load.max,
            capacity: config.capacity,
            grpcPort: grpcPort
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
  var reuseNode = !(myPurpose === 'audio'
    || myPurpose === 'video'
    || myPurpose === 'analytics'
    || myPurpose === 'conference'
    || myPurpose === 'sip');
  var consumeNodeByRoom = !(myPurpose === 'audio' || myPurpose === 'video' || myPurpose === 'analytics');

  var spawnOptions = {
    cmd: 'node',
    config: Object.assign({}, config)
  };

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

// Adapt MQ callback to grpc callback
const cbAdapter = function(callback) {
  return function (n, code, data) {
    if (code === 'error') {
      callback(new Error(data), null);
    } else {
      callback(null, {message: code});
    }
  };
};

const grpcAPI = function (rpcAPI) {
  return {
    getNode: function (call, callback) {
      rpcAPI.getNode(call.request.info, cbAdapter(callback));
    },
    recycleNode: function(call, callback) {
      rpcAPI.recycleNode(call.request.id, call.request.info,
          cbAdapter(callback));
    },
    queryNode: function(call, callback) {
      rpcAPI.queryNode(call.request.info.task, cbAdapter(callback));
    }
  };
};

if (config.agent.enable_grpc) {
  const grpcTools = require('./grpcTools');
  grpcTools.startServer('nodeManager', grpcAPI(rpcAPI(worker)))
    .then((port) => {
        // Save GRPC port
        grpcPort = port;
        log.info('as rpc server ok', config.cluster.worker.ip + ':' + grpcPort);
        joinCluster(function (rpcId) {
          init_manager();
        });
    }).catch((err) => {
        log.error('Start grpc server failed:', err);
    });

} else {
  amqper.connect(config.rabbit, function () {
    log.debug('Initializing RPC facilities, purpose:', myPurpose);
    amqper.asRpcClient(function(rpcClnt) {
        rpcClient = rpcClnt;
        joinCluster(null, function (rpcID) {
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
}

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, async function () {
        log.warn('Exiting on', sig);
        manager && manager.dropAllNodes(true);
        worker && worker.quit();
        try {
            await amqper.disconnect();
        } catch(e) {
            log.warn('Exiting e:', e);
        }
        process.exit();
    });
});

process.on('exit', function () {
    log.info('Process exit');
});

process.on('unhandledRejection', (reason) => {
    log.info('Reason: ' + reason);
});

process.on('SIGUSR2', function() {
    logger.reconfigure();
});
