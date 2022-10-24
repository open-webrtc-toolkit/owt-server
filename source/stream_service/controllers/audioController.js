// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('../logger').logger.getLogger('AudioController');
const {TypeController} = require('./typeController');
const {Publication, Subscription, Processor} = require('../stateTypes')

function AudioSession(config, direction) {
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
class AudioController extends TypeController {

  constructor(rpc) {
    super(rpc);
    // Map {processorId => Processor}
    this.processors = new Map();
    this.sessions = new Map();
  }

  /*
   * AudioConfig = {
   *   type: 'audio',
   *   id: 'processorId',
   *   mixing: {id, format, vad}, // See room view audio config
   *   transcoding: {id},
   *   selecting: {id, activeStreamIds: []},
   *   mediaPreference: {audio: {encode:[], decode:[]}},
   *   domain: 'string',
   * }
   */
  async addProcessor(audioConfig) {
    // Audio preference never used
    const mediaPreference = audioConfig.mediaPreference || {
      audio: {encode:[], decode:[]},
      origin: '',
    };
    const locality = await this.getWorkerNode(
      'audio', audioConfig.domain, audioConfig.id, mediaPreference);
    if (audioConfig.mixing) {
      const amixer = new Processor(audioConfig.id, 'amixer', audioConfig);
      amixer.locality = locality;
      amixer.domain = audioConfig.domain;
      const mixConfig = audioConfig.mixing;
      try {
        await this.makeRPC(locality.node, 'init',
            ['mixing', mixConfig, mixConfig.id, this.selfId, mixConfig.view]);
      } catch (e) {
        this.recycleWorkerNode(locality, audioConfig.domain, audioConfig.id)
            .catch((err) => log.debug('Failed to recycleNode:', err));
        throw e;
      }
      this.processors.set(audioConfig.id, amixer);
      return amixer;
    } else if (audioConfig.transcoding) {
      const atranscoder = new Processor(audioConfig.id, 'axcoder', audioConfig);
      atranscoder.locality = locality;
      atranscoder.domain = audioConfig.domain;
      const transcodeConfig = audioConfig.transcoding;
      try {
        await this.makeRPC(locality.node, 'init',
          ['transcoding', transcodeConfig, transcodeConfig.id, this.selfId, '']);
      } catch (e) {
        this.recycleWorkerNode(locality, audioConfig.domain, audioConfig.id)
            .catch((err) => log.debug('Failed to recycleNode:', err));
        throw e;
      }
      this.processors.set(audioConfig.id, atranscoder);
      return atranscoder;
    } else if (audioConfig.selecting) {
      const aselector = new Processor(audioConfig.id, 'aselector', audioConfig);
      aselector.locality = locality;
      aselector.domain = audioConfig.domain;
      const selectorConfig = audioConfig.selecting;
      try {
        await this.makeRPC(locality.node, 'init',
          ['selecting', selectorConfig, selectorConfig.id, this.selfId, '']);
        // Create publication for active audio streams after return
        process.nextTick(() => {
          for (const streamId of selectorConfig.activeStreamIds) {
            const publication = new Publication(output.id, 'audio', sessionConfig);
            publication.domain = audioConfig.domain;
            publication.locality = locality;
            const audioTrack = {id: streamId, format: {codec: 'opus'}};
            publication.source.audio.push(audioTrack);
            aselector.outputs.audio.push(audioTrack);
            this.emit('session-established', streamId, publication);
          }
        });
      } catch (e) {
        this.recycleWorkerNode(locality, audioConfig.domain, audioConfig.id)
            .catch((err) => log.debug('Failed to recycleNode:', err));
        throw e;
      }
      this.processors.set(audioConfig.id, aselector);
      return aselector;
    }
  }

  async removeProcessor(id) {
    const processor = this.processors.get(id);
    if (processor) {
      const audioConfig = this.processors.get(id).info;
      const taskId = (audioConfig.mixing || audioConfig.transcoding
          || audioConfig.selecting).id;
      const locality = processor.locality;
      // Deinit does not have callback
      this.makeRPC(locality.node, 'deinit', [taskId])
        .catch((e) => log.debug('Failed to deinit:', taskId, e));
      // Remove related inputs & outputs
      processor.inputs.audio.forEach((audioTrack) => {
        this.emit('session-aborted', audioTrack.id, 'Processor removed');
      });
      processor.outputs.audio.forEach((audioTrack) => {
        this.emit('session-aborted', audioTrack.id, 'Processor removed');
      });
      this.processors.delete(id);
      // Recycle node
      this.recycleWorkerNode(locality, audioConfig.domain, audioConfig.id)
          .catch((err) => log.debug('Failed to recycleNode:', err));
    } else {
      return Promise.reject(new Error('Invalid processor ID'));
    }
  }

  /*
  publicationConfig = {
    id: publicationId,
    media: {audio},
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
    media: {audio},
    participant: participantId,
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
    if (!processor || processor.type === 'aselector') {
      log.info('create session: invalid processor ID');
      return Promise.reject(new Error('Invalid processor ID'));
    }

    const session = AudioSession(sessionConfig, direction);
    if (direction === 'in') {
      // Generate audio stream for mixer/transcoder
      const format = sessionConfig.media?.audio?.format;
      const participant = (processor.type === 'atranscoder') ?
          fomrat.codec : sessionConfig.participant;
      // output = "streamId"
      const outputId = await this.makeRPC(processor.locality.node, 'generate',
          [participant, format.codec]);
      sessionConfig.id = outputId;
      this.sessions.set(outputId, session);
      // Create publication
      const publication = new Publication(outputId, 'audio', sessionConfig);
      publication.domain = processor.domain;
      publication.locality = processor.locality;
      const audioTrack = Object.assign({id: outputId}, sessionConfig.media.audio);
      publication.source.audio.push(audioTrack);
      processor.outputs.audio.push(audioTrack);
      process.nextTick(() => {
        this.emit('session-established', outputId, publication);
      });
      return outputId;
    } else {
      // Add input
      const inputId = sessionConfig.id;
      const inputConfig = {
        controller: this.selfId,
        publisher: sessionConfig.info?.owner || 'common',
        audio: {codec: sessionConfig.media?.audio?.format?.codec},
      };
      await this.makeRPC(processor.locality.node, 'publish',
          [inputId, 'internal', inputConfig]);
      this.sessions.set(inputId, session);
      // Create subscription
      const subInfo = {
        processor: sessionConfig.processor,
      };
      const subscription = new Subscription(inputId, 'audio', sessionConfig);
      subscription.domain = processor.domain;
      subscription.locality = processor.locality;
      const audioTrack = Object.assign({id: inputId}, sessionConfig.media.audio);
      subscription.sink.audio.push(audioTrack);
      processor.inputs.audio.push(audioTrack);
      this.emit('session-established', inputId, subscription);
      return inputId;
    }

  }

  async removeSession(id, direction, reason) {
    reason = reason || 'Participant terminate';
    if (this.sessions.has(id)) {
      const session = this.sessions.get(id);
      const processor = this.processors.get(session.processor);
      if (!processor) {
        throw new Error(`Processor for ${id} not found`);
      }
      if (session.direction === 'in') {
        // Degenerate
        this.makeRPC(processor.locality.node, 'degenerate', [id])
          .catch((e) => log.debug('ignore degenerate callback'));
        const idx = processor.outputs.audio.findIndex((track) => track.id === id);
        if (idx >= 0) {
          processor.outputs.audio.splice(idx, 1);
          this.emit('session-aborted', id, reason);
        }
      } else {
        // Remove input
        log.debug('Remove session:', session);
        // Let cutoff do remove-input
        const inputId = session.media?.audio?.from;
        this.makeRPC(processor.locality.node, 'unpublish', [inputId])
          .catch((e) => log.debug('ignore unpublish callback'));
        const idx = processor.inputs.audio.findIndex((track) => track.id === id);
        if (idx >= 0) {
          processor.inputs.audio.splice(idx, 1);
          this.emit('session-aborted', id, reason);
        }
      }
    } else {
      throw new Error(`Session does NOT exist:${id}`);
    }
  }

  async terminateByLocality(type, id) {
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
    });
  }
}

exports.AudioController = AudioController;
