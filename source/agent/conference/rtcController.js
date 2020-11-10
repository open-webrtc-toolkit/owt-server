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

  constructor(id, transport, direction, tracks) {
    this.id = id;
    this.transport = transport;
    this.transportId = transport.id;
    this.direction = direction;
    this.tracks = tracks.map(t => new Track(this, t));
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
        if (operation.transportId === transportId) {
          this.emit('session-established', operation);
        }
      }
    } else if (status.type === 'failed') {
      this.emit('transport-aborted', transportId, status.reason);
      // Destroy transport
      if (transport.locality) {
        log.debug(`Destroy transport ${transportId}`);
        this.rpcReq.destroyTransport(transport.locality.node, transportId);
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
      operation.state = COMPLETED;
      if (operation.transport.state === COMPLETED) {
        // Only emit when transport is completed
        this.emit('session-established', operation);
      }
    }
  }

  _createTransportIfNeeded(ownerId, sessionId, origin, tId) {
    if (!this.transports.has(tId)) {
      this.transports.set(tId, new Transport(tId, ownerId, origin));
      const taskConfig = {room: this.roomId, task: sessionId};
      log.debug(`getWorkerNode ${this.clusterRpcId}, ${taskConfig}, ${origin}`);
      return this.rpcReq.getWorkerNode(this.clusterRpcId, 'webrtc', taskConfig, origin)
      .then((locality) => {
        if (!this.transports.has(tId)) {
          log.debug(`Transport destroyed after getWorkerNode ${tId}`);
          this.rpcReq.recycleWorkerNode(locality.agent, locality.node, taskConfig)
            .catch(reason => {
              log.debug('AccessNode not recycled', locality);
            });
          return Promise.reject('Session has been aborted');
        }
        log.debug(`getWorkerNode ok, sessionId: ${sessionId}, locality: ${locality}`);
        this.transports.get(tId).setup(locality);
        return this.transports.get(tId);
      });
    } else {
      return Promise.resolve(this.transports.get(tId));
    }
  }

  // tracks = [ {mid, type, formatPreference, from} ]
  // Return Promise
  initiate(ownerId, sessionId, direction, origin, {transportId, tracks, legacy}) {
    log.debug(`initiate, ownerId:${ownerId}, sessionId:${sessionId}, origin:${origin}, ` +
      `transportId:${transportId}, tracks:${JSON.stringify(tracks)}, legacy:${legacy}`);

    return this._createTransportIfNeeded(ownerId, sessionId, origin, transportId)
    .then(transport => {
      if (transport.state !== PENDING && transport.state !== COMPLETED) {
        return Promise.reject(`Transport ${transportId} is not ready`);
      }
      const locality = transport.locality;

      // Add operation (publish/subscribe)
      if (this.operations.has(sessionId)) {
        log.debug(`operation exists:${sessionId}`);
        return Promise.reject(`operation exists:${sessionId}`);
      }
      const op = new Operation(sessionId, transport, direction, tracks);
      this.operations.set(sessionId, op);
      // Save promise for this operation
      const options = {transportId, tracks, controller: this.roomRpcId};
      op.promise = this.rpcReq.initiate(locality.node, sessionId, 'webrtc', direction, options);
      return op.promise;
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
    operation.promise = operation.promise.then(() => {
      if (!this.operations.has(sessionId)) {
        log.debug(`operation does NOT exist:${sessionId}`);
        return Promise.reject(`operation does NOT exist:${sessionId}`);
      }
      return this.rpcReq.terminate(locality.node, sessionId, direction);
    }).then(() => {
      if (this.operations.has(sessionId)) {
        const owner = transport.owner;
        const abortData = { direction: operation.direction, owner, reason };
        this.emit('session-aborted', sessionId, abortData);
        this.operations.delete(sessionId);

        let emptyTransport = true;
        for (const [id, op] of this.operations) {
          if (op.transportId === transport.id) {
            emptyTransport = false;
            break;
          }
        }
        if (emptyTransport) {
          log.debug(`to recycleWorkerNode: ${locality} task:, ${sessionId}`);
          const taskConfig = {room: this.roomId, task: sessionId};
          return this.rpcReq.recycleWorkerNode(locality.agent, locality.node, taskConfig)
            .catch(reason => {
              log.debug(`AccessNode not recycled ${locality}, ${reason}`);
            })
            .then(() => {
              this.transports.delete(transport.id);
            });
        }
      }
    })
    .catch(reason => {
      log.debug(`Operation terminate failed ${operation}, ${reason}`);
    });
    return operation.promise;
  };

  terminateByOwner(ownerId) {
    log.debug(`terminateByOwner ${ownerId}`);
    const terminations = [];
    // Or just destroy the transport
    this.operations.forEach((operation, operationId) => {
      const transport = this.transports.get(operation.transportId);
      if (transport.owner === ownerId) {
        const p = this.terminate(operationId, operation.direction, 'Owner leave');
        terminations.push(p);
      }
    });
    return Promise.all(terminations);
  };

  terminateByLocality(type, id) {
    log.debug(`terminateByLocality ${type} ${id}`);
    const terminations = [];
    // Or just destroy the transport
    this.operations.forEach((operation, operationId) => {
      const l = this.transports.get(operation.transportId).locality;
      if (l) {
        if ((type === 'worker' && l.agent === id) ||
            (type === 'node' && l.node === id)) {
          const p = this.terminate(operationId, operation.direction, 'Node lost');
          terminations.push(p);
        }
      }
    });
    return Promise.all(terminations);
  };

  onFaultDetected(type, id) {

  }

  destroy() {
    log.debug(`destroy`);
    const terminations = [];
    // Destroy all transports
    this.transports.forEach((transport, transportId) => {
      const p = this.rpcReq.destroyTransport(transportId);
    });
    return Promise.all(terminations);
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

    operation.promise = operation.promise.then(() => {
      if (!this.operations.has(sessionId)) {
        log.debug(`operation does NOT exist:${sessionId}`);
        return Promise.reject(`operation does NOT exist:${sessionId}`);
      }
      return this.rpcReq.mediaOnOff(
        locality.node, sessionId, tracks, operation.direction, onOff);
    });
    const ret = operation.promise;
    operation.promise = operation.promise
      .catch(reason => log.debug(`setMute failed ${sessionId}`));
    return ret;
  };

}

exports.RtcController = RtcController;
