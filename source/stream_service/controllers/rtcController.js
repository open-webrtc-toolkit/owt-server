// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('../logger').logger.getLogger('RtcController');
const {TypeController} = require('./typeController');
const {Publication, Subscription} = require('../stateTypes')

// Transport/Operation state
const INITIALIZING = 'initializing';
const COMPLETED = 'completed';

class Transport {
  constructor(id, owner, origin, locality) {
    this.id = id;
    this.owner = owner;
    this.origin = origin;
    this.locality = locality;
    this.state = INITIALIZING;
  }
}

class Track {
  constructor(operation, {type, source, mid, formatPreference, from}) {
    this.type = type;
    this.source = source;
    this.mid = mid;
    this.formatPreference = formatPreference;
    this.from = from;
    this.operationId = operation.id;
    this.direction = operation.direction;
    this.id = null;
    this.format = null;
    this.rid = null;
    this.active = false;
    this.terminated = false;
  }
  update(id, format, rid, active) {
    this.id = id;
    this.format = format;
    this.rid = rid;
    this.active = active;
  }
  clone() {
    const op = {
      id: this.operationId,
      direction: this.direction
    };
    return new Track(op, this);
  }
}

class Operation {
  constructor(id, transport, direction, tracks, legacy, attributes) {
    this.id = id;
    this.transport = transport;
    this.transportId = transport.id;
    this.direction = direction;
    this.tracks = tracks.map(t => new Track(this, t));
    this.legacy = legacy;
    this.attributes = attributes;
    this.promise = Promise.resolve();
  }
  updateTrack({trackId, mediaType, mediaFormat, direction, mid, rid, active}) {
    const opTrack = this.tracks.find(t => t.mid === mid);
    if (opTrack) {
      if (!opTrack.id) {
        opTrack.update(trackId, mediaFormat, rid, active);
      } else {
        // Simulcast
        const simTrack = opTrack.clone();
        simTrack.update(trackId, mediaFormat, rid, active);
        this.tracks.push(simTrack);
        return simTrack;
      }
    } else {
      log.error(`Session ${this.id} invalid track: ${trackId} mid ${mid}`);
    }
    return opTrack;
  }
  toSessionFormat() {
    if (Publication && Subscription) {
      const info = this.info;
      if (this.direction === 'in') {
        const publication = new Publication(this.id, 'webrtc', info);
        publication.locality = this.transport.locality;
        this.tracks.forEach((track) => {
          publication.source[track.type].push(track);
        });
        publication.domain = this.transport.domain;
        return publication;
      } else {
        const subscription = new Subscription(this.id, 'webrtc', info);
        subscription.locality = this.transport.locality;
        this.tracks.forEach((track) => {
          subscription.sink[track.type].push(track);
        });
        subscription.domain = this.transport.domain;
        return subscription;
      }
    }
    return null;
  }
}

/*
 * Events for stream engine
 * 'signaling': (owner, name, status)
 * 'session-established': (id, Publication)
 * 'session-updated': (id, Publication)
 * 'session-aborted': (id, {owner, direction, reason})
 */
