// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

const fs = require('fs');
const toml = require('toml');
const {logger} = require('./logger');

const log = logger.getLogger('Main');

let config;
try {
  config = toml.parse(fs.readFileSync('./portal.toml'));
} catch (e) {
  log.error(`Parsing config error on line ${e.line},
  column ${e.column}: ${e.message}`);
  process.exit(1);
}

// Configuration default values
config.portal = config.portal || {};
config.portal.ip_address = config.portal.ip_address || '';
config.portal.hostname = config.portal.hostname || '';
config.portal.port = config.portal.port || 8080;
config.portal.via_host = config.portal.via_host || '';
config.portal.ssl = config.portal.ssl || false;
config.portal.force_tls_v12 = config.portal.force_tls_v12 || false;
config.portal.reconnection_ticket_lifetime =
    config.portal.reconnection_ticket_lifetime || 600;
config.portal.reconnection_timeout =
    Number.isInteger(config.portal.reconnection_timeout) ?
    config.portal.reconnection_timeout : 60;
config.portal.cors = config.portal.cors || [];

config.cluster = config.cluster || {};
config.cluster.name = config.cluster.name || 'owt-cluster';
config.cluster.join_retry = config.cluster.join_retry || 60;
config.cluster.report_load_interval =
    config.cluster.report_load_interval || 1000;
config.cluster.max_load = config.cluster.max_load || 0.85;
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
  log.info(`ENV: config.portal.ip_address=${config.portal.ip_address}`);
}
if (config.portal.hostname.indexOf('$') == 0) {
  config.portal.hostname = process.env[config.portal.hostname.substr(1)];
  log.info(`ENV: config.portal.hostname=${config.portal.hostname}`);
}
if (process.env.owt_via_host !== undefined) {
  config.portal.via_host = process.env.owt_via_host;
  log.info(`ENV: config.portal.via_address=${config.portal.via_host}`);
}

global.config = config;

const amqper = require('./amqpClient')();

let rpcClient;
let socketio_server;
let portal;
let worker;

let ip_address;
(function getPublicIP() {
  const BINDED_INTERFACE_NAME = config.portal.networkInterface;
  const interfaces = require('os').networkInterfaces();
  const addresses = [];
  let k;
  let k2;
  let address;

  for (k in interfaces) {
    if (Object.prototype.hasOwnProperty.call(interfaces, k)) {
      for (k2 in interfaces[k]) {
        if (Object.prototype.hasOwnProperty.call(interfaces[k], k2)) {
          address = interfaces[k][k2];
          if (address.family === 'IPv4' && !address.internal) {
            if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
              addresses.push(address.address);
            }
          }
          if (address.family === 'IPv6' && !address.internal) {
            if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
              addresses.push(`[${address.address}]`);
            }
          }
        }
      }
    }
  }

  if (
    config.portal.ip_address === ''||
    config.portal.ip_address === undefined
  ) {
    ip_address = addresses[0];
  } else {
    ip_address = config.portal.ip_address;
  }
}());

const dropAll = function() {
  socketio_server && socketio_server.drop('all');
};

const getTokenKey = function(id, on_key, on_error) {
  const dataAccess = require('./data_access');
  dataAccess.token.key(id).then((key) => {
    on_key(key);
  }).catch((err) => {
    log.info('Failed to get token key. err:',
        (err && err.message) ? err.message : err);
    on_error(err);
  });
};

const joinCluster = function(on_ok) {
  const joinOK = on_ok;

  const joinFailed = function(reason) {
    log.error('portal join cluster failed. reason:', reason);
    worker && worker.quit();
    process.exit();
  };

  const loss = function() {
    log.info('portal lost.');
    dropAll();
  };

  const recovery = function() {
    log.info('portal recovered.');
  };

  const spec = {
    rpcClient,
    purpose: 'portal',
    clusterName: config.cluster.name,
    joinRetry: config.cluster.join_retry,
    info: {
      ip: ip_address,
      hostname: config.portal.hostname,
      port: config.portal.port,
      via_host: config.portal.via_host,
      ssl: config.portal.ssl,
      state: 2,
      max_load: config.cluster.max_load,
      capacity: config.capacity,
    },
    onJoinOK: joinOK,
    onJoinFailed: joinFailed,
    onLoss: loss,
    onRecovery: recovery,
    loadCollection: {
      period: config.cluster.report_load_interval,
      item: {name: 'cpu'},
    },
  };

  worker = require('./clusterWorker')(spec);
};

const refreshTokenKey = function(id, portal, tokenKey) {
  const interval = setInterval(() => {
    getTokenKey(id, (newTokenKey) => {
      (socketio_server === undefined) && clearInterval(interval);
      if (newTokenKey !== tokenKey) {
        log.info('Token key updated!');
        portal.updateTokenKey(newTokenKey);
        tokenKey = newTokenKey;
      }
    }, () => {
      (socketio_server === undefined) && clearInterval(interval);
      log.warn('Keep trying...');
    });
  }, 6 * 1000);
};

