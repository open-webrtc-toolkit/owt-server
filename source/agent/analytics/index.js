'use strict';
const fs = require('fs');
const toml = require('toml');
const log = require('../logger').logger.getLogger('AnalyticsNode');
const MediaFrameMulticaster = require(
    '../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');

const makeRPC = require('../makeRPC').makeRPC;
const { getVideoParameterForAddon } = require('../mediaUtil');
const VideoAnalyzer = require('../videoGstPipeline/build/Release/videoAnalyzer-pipeline');

var {InternalConnectionRouter} = require('./internalConnectionRouter');

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

module.exports = function (rpcClient, rpcId, agentId, clusterIp) {
  var that = {
    agentID: agentId,
    clusterIP: clusterIp
  };
  var router = new InternalConnectionRouter(global.config.internal);

  // get algorithms from "plugin.cfg"
  var algorithms = {};
  var newStreamId;
  var inputs = {};  //TODO:Each process will only have one input, so this can be modified to input 
  // connectionId - dispatcher
  var outputs = {};
  var engine = new VideoAnalyzer();
  var controller;

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

  // RPC callback
  const rpcSuccess = (callback) => {
    log.info('rpcSuccess callback:',callback);
    return function(result) {
      callback('callback', result || 'ok');
    }
  };
  const rpcError = (callback) => {
    return function(reason) {
      callback('callback', 'error', reason);
    }
  };


  that.getInternalAddress = function(callback) {
    const ip = global.config.internal.ip_address;
    const port = router.internalPort;
    callback('callback', {ip, port});
  };

  that.publish = function (connectionId, connectionType, options, callback) {
    log.debug('publish, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);

    if (connectionType !== 'analytics') {
      return callback('callback', {type: 'failed', reason: 'invalid connctionType'+connectionType});
    }

    var dispatcher = new MediaFrameMulticaster();
    if (engine.addOutput(dispatcher)){
      outputs[connectionId] = {dispatcher: dispatcher};
      router.addLocalSource(newStreamId, connectionType, dispatcher.source());
      callback('callback', 'ok');
    } else {
      callback('callback', 'error', 'Failed in adding output')
    }
  };

  that.unpublish = function (connectionId, callback) {
    log.debug('unpublish, connectionId:', connectionId);
    if (outputs[connectionId]) {
      var output = outputs[connectionId];
      engine.removeOutput(output.dispatcher);
      engine.clearPipeline();
      router.removeLocalSource(newStreamId);
      output.dispatcher.close();
      delete outputs[connectionId];
    }
    this.connectionclose();
    callback('callback', 'ok');
  };

  that.subscribe = function (connectionId, connectionType, options, callback) {
    log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
    if (connectionType !== 'analytics') {
      return callback('callback', {type: 'failed', reason: 'invalid connctionType'+connectionType});
    }

      // check options
    if (!algorithms[options.connection.algorithm]) {
      return callback('callback', {type: 'failed', reason: 'Not valid algorithm:'+options.connection.algorithm});
    }

    if(!inputs[connectionId]) {
      const videoFormat = options.connection.video.format;
      const videoParameters = options.connection.video.parameters;
      const algo = options.connection.algorithm;
      controller = options.controller;

      newStreamId = algo + options.media.video.from;

      const pluginName = algorithms[algo].name;
      let codec = videoFormat.codec;
      if (videoFormat.profile) {
        codec += '_' + videoFormat.profile;
      }
      codec = codec.toLowerCase();
      const {resolution, framerate, keyFrameInterval, bitrate}
          = getVideoParameterForAddon(options.connection.video);

      if ( !engine.createPipeline(codec,resolution,framerate,bitrate,keyFrameInterval,algo,pluginName) ) {
        return Promise.reject('Create pipeline failed');
      }

      const status = {type: 'ready', info: {algorithm: algo}};
      notifyStatus(options.controller, connectionId, 'out', status);

      this.connectionclose = () => {
        destroyStream(options.controller, newStreamId);
      }
      inputs[connectionId] = true;

      engine.addEventListener('fatal', function (error) {
        log.error('GStreamer pipeline error:', error);
        notifyStatus(options.controller, connectionId, 'out', {type: 'failed', reason: 'Analytics error: ' + error});
      });

      const streamInfo = {
        type: 'analytics',
        media: {video: Object.assign({}, videoFormat, videoParameters)},
        analyticsId: connectionId,
        locality: {agent:agentId, node:rpcId},
      };
      engine.addEventListener('streamadded', function (data) {
        log.debug('GStreamer pipeline stream generated:', data);

        var dispatcher = new MediaFrameMulticaster();
        if (engine.addOutput(dispatcher)){
          outputs[connectionId] = {dispatcher: dispatcher};
          router.addLocalSource(newStreamId, connectionType, dispatcher.source());
          callback('callback', 'ok');
        } else {
          callback('callback', 'error', 'Failed in adding output')
        }

        var params;
        try {
          params = JSON.parse(data)
          streamInfo.media.video.bitrate = bitrate;
          if (params.codec) {
            streamInfo.media.video.codec = params.codec;
          }

          if (params.profile) {
            if (params.profile === "constrained-baseline") {
              streamInfo.media.video.profile = 'CB';
            } else if (params.profile === "baseline") {
              streamInfo.media.video.profile = 'B';
            } else if (params.profile === "main") {
              streamInfo.media.video.profile = 'M';
            } else if (params.profile === "high") {
              streamInfo.media.video.profile = 'H';
            } else {
              log.error("Not supported profile:", params.profile);
              return;
            }
          }
          generateStream(options.controller, newStreamId, streamInfo);
        } catch (e) {
          log.error("Parse stream added data with error:", e);
        }
      });
      return callback('callback', 'ok');
    } else {
      callback('callback', 'error', 'Connection already exist: '  + connectionId);
    }
  };

  that.unsubscribe = function (connectionId, callback) {
    log.debug('unsubscribe, connectionId:', connectionId, " inputs:", inputs);
    if (inputs[connectionId]) {
      var conn = router.getConnection(connectionId);
      if (conn) {
        router.removeConnection(connectionId).then(function(ok) {
            conn.close();
            callback('callback', 'ok');
        }, rpcError(callback));
      }
    }
  };

  // streamInfo = {id: 'string', ip: 'string', port: 'number'}
  // from = {audio: streamInfo, video: streamInfo, data: streamInfo}
  that.linkup = function (connectionId, fromInfo, callback) {
    log.debug('linkup, connectionId:', connectionId, 'from:', fromInfo);
    if (inputs[connectionId]) {
      if (engine) {
        var conn = router.getOrCreateRemoteSource({
          id: fromInfo.video.id,
          ip: fromInfo.video.ip,
          port: fromInfo.video.port
        }, function onStats(stat) {
          if (stat === 'disconnected') {
            log.debug('Internal connection closed:', fromInfo.video.id);
            router.destroyRemoteSource(fromInfo.video.id);
            callback('callback', 'error', 'Failed in adding input to analytics.');
            return;
          }
        });

        if (engine.linkInput(conn)) {
          log.debug('linkInput ok, stream_id:', fromInfo.video.id);
          callback('callback', 'ok');
        } else {
          router.destroyRemoteSource(fromInfo.video.id);
          callback('callback', 'error', 'Failed in linking input to analytics.');
        }
      }
    }
  };

  that.cutoff = function (connectionId, callback) {
    log.debug('cutoff, connectionId:', connectionId);
    router.cutoff(connectionId).then(onSuccess(callback), onError(callback));
  };

  that.onFaultDetected = function (message) {
    if (message.purpose === 'conference' && controller) {
      if ((message.type === 'node' && message.id === controller) ||
        (message.type === 'worker' && controller.startsWith(message.id))) {
        log.error('Conference controller (type:', message.type, 'id:', message.id, ') fault is detected, exit.');
        process.exit();
      }
    }
  };

  return that;
};
