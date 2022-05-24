// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const grpcTools = require('./grpcTools');
const packOption = grpcTools.packOption;
const makeRPC = require('./makeRPC').makeRPC;

var RpcRequest = function(rpcChannel, listener) {
  var that = {};
  var grpcNode = {};
  var listener = listener;

  that.getRoomConfig = function(configServer, sessionId) {
    return rpcChannel.makeRPC(configServer, 'getRoomConfig', sessionId);
  };

  that.getWorkerNode = function(clusterManager, purpose, forWhom, preference) {
    return rpcChannel.makeRPC(clusterManager, 'schedule', [purpose, forWhom.task, preference, 30 * 1000])
      .then(function(workerAgent) {
        return rpcChannel.makeRPC(workerAgent.id, 'getNode', [forWhom])
          .then(function(workerNode) {
            if (purpose === 'webrtc') {
              grpcNode[workerNode] = grpcTools.startClient(
                purpose,
                workerNode
              );
              // Register listener
              const call = grpcNode[workerNode].listenToNotifications({id: ''});
              call.on('data', (notification) => {
                if (listener) {
                  // Unpack notification.data
                  listener.processNotification(notification);
                }
              });
              call.on('end', () => {});
            }
            return {agent: workerAgent.id, node: workerNode};
          });
      });
  };

  that.recycleWorkerNode = function(workerAgent, workerNode, forWhom) {
    return rpcChannel.makeRPC(workerAgent, 'recycleNode', [workerNode, forWhom])
      . catch((result) => {
        if (grpcNode[workerNode]) {
          delete grpcNode[workerAgent];
        }
        return 'ok';
      }, (err) => {
        if (grpcNode[workerNode]) {
          delete grpcNode[workerAgent];
        }
        return 'ok';
      });
  };

  that.initiate = function(accessNode, sessionId, sessionType, direction, Options) {
    if (grpcNode[accessNode]) {
      // Test GRPC
      // console.log('GRPC initiate:', sessionId, sessionType);
      return new Promise((resolve, reject) => {
        if (direction === 'in') {
          const req = {
            id: sessionId,
            type: sessionType,
            option: packOption(sessionType, Options)
          };
          grpcNode[accessNode].publish(req, (err, result) => {
            if (!err) {
              resolve(result);
            } else {
              reject(err);
            }
          });
        } else if (direction === 'out') {
          const req = {
            id: sessionId,
            type: sessionType,
            option: packOption(sessionType, Options)
          };
          grpcNode[accessNode].subscribe(req, (err, result) => {
            if (!err) {
              resolve(result);
            } else {
              reject(err);
            }
          });
        } else {
          reject('Invalid direction')
        }
      });
    }
    if (direction === 'in') {
      return rpcChannel.makeRPC(accessNode, 'publish', [sessionId, sessionType, Options]);
    } else if (direction === 'out') {
      return rpcChannel.makeRPC(accessNode, 'subscribe', [sessionId, sessionType, Options]);
    } else {
      return Promise.reject('Invalid direction');
    }
  };

  that.terminate = function(accessNode, sessionId, direction/*FIXME: direction should be unneccesarry*/) {
    if (grpcNode[accessNode]) {
      // Test GRPC
      return new Promise((resolve, reject) => {
        if (direction === 'in') {
          const req = {id: sessionId};
          grpcNode[accessNode].unpublish(req, (err, result) => {
            if (!err) {
              resolve(result);
            } else {
              reject(err);
            }
          });
        } else if (direction === 'out') {
          const req = {id: sessionId};
          grpcNode[accessNode].unsubscribe(req, (err, result) => {
            if (!err) {
              resolve(result);
            } else {
              reject(err);
            }
          });
        } else {
          reject('Invalid direction')
        }
      });
    }
    if (direction === 'in') {
      return rpcChannel.makeRPC(accessNode, 'unpublish', [sessionId]).catch((e) => { return 'ok';});
    } else if (direction === 'out') {
      return rpcChannel.makeRPC(accessNode, 'unsubscribe', [sessionId]).catch((e) => { return 'ok';});
    } else {
      return Promise.resolve('ok');
    }
  };

  that.onTransportSignaling = function(accessNode, transportId, signaling) {
    // GRPC for webrtc-agent
    if (grpcNode[accessNode]) {
      // Test GRPC
      return new Promise((resolve, reject) => {
        const req = {
          id: transportId,
          signaling: signaling,
        };
        grpcNode[accessNode].processSignaling(req, (err, result) => {
          if (!err) {
            resolve(result);
          } else {
            reject(err);
          }
        });
      });
    }
    return rpcChannel.makeRPC(accessNode, 'onTransportSignaling', [transportId, signaling]);
  };

  that.destroyTransport = function(accessNode, transportId) {
    return rpcChannel.makeRPC(accessNode, 'destroyTransport', [transportId]);
  }

  that.mediaOnOff = function(accessNode, sessionId, track, direction, onOff) {
    return rpcChannel.makeRPC(accessNode, 'mediaOnOff', [sessionId, track, direction, onOff]);
  };

  that.sendMsg = function(portal, participantId, event, data) {
    return rpcChannel.makeRPC(portal, 'notify', [participantId, event, data]);
  };

  that.broadcast = function(portal, controller, excludeList, event, data) {
    return rpcChannel.makeRPC(portal, 'broadcast', [controller, excludeList, event, data]);
  };

  that.dropUser = function(portal, participantId) {
    return rpcChannel.makeRPC(portal, 'drop', [participantId]);
  };

  that.getSipConnectivity = function(sipPortal, roomId) {
    return rpcChannel.makeRPC(sipPortal, 'getSipConnectivity', [roomId]);
  };

  that.makeSipCall = function(sipNode, peerURI, mediaIn, mediaOut, controller) {
    return rpcChannel.makeRPC(sipNode, 'makeCall', [peerURI, mediaIn, mediaOut, controller]);
  };

  that.endSipCall = function(sipNode, sipCallId) {
    return rpcChannel.makeRPC(sipNode, 'endCall', [sipCallId]);
  };

  that.makeRPC = function (_, remoteNode, rpcName, parameters, onOk, onError) {
    if (grpcNode[remoteNode]) {
      if (rpcName === 'linkup') {
        const req = {
          id: parameters[0],
          from: {
            audio: {id: parameters[1]},
            video: {id: parameters[2]},
            data: {id: parameters[3]}
          }
        };
        grpcNode[remoteNode].linkup(req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'cutoff') {
        const req = {id: parameters[0]};
        grpcNode[remoteNode].cutoff(req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'createInternalConnection') {
        const req = {
          id: parameters[0],
          direction: parameters[1],
          internalOpt: parameters[2]
        };
        grpcNode[remoteNode].createInternalConnection(req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'destroyInternalConnection') {
        const req = {
          id: parameters[0],
          direction: parameters[1]
        };
        grpcNode[remoteNode].destroyInternalConnection(req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'publish' || rpcName === 'subscribe') {
        const direction = (rpcName === 'publish') ? 'in' : 'out';
        that.initiate(remoteNode,
            parameters[0], parameters[1], direction, parameters[2])
          .then((result) => {
            onOk && onOk(result);
          })
          .catch((err) => {
            onError && onError(err);
          });
      } else if (rpcName === 'unpublish' || rpcName === 'unsubscribe') {
        const direction = (rpcName === 'unpublish') ? 'in' : 'out';
        that.terminate(remoteNode,
          parameters[0], direction)
        .then((result) => {
          onOk && onOk(result);
        })
        .catch((err) => {
          onError && onError(err);
        });
      } else {
        console.log('Unknow rpc name!');
      }
    } else {
      return makeRPC(_, remoteNode, rpcName, parameters, onOk, onError);
    }
  };

  return that;
};

module.exports = RpcRequest;

