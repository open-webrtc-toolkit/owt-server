// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('../logger').logger.getLogger('VideoController');
const {TypeController} = require('./typeController');
const {Publication, Subscription, Processor} = require('../stateTypes')

function VideoSession(config, direction) {
  const session = Object.assign({}, config);
  session.id = config.id;
  session.direction = direction;
  return session;
}

function videoFormatStr(format) {
  if (!format || !format.codec) {
    return null;
  }
  let str = format.codec;
  if (format.profile) {
    str += '_' + format.profile;
  }
  return str;
}

/* Events
 * 'session-established': (id, Publication|Subscription)
 * 'session-updated': (id, Publication|Subscription)
 * 'session-aborted': (id, reason)
 */
class VideoController extends TypeController {
  constructor(rpc) {
    super(rpc);
    // Map {processorId => Processor}
    this.processors = new Map();
    this.sessions = new Map();
  }

  /*
   * videoConfig = {
   *   type: 'video',
   *   id: 'processorId',
   *   mixing: {id, view}, // See room view video config
   *   transcoding: {id},
   *   mediaPreference: {video: {encode:[], decode:[]}},
   *   domain: 'string',
   * }
   */
  async addProcessor(videoConfig) {
    const mediaPreference = videoConfig.mediaPreference || {
      video: {
        encode: ['vp8', 'vp9', 'h264_CB', 'h264_B', 'h265'],
        decode: ['vp8', 'vp9', 'h264', 'h265'],
      },
      origin: '',
    };
    const locality = await this.getWorkerNode(
        'video', videoConfig.domain, videoConfig.id, mediaPreference);
    if (videoConfig.mixing) {
      const vmixer = new Processor(videoConfig.id, 'vmixer', videoConfig);
      vmixer.locality = locality;
      vmixer.domain = videoConfig.domain;
      const mixConfig = videoConfig.mixing;
      try {
        await this.makeRPC(locality.node, 'init',
            ['mixing', mixConfig, mixConfig.id, this.selfId, mixConfig.view]);
      } catch (e) {
        this.recycleWorkerNode(locality, videoConfig.domain, videoConfig.id)
            .catch((err) => log.debug('Failed to recycleNode:', err));
        throw e;
      }
      this.processors.set(videoConfig.id, vmixer);
      return vmixer;

    } else if (videoConfig.transcoding) {
      const vtranscoder = new Processor(videoConfig.id, 'vxcoder', videoConfig);
      vtranscoder.locality = locality;
      vtranscoder.domain = videoConfig.domain;
      const transcodeConfig = videoConfig.transcoding;
      try {
        await this.makeRPC(locality.node, 'init',
          ['transcoding', transcodeConfig, transcodeConfig.id, this.selfId, ''])
      } catch (e) {
        this.recycleWorkerNode(locality, videoConfig.domain, videoConfig.id)
            .catch((err) => log.debug('Failed to recycleNode:', err));
        throw e;
      }
      this.processors.set(videoConfig.id, vtranscoder);
      return vtranscoder;
    }
  }

  async removeProcessor(id) {
    const processor = this.processors.get(id);
    if (processor) {
      const videoConfig = this.processors.get(id).info;
      const taskId = (videoConfig.mixing || videoConfig.transcoding).id;
      const locality = processor.locality;
      // Deinit does not have callback
      this.makeRPC(locality.node, 'deinit', [taskId])
        .catch((e) => log.debug('Failed to deinit:', taskId, e));

      // Remove related inputs & outputs
      processor.inputs.video.forEach((videoTrack) => {
        this.emit('session-aborted', videoTrack.id, 'Processor removed');
      });
      processor.outputs.video.forEach((videoTrack) => {
        this.emit('session-aborted', videoTrack.id, 'Processor removed');
      });
      this.processors.delete(id);
      // Recycle node
      this.recycleWorkerNode(locality, videoConfig.domain, videoConfig.id)
          .catch((err) => log.debug('Failed to recycleNode:', err));
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
    media: {video},
    processor: processorId,
    info: {
      owner: ownerId,
      origin: origin,
      attributes: attrObject,
    }
  }
   */
  async createSession(direction, sessionConfig) {
    log.debug('Create session:', direction, sessionConfig);
    const processor = this.processors.get(sessionConfig.processor);
    if (!processor) {
      log.info('create session: invalid processor ID');
      return Promise.reject(new Error('Invalid processor ID'));
    }

    const session = VideoSession(sessionConfig, direction);
    if (direction === 'in') {
      // Generate video stream for mixer/transcoder
      const setting = sessionConfig.media?.video;
      const format = setting.format || 'unspecified';
      const {
        resolution = 'unspecified',
        framerate = 'unspecified',
        bitrate = 'unspecified',
        keyFrameInterval = 'unspecified'
      } = (setting.parameters || {});
      // output = {id, resolution, framerate, bitrate, keyFrameInterval}
      const output = await this.makeRPC(processor.locality.node, 'generate',
          [videoFormatStr(format), resolution, framerate, bitrate, keyFrameInterval]);
      sessionConfig.id = output.id;
      this.sessions.set(output.id, session);
      // Create publication
      const publication = new Publication(output.id, 'video', sessionConfig.info);
      publication.domain =  processor.domain;
      publication.locality = processor.locality;
      publication.participant = sessionConfig.participant;
      const videoTrack = Object.assign({id: output.id}, sessionConfig.media.video);
      publication.source.video.push(videoTrack);
      processor.outputs.video.push(videoTrack);
      const ret = Promise.resolve(output.id);
      ret.then(() => {
        process.nextTick(() => {
          this.emit('session-established', output.id, publication);
        });
      });
      return ret;
    } else {
      // Add input
      const inputId = sessionConfig.id;
      const inputConfig = {
        controller: this.selfId,
        publisher: sessionConfig.info?.owner || 'common',
        video: {
          codec: videoFormatStr(sessionConfig.media?.video?.format)
        },
      };
      await this.makeRPC(processor.locality.node, 'publish',
          [inputId, 'internal', inputConfig]);
      this.sessions.set(inputId, session);
      // Create subscription
      const subscription = new Subscription(inputId, 'video', sessionConfig.info);
      subscription.domain = processor.domain;
      subscription.locality = processor.locality;
      subscription.participant = processor.participant;
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
      const rpcChannel = this.rpcChannel;
      const session = this.sessions.get(id);
      const processor = this.processors.get(session.processor);
      if (!processor) {
        throw new Error(`Processor for ${id} not found`);
      }

      if (session.direction === 'in') {
        // Degenerate
        rpcChannel.makeRPC(processor.locality.node, 'degenerate', [id])
          .catch((e) => log.debug('degenerate:', e));

        const idx = processor.outputs.video.findIndex((track) => track.id === id);
        if (idx >= 0) {
          processor.outputs.video.splice(idx, 1);
          this.emit('session-aborted', id, 'Participant terminate');
        }
      } else {
        // Remove input
        log.debug('session:', session);
        // Let cutoff do remove-input
        const inputId = session.media?.video?.from;
        rpcChannel.makeRPC(processor.locality.node, 'unpublish', [inputId])
          .catch((e) => log.debug('ignore unpublish callback'));
        const idx = processor.inputs.video.findIndex((track) => track.id === id);
        if (idx >= 0) {
          processor.inputs.video.splice(idx, 1);
          this.emit('session-aborted', id, 'Participant terminate');
        }
      }
    } else {
      throw new Error(`Session does NOT exist:${id}`);
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

exports.VideoController = VideoController;
