/* global require */
'use strict';

var RpcClient = function(rpcChannel) {
  var that = {};

  that.tokenLogin = function(nuve, tokenId) {
    return rpcChannel.makeRPC(nuve, 'deleteToken', tokenId);
  };

  that.getController = function(clusterManager, sessionId) {
    return rpcChannel.makeRPC(clusterManager, 'schedule', ['session', sessionId, 30 * 1000])
      .then(function(controllerAgent) {
        return rpcChannel.makeRPC(controllerAgent.id, 'getNode', [{session: sessionId, consumer: sessionId}]);
      });
  };

  that.join = function(controller, sessionId, participant) {
    return rpcChannel.makeRPC(controller, 'join', [sessionId, participant]);
  };

  that.leave = function(controller, participantId) {
    rpcChannel.makeRPC(controller, 'leave', [participantId]);
    return Promise.resolve('ok');
  };

  that.getAccessNode = function(clusterManager, purpose, forWhom) {
    return rpcChannel.makeRPC(clusterManager, 'schedule', [purpose, forWhom.session, 30 * 1000])
      .then(function(accessAgent) {
        return rpcChannel.makeRPC(accessAgent.id, 'getNode', [{session: forWhom.session, consumer: forWhom.consumer}])
          .then(function(accessNode) {
            return {agent: accessAgent.id, node: accessNode};
          });
      });
  };

  that.recycleAccessNode = function(accessAgent, accessNode, forWhom) {
    rpcChannel.makeRPC(accessAgent, 'recycleNode', [accessNode, {session: forWhom.session, consumer: forWhom.consumer}]);
    return Promise.resolve('ok');
  };

  that.publish = function(accessNode, connectionId, connectionType, Options, onConnectionStatus) {
    return rpcChannel.makeRPC(accessNode, 'publish', [connectionId, connectionType, Options], onConnectionStatus);
  };

  that.unpublish = function(accessNode, connectionId) {
    rpcChannel.makeRPC(accessNode, 'unpublish', [connectionId]);
    return Promise.resolve('ok');
  };

  that.subscribe = function(accessNode, connectionId, connectionType, Options, onConnectionStatus) {
    return rpcChannel.makeRPC(accessNode, 'subscribe', [connectionId, connectionType, Options], onConnectionStatus);
  };

  that.unsubscribe = function(accessNode, connectionId) {
    rpcChannel.makeRPC(accessNode, 'unsubscribe', [connectionId]);
    return Promise.resolve('ok');
  };

  that.onConnectionSignalling = function(accessNode, connectionId, signaling) {
    return rpcChannel.makeRPC(accessNode, 'onConnectionSignalling', [connectionId, signaling]);
  };

  that.pub2Session = function(controller, participantId, streamId, accessNode, streamDescription, notMix) {
    return rpcChannel.makeRPC(controller, 'publish', [participantId, streamId, accessNode, streamDescription, !!notMix]);
  };

  that.unpub2Session = function(controller, participantId, streamId) {
    rpcChannel.makeRPC(controller, 'unpublish', [participantId, streamId]);
    return Promise.resolve('ok');
  };

  that.sub2Session = function(controller, participantId, subscriptionId, accessNode, subscriptionDescription) {
    return rpcChannel.makeRPC(controller, 'subscribe', [participantId, subscriptionId, accessNode, subscriptionDescription]);
  };

  that.unsub2Session = function(controller, participantId, subscriptionId) {
    rpcChannel.makeRPC(controller, 'unsubscribe', [participantId, subscriptionId]);
    return Promise.resolve('ok');
  };

  that.mix = function(controller, participantId, streamId) {
    return rpcChannel.makeRPC(controller, 'mix', [participantId, streamId]);
  };

  that.unmix = function(controller, participantId, streamId) {
    return rpcChannel.makeRPC(controller, 'unmix', [participantId, streamId]);
  };

  that.setVideoBitrate = function(accessNode, connectionId, bitrate) {
    return rpcChannel.makeRPC(accessNode, 'setVideoBitrate', [connectionId, bitrate]);
  };

  that.mediaOnOff = function(accessNode, connectionId, track, direction, onOff) {
    return rpcChannel.makeRPC(accessNode, 'mediaOnOff', [connectionId, track, direction, onOff]);
  };

  that.getRegion = function(controller, subStreamId) {
    return rpcChannel.makeRPC(controller, 'getRegion', [subStreamId]);
  };

  that.setRegion = function(controller, subStreamId, regionId) {
    return rpcChannel.makeRPC(controller, 'setRegion', [subStreamId, regionId]);
  };

  that.text = function(controller, fromWhom, toWhom, message) {
    return rpcChannel.makeRPC(controller, 'text', [fromWhom, toWhom, message]);
  };

  return that;
}

module.exports = RpcClient;

