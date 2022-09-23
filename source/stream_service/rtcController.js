// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const { randomUUID } = require('crypto');
const { EventEmitter } = require('events');

const log = require('./logger').logger.getLogger('RtcController2');

let Publication;
let Subscription;

try {
  ({Publication, Subscription} = require('./session'));
} catch (e) {
  log.debug('Failed to load session.js');
}

// Transport/Operation state
const INITIALIZING = 'initializing';
const COMPLETED = 'completed';
const CLOSED = 'closed';
const PENDING = 'pending';

class Transport {

  constructor(id, owner, origin) {
    this.id = id;
    this.owner = owner;
    this.origin = origin;
    this.locality = null;
    this.state = INITIALIZING;
  }

  setup(locality) {
    this.locality = locality;
    this.state = PENDING;
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

  toSignalingFormat() {
    return {
      id: this.id,
      type: "forward",
      media: {
        tracks: this.tracks.map((track) => {
          return {
            id: track.id,
            type: track.type,
            format: track.format,
            parameters: track.parameters,
            status: "active",
            source: undefined,
            mid: track.mid,
            rid: track.rid,
            optional: {
              format: [],
              parameters: {
                resolution: [],
                bitrate: [],
                framerate: [],
                keyFrameInterval: [],
              },
            }
          };
        }),
      },
      data: false,
      info: {
        owner: this.transport.owner,
        type: "webrtc",
        inViews: [],
        attributes: null,
      }
    };
  }

  toSessionFormat() {
    if (Publication && Subscription) {
      const info = {owner: this.transport.owner};
      if (this.direction === 'in') {
        const publication = new Publication(this.id, 'webrtc', info);
        publication.locality = this.transport.locality;
        this.tracks.forEach((track) => {
          publication.source[track.type].push(track);
        });
        publication.domain = this.transport.origin.domain;
        return publication;
      } else {
        const subscription = new Subscription(this.id, 'webrtc', info);
        subscription.locality = this.transport.locality;
        this.tracks.forEach((track) => {
          subscription.sink[track.type].push(track);
        });
        subscription.domain = this.transport.origin.domain;
        return subscription;
      }
    }
    return null;
  }
}

/* Events
 * 'transport-established': (id)
 * 'transport-aborted': (id, reason)
 * 'transport-signaling': (id, {type, ... })
 * Events for conference.js
 * 'session-established': (id, Operation)
 * 'session-updated': (id, Operation)
 * 'session-aborted': (id, {owner, direction, reason})
 */
class RtcController extends EventEmitter {

  /* rpcReq {
   *   getWorkerNode, recycleWorkerNode, initiate, terminate,
   *   onTransportSignaling, mediaOnOff, sendMsg
   * }
   */
  constructor(roomId, rpcReq, roomRpcId, clusterRpcId) {
    log.debug(`constructor ${roomId}, ${roomRpcId}, ${clusterRpcId}`);
    super();
    this.roomRpcId = roomRpcId;
    this.rpcReq = rpcReq;
    this.clusterRpcId = clusterRpcId;
    // Map {transportId => Transport}
    this.transports = new Map();
    // Map {operationId => Operation}
    this.operations = new Map();
    // Map {publicTrackId => Track}
    this.tracks = new Map();
  }

  // info = {id, owner, formatPreference = [{preferred, optional: []}], origin}
  async handleRTCSignaling(name, data, info) {
    switch(name) {
      case 'soac': {
        return this.onClientTransportSignaling(data.id, data.signaling)
          .catch((e) => log.warn('Failed to process soac:', e));
      }
      case 'publish':
      case 'subscribe': {
        const formatPreference = info.formatPreference || [];
        const owner = info.owner || '';
        const id = info.id || randomUUID().replace(/-/g, '');
        const direction = (name === 'publish')? 'in' : 'out';
        const option = {
          transportId: data.transport?.id || id,
          tracks: data.media.tracks,
          legacy: data.legacy,
          attributes: data.attributes,
        };
        option.tracks.forEach((track, i) => {
          track.formatPreference = formatPreference[i];
        });
        info.origin.domain = info.domain;
        return this.initiate(owner, id, direction, info.origin, option)
          .then(() => ({id, transportId: option.transportId}))
          .catch((e) => log.warn('Failed to initiate', id, ':', e));
      }
      case 'unpublish':
      case 'unsubscribe': {
        const id = data.id;
        const direction = (name === 'unpublish')? 'in' : 'out';
        return this.terminate(id, direction, 'Participant terminate')
          .catch((e) => log.warn('Failed to terminate', id, ':', e));
      }
      case 'stream-control':
      case 'subscription-control': {
        const id = data.id;
        if (['pause', 'play'].includes(data.operation)) {
          const setAudio = data.data.includes('a');
          const setVideo = data.data.includes('v');
          let tracks = [];
          if (this.operations.has(id)) {
            setTracks = this.operations.get(id).tracks
              .filter((track) => {
                return track.id && ((track.type === 'audio' && setAudio) ||
                    (track.type === 'video' && setVideo));
              })
              .map((track) => track.id);
          }
          const muted = (data.operation === 'pause') ? true : false;
          if (tracks.length > 0) {
            return this.setMute(id, setTracks, muted)
              .catch((e) => log.warn('Failed to setMute', id, ':', e));
          } else {
            log.debug('Cannot find track for mute/unmute:', id);
          }
        } else if ('update' === data.operation) {
          log.info('Skip "update" operation from signaling');
        } else if ('querybwe' === data.operation) {
          log.info('Skip "querybwe" operation from signaling');
        }
        break;
      }
      default:
        break;
    }
  }

