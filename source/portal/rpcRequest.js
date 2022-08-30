// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const grpcTools = require('./grpcTools');
const enableGrpc = config.portal.enable_grpc || false;
const log = require('./logger').logger.getLogger('RpcRequest');
const GRPC_TIMEOUT = 3000;

var RpcRequest = function(rpcChannel) {
  var that = {};
  let clusterClient;
  const grpcAgents = {}; // workerAgent => grpcClient
  const grpcNode = {}; // workerNode => grpcClient
  const opt = () => ({deadline: new Date(Date.now() + GRPC_TIMEOUT)});

  const startConferenceClientIfNeeded = function (node) {
    if (grpcNode[node]) {
      return grpcNode[node];
    }
    log.debug('Start conference client:', node);
    grpcNode[node] = grpcTools.startClient('conference', node);
    // Add notification listener
    const handler = that.notificationHandler;
    if (handler) {
      const call = grpcNode[node].listenToNotifications({id: 'portal'});
      call.on('data', (notification) => {
        log.debug('On notification data:', JSON.stringify(notification));
        if (notification.name === 'drop') {
          handler.drop(notification.id);
        } else {
          const data = JSON.parse(notification.data);
          if (notification.room) {
            handler.broadcast(
              notification.room, [notification.from],
              notification.name, data, () => {});
          } else {
            handler.notify(notification.id, notification.name, data, () => {});
          }
        }
      });
      call.on('end', () => {
        grpcNode[node].close();
        delete grpcNode[node];
      });
      call.on('error', (err) => {
        log.debug('Listen notifications error:', err);
      });
    }
    return grpcNode[node];
  };

  that.getController = function(clusterManager, roomId) {
    if (enableGrpc) {
      if (!clusterClient) {
        clusterClient = grpcTools.startClient(
          'clusterManager',
          clusterManager
        );
      }
      let agentAddress;
      return new Promise((resolve, reject) => {
        const req = {
          purpose: 'conference',
          task: roomId,
          preference: {}, // Change data for some preference
          reserveTime: 30 * 1000
        };
        clusterClient.schedule(req, opt(), (err, result) => {
          if (err) {
            log.debug('Schedule node error:', err);
            reject(err);
          } else {
            resolve(result);
          }
        });
      }).then((workerAgent) => {
        agentAddress = workerAgent.info.ip + ':' + workerAgent.info.grpcPort;
        if (!grpcAgents[agentAddress]) {
          grpcAgents[agentAddress] = grpcTools.startClient(
            'nodeManager',
            agentAddress
          );
        }
        return new Promise((resolve, reject) => {
          const req = {info: {room: roomId, task: roomId}};
          grpcAgents[agentAddress].getNode(req, opt(), (err, result) => {
            if (!err) {
              const node = result.message;
              startConferenceClientIfNeeded(node);
              resolve(node);
            } else {
              reject(err);
            }
          });
        });
      });
    }

    return rpcChannel.makeRPC(clusterManager, 'schedule', ['conference', roomId, 'preference'/*TODO: specify preference*/, 30 * 1000])
      .then(function(controllerAgent) {
        return rpcChannel.makeRPC(controllerAgent.id, 'getNode', [{room: roomId, task: roomId}]);
      });
  };

  that.join = function(controller, roomId, participant) {
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      const req = {roomId, participant};
      return new Promise((resolve, reject) => {
        grpcNode[controller].join(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'join', [roomId, participant], 6000);
  };

  that.leave = function(controller, participantId) {
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      const req = {id: participantId};
      return new Promise((resolve, reject) => {
        grpcNode[controller].leave(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'leave', [participantId]);
  };

  that.text = function(controller, fromWhom, toWhom, message) {
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      const req = {
        from: fromWhom,
        to: toWhom,
        message
      };
      return new Promise((resolve, reject) => {
        grpcNode[controller].text(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'text', [fromWhom, toWhom, message], 4000);
  };

  that.publish = function(controller, participantId, streamId, Options) {
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      const req = {
        participantId,
        streamId,
        pubInfo: Options
      };
      if (req.pubInfo.attributes) {
        req.pubInfo.attributes = JSON.stringify(req.pubInfo.attributes);
      }
      return new Promise((resolve, reject) => {
        grpcNode[controller].publish(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'publish', [participantId, streamId, Options]);
  };

  that.unpublish = function(controller, participantId, streamId) {
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      const req = {
        participantId,
        sessionId: streamId
      };
      return new Promise((resolve, reject) => {
        grpcNode[controller].unpublish(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'unpublish', [participantId, streamId]);
  };

  that.streamControl = function(controller, participantId, streamId, command) {
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      // To JSON command
      const req = {
        participantId,
        sessionId: streamId,
        command: JSON.stringify(command)
      };
      return new Promise((resolve, reject) => {
        grpcNode[controller].streamControl(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'streamControl', [participantId, streamId, command], 4000);
  };

  that.subscribe = function(controller, participantId, subscriptionId, Options) {
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      const req = {
        participantId,
        subscriptionId,
        subInfo: Options
      };
      return new Promise((resolve, reject) => {
        grpcNode[controller].subscribe(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'subscribe', [participantId, subscriptionId, Options]);
  };

  that.unsubscribe = function(controller, participantId, subscriptionId) {
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      const req = {
        participantId,
        sessionId: subscriptionId
      };
      return new Promise((resolve, reject) => {
        grpcNode[controller].unsubscribe(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'unsubscribe', [participantId, subscriptionId]);
  };

  that.subscriptionControl = function(controller, participantId, subscriptionId, command) {
    log.warn('subscriptionConrol, ', participantId, subscriptionId, command)
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      const req = {
        participantId,
        sessionId: subscriptionId,
        command: JSON.stringify(command)
      };
      return new Promise((resolve, reject) => {
        grpcNode[controller].subscriptionControl(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'subscriptionControl', [participantId, subscriptionId, command]);
  };

  that.onSessionSignaling = function(controller, sessionId, signaling) {
    if (enableGrpc) {
      startConferenceClientIfNeeded(controller);
      const req = {
        id: sessionId,
        signaling
      };
      return new Promise((resolve, reject) => {
        grpcNode[controller].onSessionSignaling(req, opt(), (err, result) => {
          if (err) {
            reject(err.message);
          } else {
            resolve(result);
          }
        });
      });
    }
    return rpcChannel.makeRPC(controller, 'onSessionSignaling', [sessionId, signaling]);
  };

  return that;
};

module.exports = RpcRequest;