class RtcController extends TypeController {
  constructor(rpc) {
    super(rpc);
    this.transports = new Map(); //Map{transportId => Transport}
    this.operations = new Map(); //Map{operationId => Operation}
    this.tracks = new Map(); //Map{publicTrackId => Track}
  }
  /*
  sessionConfig = {
    id: id,
    transport: {id},
    media: {
      tracks: [initTrack]
    },
    domain: domain,
    participant: pariticipantId,
    info: {
      owner: ownerId,
      origin: origin,
      attributes: attrObject,
      formatPreference: {audio, video}
    },
  }
   */
  async createSession(direction, config) {
    log.debug(`createSession, ${direction} ${config}`);
    const sessionId = config.id;
    const transportId = config.transport?.id || config.id;
    const owner = config.participant;
    await this._createTransportIfNeeded(
      owner, config.domain, transportId, config.info.origin);
    const transport = this.transports.get(transportId);
    if (!transport) {
      throw new Error(`Transport ${transportId} is not ready`);
    }
    // Add operation (publish/subscribe)
    if (this.operations.has(sessionId)) {
      log.debug(`operation exists:${sessionId}`);
      throw new Error(`operation exists:${sessionId}`);
    }
    const option = {
      transportId,
      tracks: config.media.tracks,
      attributes: config.attributes,
      controller: this.selfId,
      owner
    };
    option.tracks.forEach((track) => {
      track.formatPreference = config.info.formatPreference[track.type];
    });
    const op = new Operation(
      sessionId, transport, direction, option.tracks, false, option.attributes);
    op.info = config.info;
    this.operations.set(sessionId, op);
    // Make RPC call
    const rpcNode = transport.locality.node;
    const method = direction === 'in' ? 'publish' : 'subscribe';
    await this.makeRPC(rpcNode, method, [sessionId, 'webrtc', option]);
    return sessionId;
  }
  /*
   * sessionId: string
   * direction: 'in'|'out'
   * reason: string
   */
  async removeSession(sessionId, direction, reason) {
    log.debug(`removeSession: ${sessionId} ${direction} ${reason}`);
    // Load state
    if (!this.operations.has(sessionId)) {
      log.debug(`operation does NOT exist:${sessionId}`);
      throw new Error(`operation does NOT exist:${sessionId}`);
    }
    const operation = this.operations.get(sessionId);
    const transport = this.transports.get(operation.transportId);
    const accessNode = transport.locality.node;
    reason = reason || 'Participant terminate';
    const method = direction === 'in' ? 'unpublish' : 'unsubscribe';
    try {
      await this.makeRPC(accessNode, method, [sessionId])
    } catch (e) {
      log.debug(`Failed to terminate session: ${sessionId}`);
    }
    // After RPC
    if (this.operations.has(sessionId)) {
      const owner = transport.owner;
      const abortData = { direction: operation.direction, owner, reason };
      this.emit('session-aborted', sessionId, abortData);
      this.emit('signaling', owner, 'progress',
          {id: operation.transportId, sessionId, status: 'error', data: reason});
      this.operations.delete(sessionId);
    }
  }
  /*
  sessionConfig = {
    id: id,
    operation: op,
    data: 'audio/video'
  }
   */
  async controlSession(direction, config) {
    log.debug('controlSession:', direction, config);
    if (!config.operation) {
      log.debug('No operation');
      return;
    }
    const id = config.id;
    if (['pause', 'play'].includes(config.operation)) {
      const data = config.data;
      const setAudio = data.includes('a');
      const setVideo = data.includes('v');
      let tracks = [];
      if (this.operations.has(id)) {
        tracks = this.operations.get(id).tracks
          .filter((track) => {
            return track.id && ((track.type === 'audio' && setAudio) ||
                (track.type === 'video' && setVideo));
          })
          .map((track) => track.id);
      }
      const muted = (config.operation === 'pause') ? true : false;
      if (tracks.length > 0) {
        return this.setMute(id, tracks, muted);
      } else {
        throw new Error(`Cannot find track for mute/unmute: ${id}`);
      }
    } else if (config.operation === 'update') {
      super.controlSession(direction, config);
    } else {
      throw new Error(`Operation not supported: ${config.operation}`);
    }
  }
  // Destroy RTC controller
  destroy() {
    log.debug(`destroy`);
    // Destroy all transports
    this.transports.forEach((transport, transportId) => {
      const status = {type: 'failed', reason: 'Owner leave'};
        this.onTransportProgress(transportId, status);
    });
  };
  // RPC API as progress listener
  progressListeners() {
    return {
      onTransportProgress: this.onTransportProgress.bind(this),
      onTrackUpdate: this.onTrackUpdate.bind(this),
      onMediaUpdate: this.onMediaUpdate.bind(this),
    };
  }

  onMediaUpdate(id, direction, data) {
    log.debug('onMediaUpdate:', id, direction, data);
    const track = this.tracks.get(id);
    const op = this.operations.get(track?.operationId);
    if (op) {
      track.parameters = track.parameters || {};
      track.parameters.resolution = data.video?.parameters?.resolution;
      this.emit('session-updated', op.id,
        {type: 'update', data: op.toSessionFormat()});
    }
  }

