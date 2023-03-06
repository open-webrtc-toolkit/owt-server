// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('../logger').logger.getLogger('AnalyticsController');
const {TypeController} = require('./typeController');
const {Publication, Subscription, Processor} = require('../stateTypes')

function AnalyticsSession(config, direction) {
  const session = Object.assign({}, config);
  session.id = config.id;
  session.direction = direction;
  return session;
}

/* Events
 * 'session-established': (id, Publication|Subscription)
 * 'session-updated': (id, Publication|Subscription)
 * 'session-aborted': (id, reason)
 */
class AnalyticsController extends TypeController {
  constructor(rpc) {
    super(rpc);
    // Map {processorId => Processor}
    this.processors = new Map();
    this.sessions = new Map();
  }

  /*
   * AnalyticsConfig = {
   *   type: 'analytics',
   *   id: 'processorId',
   *   analytics: {id},
   *   mediaPreference: {audio: {encode:[], decode:[]}},
   *   domain: 'string',
   * }
   */
  async addProcessor(procConfig) {
    log.debug('addProcessor:', procConfig);
    // Unused media preference for analytics
    const mediaPreference = {audio: {encode:[], decode:[]}};
    const locality = await this.getWorkerNode(
      'audio', procConfig.domain, procConfig.id, mediaPreference);
    log.debug('locality:', locality);
    const analyzer = new Processor(procConfig.id, 'analyzer', procConfig);
    analyzer.locality = locality;
    analyzer.domain = procConfig.domain;
    // No init call for analytics worker
    this.processors.set(procConfig.id, analyzer);
    return analyzer;
  }

  async removeProcessor(id) {
    const processor = this.processors.get(id);
    if (processor) {
      const procConfig = this.processors.get(id).info;
      const locality = processor.locality;
      // Add clean up method for analytics agent

      // Remove related inputs & outputs
      processor.inputs.video.forEach((videoTrack) => {
        this.emit('session-aborted', videoTrack.id, 'Processor removed');
      });
      processor.outputs.video.forEach((videoTrack) => {
        this.emit('session-aborted', videoTrack.id, 'Processor removed');
      });
      this.processors.delete(id);
      // Recycle node
      this.recycleWorkerNode(locality, procConfig.domain, procConfig.id)
          .catch((err) => log.debug('Failed to recycleNode:', err));
    } else {
      return Promise.reject(new Error('Invalid processor ID'));
    }
  }

  /*
  publicationConfig = {
    id: publicationId,
    media: {video},
    processor: processorId,
    info: {
      owner: ownerId,
      origin: origin,
      attributes: attrObject,
    }
  }
   */
  /*
  subscriptionConfig = {
    id: publicationId,
    media: {video: {format, parameters}}
    connection: {
      algorithm: '',
      video: { // Output setting
        format: '',
        parameters: '',
      },
    },
    processor: processorId,
    info: {
      owner: ownerId,
      origin: origin,
      attributes: attrObject,
    }
  }
   */
  async createSession(direction, sessionConfig) {
    const processor = this.processors.get(sessionConfig.processor);
    if (!processor) {
      log.info('create session: invalid processor ID');
      return Promise.reject(new Error('Invalid processor ID'));
    }

    const session = AnalyticsSession(sessionConfig, direction);
    if (direction === 'in') {
      // Generate video stream for analyzer
      const format = sessionConfig.media?.video?.format;
      // Output ID saved when adding input
      const outputId = sessionConfig.id;
      this.sessions.set(outputId, session);
      // Create publication
      const publication = new Publication(outputId, 'analytics', sessionConfig);
      publication.domain = processor.domain;
      publication.locality = processor.locality;
      const videoTrack = Object.assign({id: outputId}, sessionConfig.media.video);
      publication.source.video.push(videoTrack);
      processor.outputs.video.push(videoTrack);
      process.nextTick(() => {
        this.emit('session-established', outputId, publication);
      });
      return outputId;
    } else {
      // Add input
      const inputId = sessionConfig.id;
      const options = {
        media: sessionConfig.media,
        connection: sessionConfig.connection,
        controller: this.selfId,
      };
      await this.makeRPC(processor.locality.node, 'subscribe',
          [inputId, 'analytics', options]);
      this.sessions.set(inputId, session);
      // Create subscription
      const subInfo = {
        processor: sessionConfig.processor,
      };
      const subscription = new Subscription(inputId, 'analytics', sessionConfig);
      subscription.domain = processor.domain;
      subscription.locality = processor.locality;
      const videoTrack = Object.assign({id: inputId}, sessionConfig.media.video);
      subscription.sink.video.push(videoTrack);
      processor.inputs.video.push(videoTrack);
      process.nextTick(() => {
        this.emit('session-established', inputId, subscription);
      });
      return inputId;
    }

  }

  async removeSession(id, direction, reason) {
    if (this.sessions.has(id)) {
      const session = this.sessions.get(id);
      const processor = this.processors.get(session.processor);
      if (!processor) {
        throw new Error(`Processor for ${id} not found`);
      }

      reason = reason || 'Participant terminate';
      if (direction === 'in') {
        // Degenerate
        const idx = processor.outputs.video.findIndex((track) => track.id === id);
        if (idx >= 0) {
          processor.outputs.audio.splice(idx, 1);
          this.emit('session-aborted', id, reason);
        }
      } else {
        // Remove input
        log.debug('session:', session);
        // Let cutoff do remove-input
        const inputId = session.media?.audio?.from;
        this.makeRPC(processor.locality.node, 'unsubscribe', [inputId])
          .catch((e) => log.debug('ignore unpublish callback'));
        const idx = processor.inputs.audio.findIndex((track) => track.id === id);
        if (idx >= 0) {
          processor.inputs.video.splice(idx, 1);
          this.emit('session-aborted', id, reason);
        }
      }
    } else {
      throw new Error(`Session does NOT exist:${id}`);
    }
  }

  onSessionProgress(id, type, data) {
    switch(type) {
      case 'onStreamAdded': {
        log.debug('onStreamAdded:', id, type, data);
        const session = this.sessions.get(id);
        const outputConfig = {
          id: data.id,
          media: {
            video: {
              format: data.media.video,
            }
          },
          processor: session.processor,
          info: session.info
        };
        this.createSession('in', outputConfig)
          .catch((e) => log.debug('Failed to create session on progress:', e));
        break;
      }
      case 'onStreamRemoved': {
        log.debug('onStreamRemoved:', id, type, data);
        this.removeSession(id, 'in', 'onStreamRemoved')
          .catch((e) => log.debug('Failed to remove session on progress:', e));
        break;
      }
      default:
        log.warn('Unknown progress type:', type);
        break;
    }
  }

  terminateByLocality(type, id) {
    log.debug(`terminateByLocality ${type} ${id}`);
    const terminations = [];
    this.processors.forEach((processor, processorId) => {
      const locality = processor.locality;
      if (((type === 'worker' && locality?.agent === id) ||
          (type === 'node' && locality?.node === id))) {
        const p = this.removeProcessor(processorId);
        terminations.push(p);
      }
    })
    return Promise.all(terminations).then(() => {
      //
    });
  };

  destroy() {
    log.debug(`destroy`);
    // Destroy all sessions
    const terminations = [];
    this.processors.forEach((processor, processorId) => {
      const p = this.removeProcessor(processorId);
      terminations.push(p);
    })
    return Promise.all(terminations).then(() => {
      //
    });
  }
}

exports.AnalyticsController = AnalyticsController;
