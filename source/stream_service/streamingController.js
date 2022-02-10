// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const { EventEmitter } = require('events');
const log = require('./logger').logger.getLogger('StreamingController');
const {Publication, Subscription} = require('./session');

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
class StreamingController extends EventEmitter {

  /* rpcReq {
   *   getWorkerNode, recycleWorkerNode, initiate, terminate,
   *   onTransportSignaling, mediaOnOff, sendMsg
   * }
   */
  constructor(rpcReq, selfRpcId, clusterRpcId) {
    log.debug(`constructor ${selfRpcId}, ${clusterRpcId}`);
    super();
    this.selfRpcId = selfRpcId;
    this.rpcReq = rpcReq;
    this.clusterRpcId = clusterRpcId;
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
  async createSession(direction, sessionConfig) {
    const id = sessionConfig.id;
    const session = StreamingSession(sessionConfig, direction);
    if (!this.sessions.has(id)) {
      this.sessions.set(id, session);
      const taskConfig = {room: session.domain, task: id};
      const origin = {};
      log.debug(`getWorkerNode ${this.clusterRpcId}, ${taskConfig}, ${origin}`);
      const locality = await this.rpcReq.getWorkerNode(
          this.clusterRpcId, sessionConfig.type, taskConfig, origin);
      this.sessions.get(id).locality = locality;
      this.sessions.get(id).state = 'initiating';
      const options = sessionConfig;
      if (direction === 'in') {
        if (options.media.audio === undefined) {
          options.media.audio = 'auto';
        }
        if (options.media.video === undefined) {
          options.media.video = 'auto';
        }
      }
      options.controller = this.selfRpcId;
      return this.rpcReq.initiate(locality.node, id, sessionConfig.type, direction, options)
        .then(() => id);
    } else {
      return Promise.reject(new Error('Session ID already exists'));
    }
  }

  async removeSession(id, direction, reason) {
    if (this.sessions.has(id)) {
      if (this.sessions.get(id).locality) {
        const locality = this.sessions.get(id).locality;
        try {
          await this.rpcReq.terminate(locality.node, id, direction);
        } catch (e) {
          log.debug(e, e.stack);
          log.debug(`Session terminate failed ${id}, ${reason}, ${e.stack}`);
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
        log.debug('a', data.audio, 'v', data.video);
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