  /*
  sessionConfig = {
    id: id,
    transport: {id},
    media: {
      tracks: [initTrack]
    },
    domain: domain,
    info: {
      owner: ownerId,
      origin: origin,
      attributes: attrObject,
      formatPreference: {audio, video}
    },
    participant: pariticipantId,
  }
   */
  async createSession(direction, config) {
    const option = {
      transportId: config.transport?.id || id,
      tracks: config.media.tracks,
      attributes: config.attributes,
    };
    option.tracks.forEach((track) => {
      track.formatPreference = config.info.formatPreference[track.type];
    });
    return this.initiate(
        config.participant, config.id, direction, config.info.origin, option)
      .then(() => ({id: config.id, transportId: option.transportId}));
  }

  async removeSession(id, direction, reason) {
    return this.terminate(id, direction, reason || 'Participant terminate');
  }

  /*
  sessionConfig = {
    id: id,
    operation: op,
    data: 'audio/video'
  }
   */
  async controlSession(direction, config) {
    const id = data.id;
    if (['pause', 'play'].includes(config.operation)) {
      const setAudio = config.data.includes('a');
      const setVideo = config.data.includes('v');
      let tracks = [];
      if (this.operations.has(id)) {
        setTracks = this.operations.get(id).tracks
          .filter((track) => {
            return track.id && ((track.type === 'audio' && setAudio) ||
                (track.type === 'video' && setVideo));
          })
          .map((track) => track.id);
      }
      const muted = (data.operation === 'pause') ? true : false;
      if (tracks.length > 0) {
        return this.setMute(id, setTracks, muted);
      } else {
        throw new Error(`Cannot find track for mute/unmute: ${id}`);
      }
    } else {
      throw new Error(`Operation not supported: ${config.operation}`);
    }
  }