  onSessionProgress(id, type, data) {
    log.warn('onSessionProgress', id, type, data);
    switch(type) {
      case 'onTransportProgress': {
        this.onTransportProgress(id, data);
        break;
      }
      case 'onTrackUpdate': {
        this.onTrackUpdate(id, data);
        break;
      }
      case 'onMediaUpdate': {
        log.debug('onMediaUpdate:', id, type, data);
        break;
      }
      default:
        break;
    }
  }

  onTransportProgress(transportId, status) {
    const transport = this.transports.get(transportId);
    if (!transport) {
      log.error(`onTransportProgress: Transport ${transportId} does NOT exist`);
      return;
    }
    if (status.type === 'ready') {
      transport.state = COMPLETED;
      this.emit('transport-established', transportId);
      this.emit('signaling', transport.owner, 'progress',
          {id: transportId, status: 'ready'});
      // Emit pending completed tracks
      for (const [id, operation] of this.operations) {
        if (operation.transportId === transportId &&
            operation.state === COMPLETED) {
          this.emit('session-established',
              operation.id, operation.toSessionFormat());
          this.emit('signaling', transport.owner, 'progress',
              {id: transportId, sessionId: operation.id, status: 'ready'});
        }
      }
    } else if (status.type === 'failed') {
      this.emit('transport-aborted', transportId, status.reason);
      // Destroy transport
      if (transport.locality) {
        log.debug(`Destroy transport ${transportId}`);
        this.makeRPC(transport.locality.node, 'destroyTransport', [transportId])
        .catch((e) => {
          log.debug(`Faild to clean up transport ${transportId}: ${e}`);
        }).then(() => {
          const locality = transport.locality;
          const domain = transport.origin.domain;
          log.debug(`to recycleWorkerNode: ${locality} task:, ${transportId}`);
          return this.recycleWorkerNode(locality, domain, transportId);
        }).catch((e) => log.debug(`Failed to recycleWorkerNode ${locality}`));
      } else {
        log.warn(`No locality for failed transport ${transportId}`);
      }
    } else if (status.type === 'offer' ||
        status.type === 'answer' ||
        status.type === 'candidate') {
      this.emit('transport-signaling', transport.owner, transportId, status);
      this.emit('signaling', transport.owner, 'progress',
          {id: transportId, status: 'soac', data: status});
    } else {
      log.error(`Irrispective status: ${JSON.stringify(status)}`);
    }

  }

  onClientTransportSignaling(transportId, signaling) {
    log.debug(`onClientTransportSignaling ${transportId}, ${JSON.stringify(signaling)}`);
    if (!this.transports.has(transportId)) {
      return Promise.reject(`Transport ${transportId} does NOT exist`);
    }
    const locality = this.transports.get(transportId).locality;
    if (!locality) {
      return Promise.reject(`Transport ${transportId} locality is NOT ready`);
    }
    return this.makeRPC(locality.node, 'onTransportSignaling', [transportId, signaling])
      .catch(e => log.warn(`Trnasport signaling RPC failed ${e}`));
  };

  onClientSessionSignaling(sessionId, signaling) {
    const operation = this.operations.get(sessionId);
    if (operation) {
      return this.onClientTransportSignaling(operation.transportId, signaling);
    } else {
      return Promise.reject(`Session ${sessionId} does NOT exist`);
    }
  };

