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
  config = toml.parse(fs.readFileSync('./portal.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

// Configuration default values
config.portal = config.portal || {};
config.portal.ip_address = config.portal.ip_address || '';
config.portal.hostname = config.portal.hostname|| '';
config.portal.port = config.portal.port || 8080;
config.portal.via_host = config.portal.via_host || '';
config.portal.ssl = config.portal.ssl || false;
config.portal.force_tls_v12 = config.portal.force_tls_v12 || false;
config.portal.reconnection_ticket_lifetime = config.portal.reconnection_ticket_lifetime || 600;
config.portal.reconnection_timeout = Number.isInteger(config.portal.reconnection_timeout) ? config.portal.reconnection_timeout : 60;

config.cluster = config.cluster || {};
config.cluster.name = config.cluster.name || 'owt-cluster';
config.cluster.join_retry = config.cluster.join_retry || 60;
config.cluster.report_load_interval = config.cluster.report_load_interval || 1000;
config.cluster.max_load = config.cluster.max_laod || 0.85;
config.cluster.network_max_scale = config.cluster.network_max_scale || 1000;

config.capacity = config.capacity || {};
config.capacity.isps = config.capacity.isps || [];
config.capacity.regions = config.capacity.regions || [];


config.rabbit = config.rabbit || {};
config.rabbit.host = config.rabbit.host || 'localhost';
config.rabbit.port = config.rabbit.port || 5672;

// Parse portal hostname and ip_address variables from ENV.
if (config.portal.ip_address.indexOf('$') == 0) {
    config.portal.ip_address = process.env[config.portal.ip_address.substr(1)];
    log.info('ENV: config.portal.ip_address=' + config.portal.ip_address);
}
if (config.portal.hostname.indexOf('$') == 0) {
    config.portal.hostname = process.env[config.portal.hostname.substr(1)];
    log.info('ENV: config.portal.hostname=' + config.portal.hostname);
}
if(process.env.owt_via_host !== undefined) {
    config.portal.via_host = process.env.owt_via_host;
    log.info('ENV: config.portal.via_address=' + config.portal.via_host);
}

global.config = config;


var amqper = require('./amqp_client')();
var rpcClient;
var socketio_server;
var portal;
var worker;

var ip_address;
(function getPublicIP() {
  var BINDED_INTERFACE_NAME = config.portal.networkInterface;
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

  if (config.portal.ip_address === '' || config.portal.ip_address === undefined){
    ip_address = addresses[0];
  } else {
    ip_address = config.portal.ip_address;
  }
})();

var dropAll = function() {
  socketio_server && socketio_server.drop('all');
};

var getTokenKey = function(id, on_key, on_error) {
  var dataAccess = require('./data_access');
  dataAccess.token.key(id).then(function (key) {
    on_key(key);
  }).catch(function (err) {
    log.info('Failed to get token key. err:', (err && err.message) ? err.message : err);
    on_error(err);
  });
};

var joinCluster = function (on_ok) {
  var joinOK = on_ok;

  var joinFailed = function (reason) {
    log.error('portal join cluster failed. reason:', reason);
    worker && worker.quit();
    process.exit();
  };

  var loss = function () {
    log.info('portal lost.');
    dropAll();
  };

  var recovery = function () {
    log.info('portal recovered.');
  };

  var spec = {rpcClient: rpcClient,
              purpose: 'portal',
              clusterName: config.cluster.name,
              joinRetry: config.cluster.join_retry,
              info: {ip: ip_address,
                     hostname: config.portal.hostname,
                     port: config.portal.port,
                     via_host: config.portal.via_host,
                     ssl: config.portal.ssl,
                     state: 2,
                     max_load: config.cluster.max_load,
                     capacity: config.capacity
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

var refreshTokenKey = function(id, portal, tokenKey) {
  var interval = setInterval(function() {
    getTokenKey(id, function(newTokenKey) {
      (socketio_server === undefined) && clearInterval(interval);
      if (newTokenKey !== tokenKey) {
        log.info('Token key updated!');
        portal.updateTokenKey(newTokenKey);
        tokenKey = newTokenKey;
      }
    }, function() {
      (socketio_server === undefined) && clearInterval(interval);
      log.warn('Keep trying...');
    });
  }, 6 * 1000);
};

var serviceObserver = {
  onJoin: function(tokenCode) {
    worker && worker.addTask(tokenCode);
  },
  onLeave: function(tokenCode) {
    worker && worker.removeTask(tokenCode);
  }
};

var startServers = function(id, tokenKey) {
  var rpcChannel = require('./rpcChannel')(rpcClient);
  var rpcReq = require('./rpcRequest')(rpcChannel);

  portal = require('./portal')({tokenKey: tokenKey,
                                tokenServer: 'ManagementApi',
                                clusterName: config.cluster.name,
                                selfRpcId: id},
                                rpcReq);
  socketio_server = require('./socketIOServer')({port: config.portal.port,
                                                 ssl: config.portal.ssl,
                                                 forceTlsv12: config.portal.force_tls_v12,
                                                 keystorePath: config.portal.keystorePath,
                                                 reconnectionTicketLifetime: config.portal.reconnection_ticket_lifetime,
                                                 reconnectionTimeout: config.portal.reconnection_timeout,
                                                 pingInterval: config.portal.ping_interval,
                                                 pingTimeout: config.portal.ping_timeout},
                                                 portal,
                                                 serviceObserver);
  return socketio_server.start()
    .then(function() {
      log.info('start socket.io server ok.');
      refreshTokenKey(id, portal, tokenKey);
    })
    .catch(function(err) {
      log.error('Failed to start servers, reason:', err && err.message);
      throw err;
    });
};

var stopServers = function() {
  socketio_server && socketio_server.stop();
  socketio_server = undefined;
  worker && worker.quit();
  worker = undefined;
};

var rpcPublic = {
  drop: function(participantId, callback) {
    socketio_server && socketio_server.drop(participantId);
    callback('callback', 'ok');
  },
  notify: function(participantId, event, data, callback) {
    // The "notify" is called on socket.io server,
    // but one client ID should not be exists in both servers,
    // there must be one failure, ignore this notify error here.
    var notifyFail = (err) => {};
    socketio_server && socketio_server.notify(participantId, event, data).catch(notifyFail);
    callback('callback', 'ok');
  },
  validateAndDeleteWebTransportToken: (token, callback) => {
    if(portal.validateAndDeleteWebTransportToken(token)) {
      callback('callback','ok');
    } else {
      callback('callback', 'error', 'Invalid token for WebTransport.');
    }
  }
};

amqper.connect(config.rabbit, function () {
  amqper.asRpcClient(function(rpcClnt) {
    rpcClient = rpcClnt;
    log.info('portal initializing as rpc client ok');
    joinCluster(function(id) {
      log.info('portal join cluster ok, with rpcID:', id);
        amqper.asRpcServer(id, rpcPublic, function(rpcSvr) {
          log.info('portal initializing as rpc server ok');
            amqper.asMonitor(function (data) {
              if (data.reason === 'abnormal' || data.reason === 'error' || data.reason === 'quit') {
                if (portal !== undefined) {
                  if (data.message.purpose === 'conference') {
                    return portal.getParticipantsByController(data.message.type, data.message.id)
                      .then(function (impactedParticipants) {
                        impactedParticipants.forEach(function(participantId) {
                          log.error('Fault on conference controller(type:', data.message.type, 'id:', data.message.id, ') of participant', participantId, 'was detected, drop it.');
                          socketio_server && socketio_server.drop(participantId);
                        });
                      });
                  }
                }
              }
            }, function (monitor) {
              log.info(id + ' as monitor ready');
              getTokenKey(id, function(tokenKey) {
                startServers(id, tokenKey);
              }, function() {
                log.error('portal getting token failed.');
                stopServers();
                process.exit();
              });
            }, function(reason) {
              log.error('portal initializing as rpc client failed, reason:', reason);
              stopServers();
              process.exit();
            });
      }, function(reason) {
        log.error('portal initializing as rpc client failed, reason:', reason);
        stopServers();
        process.exit();
      });
    });
  }, function(reason) {
    log.error('portal initializing as rpc client failed, reason:', reason);
    stopServers();
    process.exit();
  });
}, function(reason) {
    log.error('portal connect to rabbitMQ server failed, reason:', reason);
    process.exit();
});

['SIGINT', 'SIGTERM'].map(function (sig) {
  process.on(sig, function () {
    log.warn('Exiting on', sig);
    stopServers();
    process.exit();
  });
});

process.on('SIGPIPE', function () {
  log.warn('SIGPIPE!!');
});

process.on('exit', function () {
    amqper.disconnect();
});

process.on('unhandledRejection', (reason) => {
    log.info('Reason: ' + reason);
});

process.on('SIGUSR2', function () {
  logger.reconfigure();
});