  onSessionProgress(id, type, data) {
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
        const track = this.tracks.get(id);
        const op = this.operations.get(track?.operationId);
        if (op) {
          const updateParameters = data.video?.parameters;
          track.parameters = track.parameters || {};
          track.parameters.resolution = data.video?.parameters?.resolution;
          this.emit('session-updated', op.id, {type: 'update', data: op});
        }
        break;
      }
      default:
        break;
    }
  }

  // Return Transport
  getTransport(transportId) {
    return this.transports.get(transportId);
  }

  // Return Opreation
  getOperation(operationId) {
    return this.operations.get(operationId);
  }

  // Return Track
  getTrack(trackId) {
    return this.tracks.get(trackId);
  }

  onTransportProgress(transportId, status) {
    if (!this.transports.has(transportId)) {
      log.error(`onTransportProgress: Transport ${transportId} does NOT exist`);
      return;
    }
    const transport = this.transports.get(transportId);
    if (status.type === 'ready') {
      transport.state = COMPLETED;
      this.emit('transport-established', transportId);
      this.emit('signaling', transport.owner, 'progress',
          {id: transportId, status: 'ready'});
      // Emit pending completed tracks
      for (const [id, operation] of this.operations) {
        if (operation.transportId === transportId &&
            operation.state === COMPLETED) {
          this.emit('session-established', operation.id, operation.toSessionFormat());
          this.emit('signaling', transport.owner, 'progress',
              {id: transportId, sessionId: operation.id, status: 'ready'});
        }
      }
    } else if (status.type === 'failed') {
      this.emit('transport-aborted', transportId, status.reason);
      // Destroy transport
      if (transport.locality) {
        log.debug(`Destroy transport ${transportId}`);
        this.rpcReq.destroyTransport(transport.locality.node, transportId)
        .catch((e) => {
          log.debug(`Faild to clean up transport ${transportId}: ${e}`);
        }).then(() => {
          const locality = transport.locality;
          log.debug(`to recycleWorkerNode: ${locality} task:, ${transportId}`);
          const taskConfig = {room: transport.origin.domain, task: transportId};
          return this.rpcReq.recycleWorkerNode(locality.agent, locality.node, taskConfig)
        }).catch((e) => log.debug(`Failed to recycleWorkerNode ${locality}`));
      } else {
        log.warn(`No locality for failed transport ${transportId}`);
      }
    } else if (status.type === 'offer' || status.type === 'answer' || status.type === 'candidate') {
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
    return this.rpcReq.onTransportSignaling(locality.node, transportId, signaling)
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

  async _createTransportIfNeeded(ownerId, sessionId, origin, tId) {
    if (!this.transports.has(tId)) {
      const taskConfig = {room: origin.domain, task: tId};
      log.debug(`getWorkerNode ${this.clusterRpcId}, ${taskConfig}, ${origin}`);
      const locality = await this.rpcReq.getWorkerNode(
          this.clusterRpcId, 'webrtc', taskConfig, origin);
      this.transports.set(tId, new Transport(tId, ownerId, origin));
      this.transports.get(tId).setup(locality);
    }
    return this.transports.get(tId);
  }

  // tracks = [ {mid, type, formatPreference, from} ]
  // Return Promise
  initiate(ownerId, sessionId, direction, origin, {transportId, tracks, legacy, attributes}) {
    log.debug(`initiate, ownerId:${ownerId}, sessionId:${sessionId}, origin:${JSON.stringify(origin)}, ` +
      `transportId:${transportId}, tracks:${JSON.stringify(tracks)}, legacy:${legacy}, attributes:${attributes}`);

    return this._createTransportIfNeeded(ownerId, sessionId, origin, transportId)
    .then(transport => {
      if (!this.transports.has(transportId)) {
        return Promise.reject(`Transport ${transportId} is not ready`);
      }
      const locality = transport.locality;

      // Add operation (publish/subscribe)
      if (this.operations.has(sessionId)) {
        log.debug(`operation exists:${sessionId}`);
        return Promise.reject(`operation exists:${sessionId}`);
      }
      const op = new Operation(sessionId, transport, direction, tracks, legacy, attributes);
      this.operations.set(sessionId, op);
      // Return promise for this operation
      const options = {transportId, tracks, controller: this.roomRpcId, owner: ownerId};
      return this.rpcReq.initiate(locality.node, sessionId, 'webrtc', direction, options);
    });
  }

  // Return Promise
  terminate(sessionId, direction, reason) {
    log.debug(`terminate, sessionId: ${sessionId} direction: ${direction}, ${reason}`);

    if (!this.operations.has(sessionId)) {
      log.debug(`operation does NOT exist:${sessionId}`);
      return Promise.reject(`operation does NOT exist:${sessionId}`);
    }
    const operation = this.operations.get(sessionId);
    const transport = this.transports.get(operation.transportId);
    const locality = transport.locality;
    return this.rpcReq.terminate(locality.node, sessionId, direction).then(() => {
      if (this.operations.has(sessionId)) {
        const owner = transport.owner;
        const abortData = { direction: operation.direction, owner, reason };
        this.emit('session-aborted', sessionId, abortData);
        this.emit('signaling', owner, 'progress',
            {id: operation.transportId, sessionId, status: 'error', data: reason});
        this.operations.delete(sessionId);
      }
    })
    .catch(reason => {
      log.debug(`Operation terminate failed ${operation}, ${reason}`);
    });
  };

  terminateByOwner(ownerId) {
    log.debug(`terminateByOwner ${ownerId}`);
    const terminations = [];
    this.operations.forEach((operation, operationId) => {
      if (operation.transport.owner === ownerId) {
        const p = this.terminate(operationId, operation.direction, 'Owner leave');
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

  terminateByLocality(type, id) {
    log.debug(`terminateByLocality ${type} ${id}`);
    const terminations = [];
    const transports = new Set();
    this.operations.forEach((operation, operationId) => {
      const l = operation.transport.locality;
      if (l && ((type === 'worker' && l.agent === id) ||
          (type === 'node' && l.node === id))) {
        const p = this.terminate(operationId, operation.direction, 'Node lost');
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

  destroy() {
    log.debug(`destroy`);
    // Destroy all transports
    this.transports.forEach((transport, transportId) => {
      const status = {type: 'failed', reason: 'Owner leave'};
        this.onTransportProgress(transportId, status);
    });
  };

  setMute(sessionId, tracks, muted) {
    log.debug(`setMute, sessionId: ${sessionId} tracks: ${tracks} muted: ${muted}`);
    if (!this.operations.has(sessionId)) {
      log.debug(`operation does NOT exist:${sessionId}`);
      return Promise.reject(`operation does NOT exist:${sessionId}`);
    }
    const operation = this.operations.get(sessionId);
    const transport = this.transports.get(operation.transportId);
    const locality = transport.locality;
    const onOff = muted ? 'off' : 'on';
    return this.rpcReq.mediaOnOff(
        locality.node, sessionId, tracks, operation.direction, onOff)
            .catch(reason => log.debug(`setMute failed ${sessionId}`));;
  };

}

exports.RtcController = RtcController;
