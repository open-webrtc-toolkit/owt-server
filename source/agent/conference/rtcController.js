// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const { EventEmitter } = require('events');
const log = require('./logger').logger.getLogger('RtcController');

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

  constructor(id, transport, direction, tracks, legacy) {
    this.id = id;
    this.transport = transport;
    this.transportId = transport.id;
    this.direction = direction;
    this.tracks = tracks.map(t => new Track(this, t));
    this.legacy = legacy;
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
    this.roomId = roomId;
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
      // Emit pending completed tracks
      for (const [id, operation] of this.operations) {
        if (operation.transportId === transportId &&
            operation.state === COMPLETED) {
          this.emit('session-established', operation);
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
          const taskConfig = {room: this.roomId, task: transportId};
          return this.rpcReq.recycleWorkerNode(locality.agent, locality.node, taskConfig)
        }).catch((e) => log.debug(`Failed to recycleWorkerNode ${locality}`));
      } else {
        log.warn(`No locality for failed transport ${transportId}`);
      }
    } else if (status.type === 'offer' || status.type === 'answer' || status.type === 'candidate') {
      this.emit('transport-signaling', transport.owner, transportId, status);
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
          this.emit('session-established', operation);
        }
      }
    }
  }

  async _createTransportIfNeeded(ownerId, sessionId, origin, tId) {
    if (!this.transports.has(tId)) {
      const taskConfig = {room: this.roomId, task: tId};
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
  initiate(ownerId, sessionId, direction, origin, {transportId, tracks, legacy}) {
    log.debug(`initiate, ownerId:${ownerId}, sessionId:${sessionId}, origin:${origin}, ` +
      `transportId:${transportId}, tracks:${JSON.stringify(tracks)}, legacy:${legacy}`);

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
      const op = new Operation(sessionId, transport, direction, tracks, legacy);
      this.operations.set(sessionId, op);
      // Return promise for this operation
      const options = {transportId, tracks, controller: this.roomRpcId};
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
