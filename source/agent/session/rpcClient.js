/* global require */
'use strict';

var RpcClient = function(rpcChannel) {
  var that = {};

  that.getSessionConfig = function(configServer, sessionId) {
    return rpcChannel.makeRPC(configServer, 'getRoomConfig', sessionId);
  };

  that.getMediaNode = function(clusterManager, purpose, forWhom) {
    return rpcChannel.makeRPC(clusterManager, 'schedule', [purpose, forWhom.session, 30 * 1000])
      .then(function(agent) {
        return rpcChannel.makeRPC(agent.id, 'getNode', [forWhom])
          .then(function(node) {
            return {agent: agent.id, node: node};
          });
      });
  };

  that.recycleMediaNode = function(mediaAgent, mediaNode, forWhom) {
    return rpcChannel.makeRPC(mediaAgent, 'recycleNode', [mediaNode, forWhom]);
  };

  that.sendMsg = function(portal, participantId, event, data) {
    return rpcChannel.makeRPC(portal, 'notify', [participantId, event, data]);
  };

  that.dropUser = function(portal, participantId, sessionId) {
    return rpcChannel.makeRPC(portal, 'drop', [participantId, sessionId]);
  };

  that.setMute = function(portal, participantId, streamId, muted) {
    return rpcChannel.makeRPC(portal, 'setMute', [participantId, streamId, muted]);
  };

  that.setPermission = function(portal, targetId, act, value) {
    return rpcChannel.makeRPC(portal, 'setPermission', [targetId, act, value]);
  };

  return that;
}

module.exports = RpcClient;