const serviceObserver = {
  onJoin(tokenCode) {
    worker && worker.addTask(tokenCode);
  },
  onLeave(tokenCode) {
    worker && worker.removeTask(tokenCode);
  },
};

const startServers = function(id, tokenKey) {
  const rpcChannel = require('./rpcChannel')(rpcClient);
  const rpcReq = require('./rpcRequest')(rpcChannel);

  portal = require('./portal')({
    tokenKey,
    tokenServer: 'ManagementApi',
    clusterName: config.cluster.name,
    selfRpcId: id,
  },
  rpcReq);
  socketio_server = require('./socketIOServer')({
    port: config.portal.port,
    cors: config.portal.cors,
    ssl: config.portal.ssl,
    forceTlsv12: config.portal.force_tls_v12,
    keystorePath: config.portal.keystorePath,
    reconnectionTicketLifetime: config.portal.reconnection_ticket_lifetime,
    reconnectionTimeout: config.portal.reconnection_timeout,
    pingInterval: config.portal.ping_interval,
    pingTimeout: config.portal.ping_timeout,
  },
  portal,
  serviceObserver);
  return socketio_server.start()
      .then(() => {
        log.info('start socket.io server ok.');
        refreshTokenKey(id, portal, tokenKey);
      })
      .catch((err) => {
        log.error('Failed to start servers, reason:', err && err.message);
        throw err;
      });
};

const stopServers = function() {
  socketio_server && socketio_server.stop();
  socketio_server = undefined;
  worker && worker.quit();
  worker = undefined;
};

const rpcPublic = {
  drop(participantId, callback) {
    socketio_server && socketio_server.drop(participantId);
    callback('callback', 'ok');
  },
  notify(participantId, event, data, callback) {
    // The "notify" is called on socket.io server,
    // but one client ID should not be exists in both servers,
    // there must be one failure, ignore this notify error here.
    const notifyFail = () => {};
    socketio_server &&
        socketio_server.notify(participantId, event, data).catch(notifyFail);
    callback('callback', 'ok');
  },
  validateAndDeleteWebTransportToken: (token, callback) => {
    if (portal.validateAndDeleteWebTransportToken(token)) {
      callback('callback', 'ok');
    } else {
      callback('callback', 'error', 'Invalid token for WebTransport.');
    }
  },
  broadcast(controller, excludeList, event, data, callback) {
    socketio_server &&
        socketio_server.broadcast(controller, excludeList, event, data);
    callback('callback', 'ok');
  },
};

amqper.connect(config.rabbit, () => {
  amqper.asRpcClient((rpcClnt) => {
    rpcClient = rpcClnt;
    log.info('portal initializing as rpc client ok');
    joinCluster((id) => {
      log.info('portal join cluster ok, with rpcID:', id);
      amqper.asRpcServer(id, rpcPublic, () => {
        log.info('portal initializing as rpc server ok');
        amqper.asMonitor((data) => {
          if (
            data.reason === 'abnormal' ||
            data.reason === 'error' ||
            data.reason === 'quit'
          ) {
            if (portal !== undefined) {
              if (data.message.purpose === 'conference') {
                return portal.getParticipantsByController(data.message.type,
                    data.message.id)
                    .then((impactedParticipants) => {
                      impactedParticipants.forEach((participantId) => {
                        log.error('Fault on conference controller(type:',
                            data.message.type,
                            'id:', data.message.id,
                            ') of participant', participantId,
                            'was detected, drop it.');
                        socketio_server && socketio_server.drop(participantId);
                      });
                    });
              }
            }
          }
        }, () => {
          log.info(`${id} as monitor ready`);
          getTokenKey(id, (tokenKey) => {
            startServers(id, tokenKey);
          }, () => {
            log.error('portal getting token failed.');
            stopServers();
            process.exit();
          });
        }, (reason) => {
          log.error('portal initializing as rpc client failed, reason:',
              reason);
          stopServers();
          process.exit();
        });
      }, (reason) => {
        log.error('portal initializing as rpc client failed, reason:', reason);
        stopServers();
        process.exit();
      });
    });
  }, (reason) => {
    log.error('portal initializing as rpc client failed, reason:', reason);
    stopServers();
    process.exit();
  });
}, (reason) => {
  log.error('portal connect to rabbitMQ server failed, reason:', reason);
  process.exit();
});

['SIGINT', 'SIGTERM'].map((sig) => {
  process.on(sig, async () => {
    log.warn('Exiting on', sig);
    stopServers();
    amqper.disconnect();
    process.exit();
  });
});

process.on('SIGPIPE', () => {
  log.warn('SIGPIPE!!');
});

process.on('exit', () => {
  log.info('Process exit');
});

process.on('unhandledRejection', (reason) => {
  log.info(`Reason: ${reason}`);
});

process.on('SIGUSR2', () => {
  logger.reconfigure();
});
