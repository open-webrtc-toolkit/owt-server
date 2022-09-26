// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var fs = require('fs');
var toml = require('toml');
var logger = require('./logger').logger;
var log = logger.getLogger('Main');

var config;
try {
  config = toml.parse(fs.readFileSync('./agent.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

// Configuration default values
config.bridge = config.bridge || {};
config.bridge.ip_address = config.bridge.ip_address || '';
config.bridge.hostname = config.bridge.hostname|| '';
config.bridge.port = config.bridge.port || 7700;

config.cluster = config.cluster || {};
config.cluster.name = config.cluster.name || 'owt-cluster';
config.cluster.join_retry = config.cluster.join_retry || 60;
config.cluster.report_load_interval = config.cluster.report_load_interval || 1000;
config.cluster.max_load = config.cluster.max_load || 0.85;
config.cluster.network_max_scale = config.cluster.network_max_scale || 1000;


config.rabbit = config.rabbit || {};
config.rabbit.host = config.rabbit.host || 'localhost';
config.rabbit.port = config.rabbit.port || 5672;

// Parse bridge hostname and ip_address variables from ENV.
if (config.bridge.ip_address.indexOf('$') == 0) {
    config.bridge.ip_address = process.env[config.bridge.ip_address.substr(1)];
    log.info('ENV: config.bridge.ip_address=' + config.bridge.ip_address);
}
if (config.bridge.hostname.indexOf('$') == 0) {
    config.bridge.hostname = process.env[config.bridge.hostname.substr(1)];
    log.info('ENV: config.bridge.hostname=' + config.bridge.hostname);
}

global.config = config;


var amqper = require('./amqpClient')();
var rpcClient;
var bridge;
var event_cascading;
var worker;

var ip_address;
(function getPublicIP() {
  var BINDED_INTERFACE_NAME = config.bridge.networkInterface;
  var interfaces = require('os').networkInterfaces(),
    addresses = [],
    k,
    k2,
    address;

  for (k in interfaces) {
    if (interfaces.hasOwnProperty(k)) {
      for (k2 in interfaces[k]) {
        if (interfaces[k].hasOwnProperty(k2)) {
          address = interfaces[k][k2];
          if (address.family === 'IPv4' && !address.internal) {
            if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
              addresses.push(address.address);
            }
          }
          if (address.family === 'IPv6' && !address.internal) {
            if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
              addresses.push('[' + address.address + ']');
            }
          }
        }
      }
    }
  }

  if (config.bridge.ip_address === '' || config.bridge.ip_address === undefined){
    ip_address = addresses[0];
  } else {
    ip_address = config.bridge.ip_address;
  }
})();

var joinCluster = function (on_ok) {
  var joinOK = on_ok;

  var joinFailed = function (reason) {
    log.error('event bridge join cluster failed. reason:', reason);
    worker && worker.quit();
    process.exit();
  };

  var loss = function () {
    log.info('event bridge lost.');
  };

  var recovery = function () {
    log.info('event bridge recovered.');
  };

  var spec = {rpcClient: rpcClient,
              purpose: 'eventbridge',
              clusterName: config.cluster.name,
              joinRetry: config.cluster.join_retry,
              info: {ip: ip_address,
                     port: config.bridge.port,
                     state: 2,
                     max_load: config.cluster.max_load,
                     capacity: {}
                    },
              onJoinOK: joinOK,
              onJoinFailed: joinFailed,
              onLoss: loss,
              onRecovery: recovery,
              loadCollection: {period: config.cluster.report_load_interval,
                               item: {name: 'cpu'}}
             };

  worker = require('./clusterWorker')(spec);
};

var startServers = function(id) {
  var rpcChannel = require('./rpcChannel')(rpcClient);
  var rpcReq = require('./rpcRequest')(rpcChannel);

  event_cascading = require('./eventCascading')({port: config.bridge.port,
                                                   ssl: config.bridge.ssl,
                                                   clusterName: config.cluster.name,
                                                   selfRpcId: id},
                                                   rpcReq);
    event_cascading.start();
    /*return event_cascading.start()
      .then(function() {
        log.info('start event bridge server ok.');
      })
      .catch(function(err) {
        log.error('Failed to start servers, reason:', err && err.message);
        throw err;
      });*/
};

var stopServers = function() {
  log.info('stop event bridge server ok.');
  if (event_cascading) {
    return event_cascading.stop()
            .then (() => {
              event_cascading = undefined;
              return Promise.resolve('ok');
            }) ;
  }

  return Promise.resolve('ok');
};

var rpcPublic = {
  drop: function(participantId, callback) {
    event_cascading && event_cascading.drop(participantId);
    callback('callback', 'ok');
  },
  notify: function(participantId, event, data, callback) {
    // The "notify" is called on socket.io server,
    // but one client ID should not be exists in both servers,
    // there must be one failure, ignore this notify error here.
    var notifyFail = (err) => {};
    event_cascading && event_cascading.notify(participantId, event, data).catch(notifyFail);
    callback('callback', 'ok');
  },
  broadcast: function(controller, excludeList, event, data, callback) {
    event_cascading && event_cascading.broadcast(controller, excludeList, event, data);
    callback('callback', 'ok');
  },
  getInfo: function() {
    callback('callback', {ip:ip_address, port:config.bridge.port})
  },
  startCascading: function(data, callback) {
    event_cascading && event_cascading.startCascading(data, function () {
      callback('callback', 'ok');
    }, function(error) {
      callback('callback', 'error', error);
    });
  },
  destroyRoom: function(participantId, event, data, callback) {
    event_cascading && event_cascading.destroyRoom(data);
    callback('callback', 'ok');
  }
};

amqper.connect(config.rabbit, function () {
  amqper.asRpcClient(function(rpcClnt) {
    rpcClient = rpcClnt;
    log.info('bridge initializing as rpc client ok and client is:', rpcClnt);
    joinCluster(function(id) {
      log.info('bridge join cluster ok, with rpcID:', id);
        amqper.asRpcServer(id, rpcPublic, function(rpcSvr) {
          log.info('bridge initializing as rpc server ok');
            amqper.asMonitor(function (data) {
              if (data.reason === 'abnormal' || data.reason === 'error' || data.reason === 'quit') {
                if (bridge !== undefined) {
                  if (data.message.purpose === 'conference') {
                    return bridge.getParticipantsByController(data.message.type, data.message.id)
                      .then(function (impactedParticipants) {
                        impactedParticipants.forEach(function(participantId) {
                          log.error('Fault on conference controller(type:', data.message.type, 'id:', data.message.id, ') of participant', participantId, 'was detected, drop it.');
                          //event_cascading && socketio_server.drop(participantId);
                        });
                      });
                  }
                }
              }
            }, function (monitor) {
              log.info('eventbridge as monitor ready');
              startServers(id);
            }, function(reason) {
              log.error('bridge initializing as monitor failed, reason:', reason);
              stopServers();
              process.exit();
            });
      }, function(reason) {
        log.error('bridge initializing as rpc server failed, reason:', reason);
        stopServers();
        process.exit();
      });
    });
  }, function(reason) {
    log.error('bridge initializing as rpc client failed, reason:', reason);
    stopServers();
    process.exit();
  });
}, function(reason) {
    log.error('bridge connect to rabbitMQ server failed, reason:', reason);
    process.exit();
});

['SIGINT', 'SIGTERM'].map(function (sig) {
  process.on(sig, async function () {
    log.warn('Exiting on', sig);
    await stopServers();
    setTimeout(() => {
      amqper.disconnect();
      process.exit();
    }, 10);
  });
});

process.on('SIGPIPE', function () {
  log.warn('SIGPIPE!!');
});

process.on('exit', function () {
  log.info('Process exit');
});

process.on('unhandledRejection', (reason) => {
    log.info('Reason: ' + reason);
});

process.on('SIGUSR2', function () {
  logger.reconfigure();
});
