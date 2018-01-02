/* global require */
'use strict';

var RpcRequest = function(rpcChannel) {
  var that = {};

  that.getController = function(clusterManager, roomId) {
    return rpcChannel.makeRPC(clusterManager, 'schedule', ['conference', roomId, 'preference'/*TODO: specify preference*/, 30 * 1000])
      .then(function(controllerAgent) {
        return rpcChannel.makeRPC(controllerAgent.id, 'getNode', [{room: roomId, task: roomId}]);
      });
  };

  that.join = function(controller, roomId, participant) {
    return rpcChannel.makeRPC(controller, 'join', [roomId, participant], 6000);
  };

  that.leave = function(controller, participantId) {
    return rpcChannel.makeRPC(controller, 'leave', [participantId]);
  };

  that.text = function(controller, fromWhom, toWhom, message) {
    return rpcChannel.makeRPC(controller, 'text', [fromWhom, toWhom, message], 4000);
  };

  that.publish = function(controller, participantId, streamId, Options) {
    return rpcChannel.makeRPC(controller, 'publish', [participantId, streamId, Options]);
  };

  that.unpublish = function(controller, participantId, streamId) {
    return rpcChannel.makeRPC(controller, 'unpublish', [participantId, streamId]);
  };

  that.streamControl = function(controller, participantId, streamId, command) {
    return rpcChannel.makeRPC(controller, 'streamControl', [participantId, streamId, command], 4000);
  };

  that.subscribe = function(controller, participantId, subscriptionId, Options) {
    return rpcChannel.makeRPC(controller, 'subscribe', [participantId, subscriptionId, Options]);
  };

  that.unsubscribe = function(controller, participantId, subscriptionId) {
    return rpcChannel.makeRPC(controller, 'unsubscribe', [participantId, subscriptionId]);
  };

  that.subscriptionControl = function(controller, participantId, subscriptionId, command) {
    return rpcChannel.makeRPC(controller, 'subscriptionControl', [participantId, subscriptionId, command]);
  };

  that.onSessionSignaling = function(controller, sessionId, signaling) {
    return rpcChannel.makeRPC(controller, 'onSessionSignaling', [sessionId, signaling]);
  };

  return that;
};

module.exports = RpcRequest;

