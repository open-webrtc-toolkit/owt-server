// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('../logger').logger.getLogger('StreamingController');
const {TypeController} = require('./typeController');
const {Publication, Subscription} = require('../stateTypes')

function StreamingSession(config, direction) {
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
class StreamingController extends TypeController {
  constructor(rpc) {
    super(rpc);
    // Map {sessionId => StreamingSession}
    /*
    StreamingSession = {
      id: publicationId | subscriptionId,
      media: sessionConfig.media,
      connection: sessionConfig.connection,
      info: sessionConfig.info,
    }
    */
    this.sessions = new Map();
  }

  //===== starts ======
  /*
  publicationConfig = {
    id: publicationId,
    media: {audio, video},
    connection: {
      url,
      transportProtocol,
      bufferSize,
    },
    domain: roomId,
    info: {
      owner: ownerId,
      origin: origin,
      attributes: attrObject,
    }
  }
   */
  /*
  subscriptionConfig = {
    id: subscriptionId,
    protocol: string,
    url: string,
    media: {audio, video},
    parameters: {},
    domain: roomId,
    info: {
      owner: ownerId,
      origin: origin,
      attributes: attrObject,
    }
  }
   */
  async createSession(direction, config) {
    const id = config.id;
    const session = StreamingSession(config, direction);
    if (!this.sessions.has(id)) {
      this.sessions.set(id, session);
      const origin = config.info?.origin || {};
      const locality = await this.getWorkerNode(
          config.type, config.domain, id, origin);
      this.sessions.get(id).locality = locality;
      this.sessions.get(id).state = 'initiating';
      const options = config;
      if (direction === 'in') {
        if (options.media.audio === undefined) {
          options.media.audio = 'auto';
        }
        if (options.media.video === undefined) {
          options.media.video = 'auto';
        }
      }
      options.controller = this.selfId;
      const rpcNode = locality.node;
      const method = direction === 'in' ? 'publish' : 'subscribe';
      return this.makeRPC(rpcNode, method, [id, config.type, options])
        .then(() => id);
    } else {
      throw new Error('Session ID already exists');
    }
  }

  async removeSession(id, direction, reason) {
    if (this.sessions.has(id)) {
      if (this.sessions.get(id).locality) {
        const locality = this.sessions.get(id).locality;
        const domain = this.sessions.get(id).domain;
        reason = reason || 'Participant terminate';
        const method = direction === 'in' ? 'unpublish' : 'unsubscribe';
        try {
          await this.makeRPC(locality.node, method, [id]);
          log.debug(`to recycleWorkerNode: ${locality} task:, ${id}`);
          await this.recycleWorkerNode(locality, domain, id);
        } catch (e) {
          log.debug(`Failed to terminate session: ${id}`);
        }
      } else {
        throw new Error(`Session does NOT initiated:${id}`);
      }
    } else {
      throw new Error(`Session does NOT exist:${id}`);
    }
  }

  onSessionProgress(id, direction, data) {
    log.debug('onSessionProgress', id, direction, data);
    if (!this.sessions.has(id)) {
      log.warn(`Cannot find ${id} for progress ${direction}, ${data}`);
      return;
    }
    const type = this.sessions.get(id).type;
    if (data.type === 'ready') {
      if (direction === 'in') {
        // Parse media format
        const publication = new Publication(id, type, {});
        publication.domain = this.sessions.get(id).domain;
        publication.locality = this.sessions.get(id).locality;
        if (data.audio) {
          const audioTrack = {id, format: data.audio};
          publication.source.audio.push(audioTrack);
        }
        if (data.video) {
          const videoTrack = {id, format: data.video};
          publication.source.video.push(videoTrack);
        }
        log.debug('a:', data.audio, 'v:', data.video);
        this.emit('session-established', id, publication);
      } else {
        const subscription = new Subscription(id, type, {});
        subscription.domain = this.sessions.get(id).domain;
        subscription.locality = this.sessions.get(id).locality;
        const session = this.sessions.get(id);
        if (session.media.audio) {
          const audioTrack = session.media.audio;
          audioTrack.id = id;
          subscription.sink.audio.push(audioTrack);
        }
        if (session.media.video) {
          const videoTrack = session.media.video;
          videoTrack.id = id;
          subscription.sink.video.push(videoTrack);
        }
        this.emit('session-established', id, subscription);
      }
    } else if (data.type === 'failed') {
      this.emit('session-aborted', id, (data.reason || 'unknown'));
    }
  }

  terminateByLocality(type, id) {
    log.debug(`terminateByLocality ${type} ${id}`);
    const terminations = [];
    this.sessions.forEach((session, sessionId) => {
      const l = session.locality;
      if (l && ((type === 'worker' && l.agent === id) ||
          (type === 'node' && l.node === id))) {
        const p = this.removeSession(sessionId, session.direction, 'Node lost');
        terminations.push(p);
      }
    });
    return Promise.all(terminations).then(() => {
      //
    });
  };

  destroy() {
    log.debug(`destroy`);
    // Destroy all sessions
    const terminations = [];
    this.sessions.forEach((session, sessionId) => {
      const p = this.removeSession(sessionId, session.direction, 'Controller destroy');
      terminations.push(p);
    });
    return Promise.all(terminations).then(() => {
      //
    });
  };
}

exports.StreamingController = StreamingController;
