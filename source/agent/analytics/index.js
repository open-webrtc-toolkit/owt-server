'use strict';
const fs = require('fs');
const toml = require('toml');
const log = require('../logger').logger.getLogger('AnalyticsNode');

const AnalyticsAgent = require('./analytics-agent');
const makeRPC = require('../makeRPC').makeRPC;

function doPublish(rpcClient, conference, user, streamId, streamInfo) {
  return new Promise(function(resolve, reject) {
      makeRPC(
          rpcClient,
          conference,
          'publish',
          [user, streamId, streamInfo],
          resolve,
          reject);
  });
}

function doUnpublish(rpcClient, conference, user, streamId) {
    return new Promise(function(resolve, reject) {
        makeRPC(
            rpcClient,
            conference,
            'unpublish',
            [user, streamId],
            resolve,
            reject);
    });
}

// clusterConfig = { clusterIp, agentId, rpcId, networkInterfaces }
module.exports = function (rpcClient, rpcId, agentId, clusterIp) {
  // get algorithms from "plugin.cfg"
  var algorithms = {};
  try {
    algorithms = toml.parse(fs.readFileSync('./plugin.cfg'));
  } catch (parseError) {
    log.warn('Failed to get plugin config', parseError);
  }

  const notifyStatus = (controller, sessionId, direction, status) => {
    rpcClient.remoteCast(controller, 'onSessionProgress', [sessionId, direction, status]);
  };

  // for algorithms that generate new stream
  const generateStream = (controller, streamId, streamInfo) => {
    log.debug('generateStream:', controller, streamId, streamId);
    doPublish(rpcClient, controller, 'admin', streamId, streamInfo)
      .then(() => {
        log.debug('Generated stream:', streamId, 'publish success');
      })
      .catch((err) => {
        log.debug('Generated stream:', streamId, 'publish failed', err);
      });
  };
  // destroy generated stream
  const destroyStream = (controller, streamId) => {
    log.debug('destroyStream:', controller, streamId);
    doUnpublish(rpcClient, controller, 'admin', streamId)
      .then(() => {
        log.debug('Destroyed stream:', streamId, 'unpublish success');
      })
      .catch((err) => {
        log.debug('Destroyed stream:', streamId, 'unpublish failed', err);
      });
  };

  const config = {
    // BaseAgent
    clusterIp: clusterIp,
    agentId: agentId,
    rpcId: rpcId,
    // AnalyticsAgent
    algorithms,
    onStatus: notifyStatus,
    onStreamGenerated: generateStream,
    onStreamDestroyed: destroyStream,
  };
  const agent = new AnalyticsAgent(config);

  // RPC callback
  const rpcSuccess = (callback) => {
    log.info('rpcSuccess callback:',callback);
    return function(result) {
      log.info('rpcSuccess result:',result);
      callback('callback', result || 'ok');
    }
  };
  const rpcError = (callback) => {
    return function(reason) {

      callback('callback', 'error', reason);
    }
  };

  // RPC API
  return {
    createInternalConnection: function (connectionId, direction, internalOpt, callback) {
     log.debug('createInternalConnection,','connectionId:',connectionId,'direction:',direction,'internalOpt:',internalOpt,'callback:',callback);
      agent.createInternalConnection(connectionId, direction, internalOpt)
        .then(rpcSuccess(callback))
        .catch(rpcError(callback));
    },
    destroyInternalConnection: function (connectionId, direction, callback) {
      agent.destroyInternalConnection(connectionId, direction)
        .then(rpcSuccess(callback))
        .catch(rpcError(callback));
    },
    publish: function (connectionId, connectionType, options, callback) {
     log.debug('publish,connectionId:',connectionId,'connectionType:',connectionType,'options:',options,'callback:',callback);
      agent.publish(connectionId, connectionType, options)
        .then(rpcSuccess(callback))
        .catch(rpcError(callback));
    },
    unpublish: function (connectionId, callback) {
      log.debug('unpublish:',connectionId);
      agent.unpublish(connectionId)
        .then(rpcSuccess(callback))
        .catch(rpcError(callback));
    },
    subscribe: function (connectionId, connectionType, options, callback) {
      log.debug('subscribe,connectionId:',connectionId,'connectionType:',connectionType,'options:',options,'callback:',callback);
      agent.subscribe(connectionId, connectionType, options)
        .then(rpcSuccess(callback))
        .catch(rpcError(callback));
    },
    unsubscribe: function (connectionId, callback) {
      log.debug("unsubscribe",connectionId);
      agent.unsubscribe(connectionId)
        .then(rpcSuccess(callback))
        .catch(rpcError(callback));
    },
    linkup: function (connectionId, audioFrom, videoFrom, dataFrom, callback) {
      log.debug('linkup,','connectionId:',connectionId,'videoFrom:',videoFrom,'callback:',callback);
      agent.linkup(connectionId, audioFrom, videoFrom, dataFrom)
        .then(rpcSuccess(callback))
        .catch(rpcError(callback));
    },
    cutoff: function (connectionId, callback) {
      log.debug("cutoff",connectionId);
      agent.cutoff(connectionId)
        .then(rpcSuccess(callback))
        .catch(rpcError(callback));
    },
    /*close: function(callback) {
      agent.cleanup()
        .then(rpcSuccess(callback))
        .catch(rpcError(callback));
    },*/
    onFaultDetected: function (message) {
      log.debug("onFaultDetected",message);
      if (message.purpose === 'conference') {
        agent.cleanup();
      }
    },
  };
};
