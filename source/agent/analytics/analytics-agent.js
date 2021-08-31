'use strict';

const log = require('../logger').logger.getLogger('AnalyticsAgent');
const BaseAgent = require('./base-agent');

const VideoAnalyzer = require('../videoGstPipeline/build/Release/videoAnalyzer-pipeline');
const EventEmitter = require('events').EventEmitter;
const { getVideoParameterForAddon } = require('../mediaUtil'); 

class AnalyticsAgent extends BaseAgent {
  constructor(config) {
    super('analytics', config);
    this.algorithms = config.algorithms;
    this.onStatus = config.onStatus;
    this.onStreamGenerated = config.onStreamGenerated;
    this.onStreamDestroyed = config.onStreamDestroyed;

    this.agentId = config.agentId;
    this.rpcId = config.rpcId;
    // connectionId - {engine, options, output, videoFrom}
    this.inputs = {};
    // connectionId - dispatcher
    this.outputs = {};

    this.engine = new VideoAnalyzer();

    this.flag = 0;
    this.ticket = null;
  }

// override
createInternalConnection(connectionId, direction, internalOpt) {
    internalOpt.minport = global.config.internal.minport;
    internalOpt.maxport = global.config.internal.maxport;
    this.ticket = internalOpt.ticket;
    if (direction == 'in') {
      this.engine.emitListenTo(internalOpt.minport,internalOpt.maxport, this.ticket);
      const portInfo = this.engine.getListeningPort();
      // Create internal connection always success
      return Promise.resolve({ip: global.config.internal.ip_address, port: portInfo});
    }
    else {
      return super.createInternalConnection(connectionId, direction, internalOpt);
    }
  }

  // override
  publish(connectionId, connectionType, options) {
    log.debug('publish:', connectionId, connectionType, options);
    if (connectionType !== 'analytics') {
      return Promise.resolve("ok");
    }
    // should not be reached
    return Promise.reject('no analytics publish');
  }

  // override
  unpublish(connectionId) {
    log.debug('unpublish:', connectionId);
    this.engine.clearPipeline();
    this.connectionclose();
    return Promise.resolve("ok");
  }

  // override
  subscribe(connectionId, connectionType, options) { 
    log.debug('subscribe:', connectionId, connectionType, JSON.stringify(options));
    if (connectionType !== 'analytics') {
       this.outputs[connectionId] = true;
       return super.subscribe(connectionId, connectionType, options);
    }

    // check options
    if (!this.algorithms[options.connection.algorithm]) {
      return Promise.reject('Not valid algorithm');
    }

    if(!this.inputs[connectionId]) {
      const videoFormat = options.connection.video.format;
      const videoParameters = options.connection.video.parameters;
      const algo = options.connection.algorithm;
      const status = {type: 'ready', info: {algorithm: algo}};
      this.onStatus(options.controller, connectionId, 'out', status);

      const newStreamId = algo + options.media.video.from;

      const pluginName = this.algorithms[algo].name;
      let codec = videoFormat.codec;
            if (videoFormat.profile) {
              codec += '_' + videoFormat.profile;
            }
      codec = codec.toLowerCase();
      const {resolution, framerate, keyFrameInterval, bitrate}
              = getVideoParameterForAddon(options.connection.video);

      log.debug('resolution:',resolution,'framerate:',framerate,'keyFrameInterval:',
               keyFrameInterval, 'bitrate:',bitrate);

      this.engine.setInputParam(codec,resolution,framerate,bitrate,keyFrameInterval,algo,pluginName);
      if (this.engine.createPipeline() < 0) {
        return Promise.reject('Create pipeline failed');
      }

      if (this.engine.addElementMany() < 0) {
        return Promise.reject('Link element failed');
      }

      this.connectionclose = () => {
          this.onStreamDestroyed(options.controller, newStreamId);
      }
      this.inputs[connectionId] = true;

      var notifyStatus = this.onStatus;
      this.engine.addEventListener('fatal', function (error) {
          log.error('GStreamer pipeline error:', error);
          notifyStatus(options.controller, connectionId, 'out', {type: 'failed', reason: 'Analytics error: ' + error});
      });

      var generateStream = this.onStreamGenerated;
      const streamInfo = {
        type: 'analytics',
        media: {video: Object.assign({}, videoFormat, videoParameters)},
        analyticsId: connectionId,
        locality: {agent:this.agentId, node:this.rpcId},
      };
      this.engine.addEventListener('streamadded', function (data) {
          log.debug('GStreamer pipeline stream generated:', data);
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

      return Promise.resolve();
    }
    return Promise.reject('Connection already exist: ' + connectionId);
  }

  // override
  unsubscribe(connectionId) {
    log.debug('unsubscribe:', connectionId);
    if (this.outputs[connectionId]) {
      var iConn;
      log.debug('disconnect connection id:', connectionId);
      iConn = this.connections.getConnection(connectionId);
      if (iConn)
      {
        this.engine.disconnect(iConn.connection.receiver());
      }
    }
    return super.unsubscribe(connectionId);
  }

  // override
  linkup(connectionId, audioFrom, videoFrom) {
    log.debug('linkup:', connectionId, audioFrom, videoFrom);
    if (this.inputs[connectionId]) {
      this.engine.setPlaying();
    }

    if (this.outputs[connectionId]) {
      var iConn;
      log.debug('linkup with connection id:', connectionId);
      iConn = this.connections.getConnection(connectionId);
      if (iConn && iConn.direction === 'out' && !iConn.videoFrom)
      {
        this.engine.addOutput(connectionId, iConn.connection.receiver());
      }
    }

    return Promise.resolve();
  }

  // override
  cutoff(connectionId) {
    log.debug('cutoff:', connectionId);
    return Promise.resolve();
  }

  cleanup() {
    log.debug('cleanup');
    return Promise.resolve();
  }
}


module.exports = AnalyticsAgent;