  onTrackUpdate(transportId, info) {
    log.debug(`onTrackUpdate ${transportId}, ${JSON.stringify(info)}`);
    if (info.type === 'track-added') {
      // {type, trackId, mediaType, mediaFormat, direction, operationId, mid, rid, active}
      if (this.tracks.has(info.trackId)) {
        log.error(`onTrackUpdate: duplicate track ID ${info.trackId}`);
      } else {
        if (this.operations.has(info.operationId)) {
          const operation = this.operations.get(info.operationId);
          const newTrack = operation.updateTrack(info);
          if (newTrack) {
            this.tracks.set(info.trackId, newTrack);
          }
          if (operation.state === COMPLETED) {
            log.warn('Unexpected order for track-added event');
            this.emit('session-updated', info.operationId, {type: 'add', data: info});
          }
        } else {
          log.warn(`Operation ${info.operationId} deleted for track-added`);
        }
      }
    } else if (info.type === 'track-removed') {
      if (!this.tracks.has(info.trackId)) {
        log.error(`onTrackUpdate: non-exist track ID ${info.trackId}`);
      } else {
        const operationId = this.tracks.get(info.trackId).operationId;
        const operation = this.operations.get(operationId);
        // if (operation.state === COMPLETED) {
        //   // TODO: close the session more accurate
        //   operation.state = CLOSED;
        //   const owner = this.transports.get(operation.transportId).owner;
        //   const abortData = { direction: operation.direction, owner };
        //   this.emit('session-aborted', operationId, abortData);
        // }
        this.tracks.delete(info.trackId);
      }
    } else if (info.type === 'tracks-complete') {
      const operation = this.operations.get(info.operationId);
      if (operation) {
        operation.state = COMPLETED;
        if (operation.transport.state === COMPLETED) {
          // Only emit when transport is completed
          this.emit('session-established', operation.id, operation.toSessionFormat());
          this.emit('signaling', operation.transport.owner, 'progress',
              {id: operation.transportId, sessionId: operation.id, status: 'ready'});
        }
      }
    }
  }

  async _createTransportIfNeeded(ownerId, domain, tId, origin) {
    if (!this.transports.has(tId)) {
      const locality = await this.getWorkerNode('webrtc', domain, tId, origin);
      this.transports.set(tId, new Transport(tId, ownerId, origin, locality));
      this.transports.get(tId).domain = domain;
    }
    return this.transports.get(tId);
  }

  async terminateByOwner(ownerId) {
    log.debug(`terminateByOwner ${ownerId}`);
    const terminations = [];
    this.operations.forEach((operation, operationId) => {
      if (operation.transport.owner === ownerId) {
        const p = this.removeSession(operationId, operation.direction, 'Owner leave');
        terminations.push(p);
      }
    });
    const transports = new Set();
    this.transports.forEach((transport, transportId) => {
      if (transport.owner === ownerId) {
        transports.add(transport.id);
      }
    });

    return Promise.all(terminations).then(() => {
      transports.forEach((transportId) => {
        const status = {type: 'failed', reason: 'Owner leave'};
        this.onTransportProgress(transportId, status);
      });
    });
  };

  async terminateByLocality(type, id) {
    log.debug(`terminateByLocality ${type} ${id}`);
    const terminations = [];
    const transports = new Set();
    this.operations.forEach((operation, operationId) => {
      const l = operation.transport.locality;
      if (l && ((type === 'worker' && l.agent === id) ||
          (type === 'node' && l.node === id))) {
        const p = this.removeSession(operationId, operation.direction, 'Node lost');
        terminations.push(p);
      }
    });
    this.transports.forEach((transport, transportId) => {
      const l = transport.locality;
      if (l && ((type === 'worker' && l.agent === id) ||
          (type === 'node' && l.node === id))) {
        transports.add(transport.id);
      }
    });
    return Promise.all(terminations).then(() => {
      transports.forEach((transportId) => {
        const status = {type: 'failed', reason: 'Owner leave'};
        this.onTransportProgress(transportId, status);
      });
    });
  };

  async setMute(sessionId, tracks, muted) {
    log.debug(`setMute: ${sessionId} tracks: ${tracks} muted: ${muted}`);
    if (!this.operations.has(sessionId)) {
      log.debug(`operation does NOT exist:${sessionId}`);
      return Promise.reject(`operation does NOT exist:${sessionId}`);
    }
    const operation = this.operations.get(sessionId);
    const transport = this.transports.get(operation.transportId);
    const locality = transport.locality;
    const onOff = muted ? 'off' : 'on';
    return this.makeRPC(locality.node, 'mediaOnOff',
        [sessionId, tracks, operation.direction, onOff])
      .catch(reason => log.debug(`setMute failed ${sessionId}`));;
  };

}

exports.RtcController = RtcController;
