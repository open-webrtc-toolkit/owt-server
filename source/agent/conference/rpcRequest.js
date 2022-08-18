// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const grpcTools = require('./grpcTools');
const packOption = grpcTools.packOption;
const unpackNotification = grpcTools.unpackNotification;

const makeRPC = require('./makeRPC').makeRPC;
const log = require('./logger').logger.getLogger('RpcRequest');

const enableGrpc = global.config.agent.enable_grpc || false;

const RpcRequest = function(rpcChannel, listener) {
  const that = {};
  const grpcAgents = {}; // workerAgent => grpcClient
  const grpcNode = {}; // workerNode => grpcClient
  const nodeType = {}; // NodeId => Type
  let clusterClient;
  let sipPortalClient;

  that.getRoomConfig = function(configServer, sessionId) {
    return rpcChannel.makeRPC(configServer, 'getRoomConfig', sessionId);
  };

  that.getWorkerNode = function(clusterManager, purpose, forWhom, preference) {
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
        clusterClient.schedule(req, (err, result) => {
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
          grpcAgents[agentAddress].getNode({info: forWhom}, (err, result) => {
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
        if (purpose === 'quic') {
          // Register validate callback
          const call = grpcNode[workerNode].validateTokenCallback();
          call.on('data', (token) => {
            // Validate token
            listener.processCallback(token, (result) => {
              call.write({tokenId: token.tokenId, ok: result});
            });
          });
          call.on('end', () => {
          });
          call.on('error', (err) => {
            // On error
            log.debug('Call on error:', err);
          });
        }
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
        grpcAgents[workerAgent].recycleNode(req, (err, result) => {
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

  that.initiate = function(accessNode, sessionId, sessionType, direction, Options) {
    if (grpcNode[accessNode]) {
      // Use GRPC
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
      // Use GRPC
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
      // Use GRPC
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
    // GRPC for webrtc-agent
    if (grpcNode[accessNode]) {
      // Use GRPC
      const req = {id: transportId};
      return new Promise((resolve, reject) => {
        grpcNode[accessNode].destroyTransport(req, (err, result) => {
          if (!err) {
            resolve(result);
          } else {
            reject(err);
          }
        });
      });
    }
    return rpcChannel.makeRPC(accessNode, 'destroyTransport', [transportId]);
  }

  that.mediaOnOff = function(accessNode, sessionId, track, direction, onOff) {
    if (grpcNode[accessNode]) {
      // Use GRPC
      const req = {
        id: sessionId,
        tracks: track,
        direction: direction,
        action: onOff
      };
      return new Promise((resolve, reject) => {
        grpcNode[accessNode].sessionControl(req, (err, result) => {
          if (!err) {
            resolve(result);
          } else {
            reject(err);
          }
        });
      });
    }
    return rpcChannel.makeRPC(accessNode, 'mediaOnOff', [sessionId, track, direction, onOff]);
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

  that.dropUser = function(portal, participantId) {
    if (enableGrpc) {
      return Promise.resolve();
    }
    return rpcChannel.makeRPC(portal, 'drop', [participantId]);
  };

  that.getSipConnectivity = function(sipPortal, roomId) {
    if (enableGrpc) {
      if (!sipPortalClient) {
        const sipPortal = global.config.cluster.sip_portal || 'localhost:9090';
        sipPortalClient = startClient('sipPortal', sipPortal);
      }
      return new Promise((resolve, reject) => {
        sipPortalClient.getSipConnectivity({id: roomId}, (err, result) => {
          if (!err) {
            const sipNode = result.message;
            grpcNode[sipNode] = grpcTools.startClient('sip', sipNode);
            resolve(sipNode);
          } else {
            reject(err);
          }
        });
      });
    }
    return rpcChannel.makeRPC(sipPortal, 'getSipConnectivity', [roomId]);
  };

  that.makeSipCall = function(sipNode, peerURI, mediaIn, mediaOut, controller) {
    if (grpcNode[sipNode]) {
      // Use GRPC
      return new Promise((resolve, reject) => {
        const req = {
          peerUri: peerURI,
          mediaIn: mediaIn,
          mediaOut: mediaOut,
          controller: controller
        };
        grpcNode[sipNode].makeCall(req, (err, result) => {
          if (!err) {
            resolve(result.message);
          } else {
            reject(err);
          }
        });
      });
    }
    return rpcChannel.makeRPC(sipNode, 'makeCall', [peerURI, mediaIn, mediaOut, controller]);
  };

  that.endSipCall = function(sipNode, sipCallId) {
    if (grpcNode[sipNode]) {
      // Use GRPC
      return new Promise((resolve, reject) => {
        grpcNode[sipNode].endSipCall({id: sipCallId}, (err, result) => {
          if (!err) {
            resolve(result.message);
          } else {
            reject(err);
          }
        });
      });
    }
    return rpcChannel.makeRPC(sipNode, 'endCall', [sipCallId]);
  };

  that.validateAndDeleteWebTransportToken = function(portal, token) {
    if (enableGrpc) {
      return Promise.resolve();
    }
    return rpcChannel.makeRPC(
      portal, 'validateAndDeleteWebTransportToken', [token]);
  };

  that.makeRPC = function (_, remoteNode, rpcName, parameters, onOk, onError) {
    if (grpcNode[remoteNode]) {
      if (rpcName === 'linkup') {
        const req = {
          id: parameters[0],
          from: parameters[1],
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
      } else if (rpcName === 'getInternalAddress') {
        grpcNode[remoteNode].getInternalAddress({}, (err, result) => {
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
      } else if (rpcName === 'init') {
        const type = nodeType[remoteNode];
        const initOption = {
          service: parameters[0],
          controller: parameters[3],
          label: parameters[4],
          init: parameters[1]
        };
        const req = {
          id: parameters[2],
          type: type,
          option: packOption(type, initOption)
        };
        grpcNode[remoteNode].init(req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'generate') {
        const req = {
          id: parameters[0],
          media: {}
        };
        if (parameters.length === 2) {
          req.media.audio = {format: {codec: parameters[1]}};
        } else if (parameters.length === 5) {
          parameters = parameters.map(
              (par) => (par === 'unspecified' ? undefined : par));
          req.media.video = {
            format: {codec: parameters[0]},
            parameters: {
              resolution: parameters[1],
              framerate: parameters[2],
              bitrate: parameters[3],
              keyFrameInterval: parameters[4]
            }
          };
        }
        grpcNode[remoteNode].generate(req, (err, result) => {
          if (!err) {
            const resp = req.media.audio ? result.id : result;
            onOk && onOk(resp);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'degenerate') {
        const req = {id: parameters[0]};
        grpcNode[remoteNode].degenerate(req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'enableVAD') {
        const req = {periodMs: parameters[0]};
        grpcNode[remoteNode].enableVad(req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'resetVAD' || rpcName === 'deinit') {
        const req = {};
        if (rpcName === 'resetVAD') {
          rpcName = 'resetVad';
        }
        grpcNode[remoteNode][rpcName](req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'setInputActive') {
        const req = {
          id: parameters[0],
          active: parameters[1]
        };
        grpcNode[remoteNode][rpcName](req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'getRegion') {
        const req = {id: parameters[0]};
        grpcNode[remoteNode][rpcName](req, (err, result) => {
          if (!err) {
            onOk && onOk(result.message);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'setRegion') {
        const req = {
          streamId: parameters[0],
          regionId: parameters[1]
        };
        grpcNode[remoteNode][rpcName](req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'setLayout') {
        const req = {regions: parameters[0]};
        grpcNode[remoteNode][rpcName](req, (err, result) => {
          if (!err) {
            onOk && onOk(result.regions);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'setPrimary' || rpcName === 'forceKeyFrame') {
        const req = {id: parameters[0]};
        grpcNode[remoteNode][rpcName](req, (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else {
        log.error('Unknow rpc name:', rpcName);
      }
    } else {
      if (enableGrpc) {
        log.warn('Unknown gRPC node:', remoteNode);
      }
      return makeRPC(_, remoteNode, rpcName, parameters, onOk, onError);
    }
  };

  that.addSipNode = function (workerNode) {
    grpcNode[workerNode] = grpcTools.startClient(
      'sip',
      workerNode
    );
  }

  return that;
};

module.exports = RpcRequest;

