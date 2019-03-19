'use strict';

const log = require('../logger').logger.getLogger('AnalyticsAgent');
const BaseAgent = require('./base-agent');
const VideoAnalyzer = require('../videoAnalyzer_sw/build/Release/videoAnalyzer-sw');
const MediaFrameMulticaster = require('../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');
const EventEmitter = require('events').EventEmitter;

class AddonEngine {
  constructor() {
    var config = {
      'hardware': false,
      'simulcast': false,
      'crop': false,
      'gaccplugin': false,
      'MFE_timeout': 0
    };
    this.engine = new VideoAnalyzer(config);
  }

  setInput(inputId, codec, inConnection) {
    log.info('addon setInput', inputId, codec);
    this.engine.setInput(inputId, codec, inConnection);
  }

  unsetInput(inputId) {
    log.info('addon unsetInput', inputId);
    this.engine.unsetInput(inputId);
  }

  // dispatcher is MediaFrameMulticaster
  addOutput(outputId, codec, resolution, framerate, bitrate, kfi,
      algo, pluginName, dispatcher) {
    log.info('addon addOutput', outputId);
    this.engine.addOutput(outputId, codec, resolution, framerate, bitrate, kfi, algo, pluginName, dispatcher);
  }

  removeOutput(outputId) {
    log.info('addon removeOutput', outputId);
    this.engine.removeOutput(outputId);
  }

  close() {
    log.info('addon close');
    this.engine.close();
  }
}

class AnalyticsAgent extends BaseAgent {
  constructor(config) {
    super('analytics', config);
    this.algorithms = config.algorithms;
    this.onStatus = config.onStatus;
    this.onStreamGenerated = config.onStreamGenerated;
    this.onStreamDestroyed = config.onStreamDestroyed;
    // connectionId - {engine, options, output, videoFrom}
    this.inputs = {};
    // connectionId - dispatcher
    this.outputs = {};
  }

  // override
  publish(connectionId, connectionType, options) {
    log.debug('publish:', connectionId, connectionType, options);
    if (connectionType !== 'analytics') {
      // call BaseAgent's publish
      return super.publish(connectionId, connectionType, options);
    }
    // should not be reached
    return Promise.reject('no analytics publish');
  }

  // override
  unpublish(connectionId) {
    log.debug('unpublish:', connectionId);
    // call BaseAgent's unpublish
    return super.unpublish(connectionId);
  }

  // override
  subscribe(connectionId, connectionType, options) {
    log.debug('subscribe:', connectionId, connectionType, JSON.stringify(options));
    if (connectionType !== 'analytics') {
      return super.subscribe(connectionId, connectionType, options);
    }

    // check options
    if (!this.algorithms[options.connection.algorithm]) {
      return Promise.reject('Not valid algorithm');
    }

    var engine;
    if (!this.inputs[connectionId]) {
      engine = new AddonEngine();
      const media = options.connection.media;
      const algo = options.connection.algorithm;
      const pluginName = this.algorithms[algo].name;
      this.inputs[connectionId] = {
        media,
        algo,
        pluginName,
        close: () => {}
      };

      this.inputs[connectionId].engine = engine;
      // notify ready
      const status = {type: 'ready', info: {algorithm: algo}};
      this.onStatus(options.controller, connectionId, 'out', status);

      // check if this algorithm has output
      if (this.algorithms[algo].outputfourcc === 'I420') {
        // generated a new stream
        const newStreamId = connectionId + '-' + algo;
        log.info('new stream added', newStreamId);
        const streamInfo = {
          type: 'analytics',
          media: media,
          analyticsId: connectionId,
          locality: {agent:this.agentId, node:this.rpcId},
        };
        if (!this.outputs[newStreamId]) {
          if (this.inputs[connectionId]) {
            this.inputs[connectionId].output = newStreamId;
            this.outputs[newStreamId] = new MediaFrameMulticaster();

            // following default arguments are from video transcoder
            let codec = media.video.codec;
            if (media.video.profile) {
              codec += '_' + media.video.profile;
            }
            codec = codec.toLowerCase();
            const resolution = 'r0x0';
            const framerate = 30;
            const bitrate = 0;
            const kfi = 1000;
            engine.addOutput(newStreamId, codec, resolution, framerate, bitrate, kfi,
              algo, pluginName, this.outputs[newStreamId]);
            this.onStreamGenerated(options.controller, newStreamId, streamInfo);
          }
        } else {
          log.warn('Ignore duplicated output stream id:', newStreamId);
        }

        // destroy generated stream when closing input
        this.inputs[connectionId].close = () => {
          this.onStreamDestroyed(options.controller, newStreamId);
        };
      }
      return Promise.resolve();
    }
    return Promise.reject('Connection already exist: ' + connectionId);
  }

