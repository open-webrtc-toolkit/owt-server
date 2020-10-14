// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var RpcRequest = function(rpcChannel) {
  var that = {};

  that.getRoomConfig = function(configServer, sessionId) {
    return rpcChannel.makeRPC(configServer, 'getRoomConfig', sessionId);
  };

  that.getWorkerNode = function(clusterManager, purpose, forWhom, preference) {
    return rpcChannel.makeRPC(clusterManager, 'schedule', [purpose, forWhom.task, preference, 30 * 1000])
      .then(function(workerAgent) {
        return rpcChannel.makeRPC(workerAgent.id, 'getNode', [forWhom])
          .then(function(workerNode) {
            return {agent: workerAgent.id, node: workerNode};
          });
      });
  };

  that.recycleWorkerNode = function(workerAgent, workerNode, forWhom) {
    return rpcChannel.makeRPC(workerAgent, 'recycleNode', [workerNode, forWhom])
      . catch((result) => {
        return 'ok';
      }, (err) => {
        return 'ok';
      });
  };

  that.initiate = function(accessNode, sessionId, sessionType, direction, Options) {
    if (direction === 'in') {
      return rpcChannel.makeRPC(accessNode, 'publish', [sessionId, sessionType, Options]);
    } else if (direction === 'out') {
      return rpcChannel.makeRPC(accessNode, 'subscribe', [sessionId, sessionType, Options]);
    } else {
      return Promise.reject('Invalid direction');
    }
  };

  that.terminate = function(accessNode, sessionId, direction/*FIXME: direction should be unneccesarry*/) {
    if (direction === 'in') {
      return rpcChannel.makeRPC(accessNode, 'unpublish', [sessionId]).catch((e) => { return 'ok';});
    } else if (direction === 'out') {
      return rpcChannel.makeRPC(accessNode, 'unsubscribe', [sessionId]).catch((e) => { return 'ok';});
    } else {
      return Promise.resolve('ok');
    }
  };

  that.onTransportSignaling = function(accessNode, transportId, signaling) {
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

  return that;
};

module.exports = RpcRequest;

