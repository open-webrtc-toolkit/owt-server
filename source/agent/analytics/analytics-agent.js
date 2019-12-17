'use strict';

const log = require('../logger').logger.getLogger('AnalyticsAgent');
const BaseAgent = require('./base-agent');

const VideoPipeline = require('../videoGstPipeline/build/Release/videoAnalyzer-pipeline');

const EventEmitter = require('events').EventEmitter;
const { getVideoParameterForAddon } = require('../mediaUtil');

var portInfo = 0; 
var count = 0;


class AnalyticsAgent  {
  constructor(config) {
    this.algorithms = config.algorithms;
    this.onStatus = config.onStatus;
    this.onStreamGenerated = config.onStreamGenerated;
    this.onStreamDestroyed = config.onStreamDestroyed;

    this.agentId = config.agentId;
    this.rpcId = config.rpcId;
    this.outputs = {}

    var conf = {
      'hardware': false,
      'simulcast': false,
      'crop': false,
      'gaccplugin': false,
      'MFE_timeout': 0
    };
    this.engine = new VideoPipeline(conf);

    this.flag = 0;
  }

// override
createInternalConnection(connectionId, direction, internalOpt) {
    internalOpt.minport = global.config.internal.minport;
    internalOpt.maxport = global.config.internal.maxport;
    if(direction == 'in'){
      this.engine.emitListenTo(internalOpt.minport,internalOpt.maxport);
      portInfo = this.engine.getListeningPort();
    }
    
    // Create internal connection always success
    return Promise.resolve({ip: global.config.internal.ip_address, port: portInfo});
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
    return Promise.resolve('ok');
  }

  // override
  subscribe(connectionId, connectionType, options) { 
    log.debug('subscribe:', connectionId, connectionType, JSON.stringify(options));
    if (connectionType !== 'analytics') {
       // return super.subscribe(connectionId, connectionType, options);
       if(!this.outputs[connectionId]){
        this.outputs[connectionId] = count;
        this.engine.stopLoop();
        this.engine.emitConnectTo(count,options.ip,options.port);
        count++;
       }
      return Promise.resolve("ok");
    }
      
      const videoFormat = options.connection.video.format;
      const videoParameters = options.connection.video.parameters;
      const algo = options.connection.algorithm;
      const status = {type: 'ready', info: {algorithm: algo}};
      this.onStatus(options.controller, connectionId, 'out', status);

      //const newStreamId = Math.random() * 1000000000000000000 + '';
      const newStreamId = algo + options.media.video.from;
      const streamInfo = {
          type: 'analytics',
          media: {video: Object.assign({}, videoFormat, videoParameters)},
          analyticsId: connectionId,
          locality: {agent:this.agentId, node:this.rpcId},
        };

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
      
      this.engine.setOutputParam(codec,resolution,framerate,bitrate,keyFrameInterval,algo,pluginName);
      this.engine.createPipeline();

      streamInfo.media.video.bitrate = bitrate;
            this.onStreamGenerated(options.controller, newStreamId, streamInfo);

      this.engine.addElementMany();;
      this.connectionclose = () => {
          this.onStreamDestroyed(options.controller, newStreamId);
      }
      return Promise.resolve();
  }

  // override
  unsubscribe(connectionId) {
    log.debug('unsubscribe:', connectionId);
    //this.engine.disconnect(this.outputs[connectionId]);
    this.connectionclose();
    return Promise.resolve();
  }

  // override
  linkup(connectionId, audioFrom, videoFrom) {
    log.debug('linkup:', connectionId, audioFrom, videoFrom);
    this.engine.setPlaying();

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
