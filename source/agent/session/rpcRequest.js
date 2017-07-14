/* global require */
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
    return rpcChannel.makeRPC(workerAgent, 'recycleNode', [workerNode, {room: forWhom.session, task: forWhom.task}])
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
    return new Promise((resolve, reject) => {
      if (direction === 'in') {
        return rpcChannel.makeRPC(accessNode, 'unpublish', [sessionId]);
      } else if (direction === 'out') {
        return rpcChannel.makeRPC(accessNode, 'unsubscribe', [sessionId]);
      } else {
        return Promise.reject('Invalid direction');
      }
    }).then((result) => {
      return 'ok';
    }, (err) => {
      return 'ok';
    });
  };

  that.onSessionSignaling = function(accessNode, sessionId, signaling) {
    return rpcChannel.makeRPC(accessNode, 'onSessionSignaling', [sessionId, signaling]);
  };

  that.mediaOnOff = function(accessNode, sessionId, track, direction, onOff) {
    return rpcChannel.makeRPC(accessNode, 'mediaOnOff', [sessionId, track, direction, onOff]);
  };

  that.sendMsg = function(portal, participantId, event, data) {
    return rpcChannel.makeRPC(portal, 'notify', [participantId, event, data]);
  };

  that.dropUser = function(portal, participantId) {
    return rpcChannel.makeRPC(portal, 'drop', [participantId]);
  };

  return that;
};

module.exports = RpcRequest;