  // override
  unsubscribe(connectionId) {
    log.debug('unsubscribe:', connectionId);
    if (this.inputs[connectionId]) {
      // destroy generated stream if it has
      this.inputs[connectionId].close();
      return this.cutoff(connectionId);
    }
    return super.unsubscribe(connectionId);
  }

  // override
  linkup(connectionId, audioFrom, videoFrom) {
    log.debug('linkup:', connectionId, audioFrom, videoFrom);
    var iConn;
    if (this.inputs[connectionId]) {
      // link inputs
      iConn = this.connections.getConnection(videoFrom);
      if (iConn && iConn.direction === 'in' && !iConn.videoTo) {
        this.inputs[connectionId].videoFrom = videoFrom;
        iConn.videoTo = connectionId;
        let codec = this.inputs[connectionId].media.video.codec;
        codec = codec.toLowerCase();
        this.inputs[connectionId].engine.setInput(videoFrom, codec, iConn.connection);
        return Promise.resolve();
      }
    }
    if (this.outputs[videoFrom]) {
      // link outputs
      iConn = this.connections.getConnection(connectionId);
      if (iConn && iConn.direction === 'out' && !iConn.videoFrom && this.outputs[videoFrom]) {
        this.outputs[videoFrom].addDestination('video', iConn.connection.receiver());
        iConn.videoFrom = videoFrom;
        return Promise.resolve();
      }
    }

    return Promise.reject('linkup failed');
  }

  // override
  cutoff(connectionId) {
    log.debug('cutoff:', connectionId);
    var iConn = this.connections.getConnection(connectionId);
    var engine;
    if (!iConn && this.inputs[connectionId]) {
      connectionId = this.inputs[connectionId].videoFrom
      iConn = this.connections.getConnection(connectionId);
    } else if (!iConn && this.outputs[connectionId]) {
      this.connections.getIds().forEach((connId) => {
        iConn = this.connections.getConnection(connId);
        if (iConn.videoFrom === connectionId) {
          delete iConn.videoFrom;
          this.outputs[connectionId].removeDestination('video', iConn.connection.receiver());
        }
      });
      return Promise.resolve();
    }

    if (iConn) {
      // cutoff internal connection
      if (iConn.direction === 'in') {
        // cutoff inputs
        if (this.inputs[iConn.videoTo].output) {
          // remove generated output first
          let generateId = this.inputs[iConn.videoTo].output;
          engine = this.inputs[iConn.videoTo].engine;
          this.cutoff(generateId).catch((e) => log.error(e));
        }
        delete this.inputs[iConn.videoTo].videoFrom;
        delete iConn.videoTo;
        engine.unsetInput(connectionId);
        return Promise.resolve();
      } else {
        // cutoff outputs
        if (iConn.videoFrom && this.outputs[iConn.videoFrom]) {
          delete iConn.videoFrom;
          this.outputs[iConn.videoFrom].removeDestination('video', iConn.connection.receiver());
          return Promise.resolve();
        }
      }
    }
    return Promise.reject('cutoff failed');
  }

  cleanup() {
    log.debug('cleanup');
    var connectionId;
    var cuts = [];
    var output;
    // remove all the inputs, outputs will also be removed
    for (connectionId in this.inputs) {
      this.cutoff(connectionId).catch((e) => log.error(e));
      output = this.inputs[connectionId].output;
      this.inputs[connectionId].engine.removeOutput(output);
      this.inputs[connectionId].engine.close();
    }
    return Promise.resolve();
  }
}

module.exports = AnalyticsAgent;
