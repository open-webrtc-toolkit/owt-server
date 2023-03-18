// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const grpcTools = require('./grpcTools');
const packOption = grpcTools.packOption;
const unpackNotification = grpcTools.unpackNotification;

const makeRPC = require('./makeRPC').makeRPC;
const log = require('./logger').logger.getLogger('RpcRequest');

const enableGrpc = global.config?.service?.enable_grpc || false;
const GRPC_TIMEOUT = 2000;

const RpcRequest = function(rpcChannel, listener) {
  const that = {};
  const grpcAgents = {}; // workerAgent => grpcClient
  const grpcNode = {}; // workerNode => grpcClient
  const nodeType = {}; // NodeId => Type
  let clusterClient;
  const opt = () => ({deadline: new Date(Date.now() + GRPC_TIMEOUT)});

  that.getWorkerNode = function(clusterManager, purpose, forWhom, preference) {
    log.debug('getworker node:', purpose, forWhom, 'enable grpc:', enableGrpc, clusterManager);
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
          purpose,
          task: forWhom.task,
          preference, // Change data for some preference
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
          grpcAgents[agentAddress].getNode({info: forWhom}, opt(), (err, result) => {
            if (!err) {
              resolve(result.message);
            } else {
              reject(err);
            }
          });
        });
      }).then((workerNode) => {
        if (grpcNode[workerNode]) {
          // Has client already
          return {agent: agentAddress, node: workerNode};
        }
        log.debug('Start gRPC client:', purpose, workerNode);
        grpcNode[workerNode] = grpcTools.startClient(
          purpose,
          workerNode
        );
        nodeType[workerNode] = purpose;
        // Register listener
        const call = grpcNode[workerNode].listenToNotifications({id: ''});
        call.on('data', (notification) => {
          if (listener) {
            // Unpack notification.data
            const data = unpackNotification(notification);
            if (data) {
              listener.processNotification(data);
            }
          }
        });
        call.on('end', (err) => {
          log.debug('Call on end:', err);
          if (grpcNode[workerNode]) {
            grpcNode[workerNode].close();
            delete grpcNode[workerNode];
          }
        });
        call.on('error', (err) => {
          // On error
          log.debug('Call on error:', err);
        });
        return {agent: agentAddress, node: workerNode};
      });
    }

    return rpcChannel.makeRPC(clusterManager, 'schedule', [purpose, forWhom.task, preference, 30 * 1000])
      .then(function(workerAgent) {
        return rpcChannel.makeRPC(workerAgent.id, 'getNode', [forWhom])
          .then(function(workerNode) {
            return {agent: workerAgent.id, node: workerNode};
          });
      });
  };

  that.recycleWorkerNode = function(workerAgent, workerNode, forWhom) {
    if (grpcAgents[workerAgent]) {
      const req = {id: workerNode, info: forWhom};
      return new Promise((resolve, reject) => {
        grpcAgents[workerAgent].recycleNode(req, opt(), (err, result) => {
          if (err) {
            log.debug('Recycle node error:', err);
            reject(err);
          }
          if (grpcNode[workerNode]) {
            grpcNode[workerNode].close();
            delete grpcNode[workerNode];
            delete nodeType[workerNode];
          }
          resolve('ok');
        });
      });
    } else {
      return rpcChannel.makeRPC(workerAgent, 'recycleNode', [workerNode, forWhom]);
    }
  };

  that.sendMsg = function(portal, participantId, event, data) {
    if (enableGrpc) {
      return Promise.resolve();
    }
    return rpcChannel.makeRPC(portal, 'notify', [participantId, event, data]);
  };

  that.broadcast = function(portal, controller, excludeList, event, data) {
    if (enableGrpc) {
      return Promise.resolve();
    }
    return rpcChannel.makeRPC(portal, 'broadcast', [controller, excludeList, event, data]);
  };

  return that;
};

module.exports = RpcRequest;

