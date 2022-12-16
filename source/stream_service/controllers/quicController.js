// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file was initially copied from rtcController. But a lot of changes were made for QuicTransport.
// The design for *Controllers is a little bit confused. It would be better to have a single controller manages all sessions.

'use strict';

const log = require('../logger').logger.getLogger('QuicController');
const {TypeController} = require('./typeController');
const {Publication, Subscription} = require('../stateTypes')

// Transport/Operation state
const INITIALIZING = 'initializing';
const COMPLETED = 'completed';
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

class Operation {
  constructor(id, transport, direction, tracks, data) {
    this.id = id;
    this.transport = transport;
    this.transportId = transport.id;
    this.direction = direction;
    this.tracks = tracks;
    this.data = data;
    this.promise = Promise.resolve();
    this.tracks = this.tracks ? this.tracks.map(t => {
        if (t.type === 'video') {
            t.format = { codec : 'h264', profile : 'B' };
        }
        return t;
    })
                              : undefined;
  }
}

/* Events
 * 'transport-established': (id)
 * 'transport-aborted': (id, reason)
 * Events for conference.js
 * 'session-established': (id, Operation)
 * 'session-updated': (id, Operation)
 * 'session-aborted': (id, {owner, direction, reason})
 */
class QuicController extends TypeController {
  constructor(rpc) {
    super(rpc);
    // Map {transportId => Transport}
    this.transports = new Map();
    // Map {operationId => Operation}
    this.operations = new Map();
  }

  // Return Transport
  getTransport(transportId) {
    return this.transports.get(transportId);
  }

  // Return Opreation
  getOperation(operationId) {
    return this.operations.get(operationId);
  }

  onSessionProgress(sessionId, status)
  {
      if (!status.data) {
          log.error('QUIC agent only support data forwarding.');
          return;
      }
      if (status.type === 'ready') {
          if (!this.operations.get(sessionId)) {
              log.error('Invalid session ID.');
              return;
          }
          this.emit('session-established', this.operations.get(sessionId));
      }
  }

  async _createTransportIfNeeded(ownerId, domain, tId, origin) {
    if (!this.transports.has(tId)) {
      this.transports.set(tId, new Transport(tId, ownerId, origin));
      const locality = await this.getWorkerNode('quic', domain, tId, origin);
      if (!this.transports.has(tId)) {
        log.debug(`Transport destroyed after getWorkerNode ${tId}`);
        this.recycleWorkerNode(locality, domain, tId)
          .catch(reason => {
            log.debug('AccessNode not recycled', locality);
          });
        throw new Error('Session has been aborted');
      }
      log.debug(`getWorkerNode ok, sessionId: ${sessionId}, locality: ${locality}`);
      this.transports.get(tId).setup(locality);
      this.transports.get(tId).domain = domain;
      return this.transports.get(tId);
    } else {
      return this.transports.get(tId);
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
    log.debug(`createSession, ${direction} ${config}`);
    const sessionId = config.id;
    const transportId = config.transport?.id || config.id;
    const owner = config.participant;
    const transport = await this._createTransportIfNeeded(
      owner, config.domain, transportId, config.info.origin);
    log.debug('Success create transport.');
    if (transport.state !== PENDING && transport.state !== COMPLETED) {
      throw new Error(`Transport ${transportId} is not ready`);
    }
    const locality = transport.locality;
    // Add operation (publish/subscribe)
    if (this.operations.has(sessionId)) {
      log.debug(`operation exists:${sessionId}`);
      return Promise.reject(`operation exists:${sessionId}`);
    }
    const op = new Operation(sessionId, transport, direction, tracks, data);
    this.operations.set(sessionId, op);
    // Save promise for this operation
    const options = {transport:{id:transportId, type:'quic'}, tracks, controller: this.selfId, data};
    // Make RPC call
    const rpcNode = locality.node;
    const method = direction === 'in' ? 'publish' : 'subscribe';
    return this.makeRPC(rpcNode, method, [sessionId, 'quic', options]);
  }

  async removeSession(sessionId, direction, reason) {
    log.debug(`removeSession: ${sessionId} ${direction} ${reason}`);
    if (!this.operations.has(sessionId)) {
      log.debug(`operation does NOT exist:${sessionId}`);
      throw new Error(`operation does NOT exist:${sessionId}`);
    }
    const operation = this.operations.get(sessionId);
    const transport = this.transports.get(operation.transportId);
    const locality = transport.locality;
    operation.promise = operation.promise.then(() => {
      if (!this.operations.has(sessionId)) {
        log.debug(`operation does NOT exist:${sessionId}`);
        return Promise.reject(`operation does NOT exist:${sessionId}`);
      }
      const method = direction === 'in' ? 'unpublish' : 'unsubscribe';
      return this.makeRPC(locality.node, method, [sessionId])
    }).then(() => {
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
    return operation.promise;
  }

  // RPC API as progress listener
  progressListeners() {
    //TODO: add progress listeners for QUIC
    return {
      // onTransportProgress: this.onTransportProgress.bind(this),
      // onTrackUpdate: this.onTrackUpdate.bind(this),
      // onMediaUpdate: this.onMediaUpdate.bind(this),
    };
  }

  terminateByOwner(ownerId) {
    log.debug(`terminateByOwner ${ownerId}`);
    const terminations = [];
    // Or just destroy the transport
    this.operations.forEach((operation, operationId) => {
      const transport = this.transports.get(operation.transportId);
      if (transport.owner === ownerId) {
        const p = this.removeSession(operationId, operation.direction, 'Owner leave');
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
          const p = this.removeSession(operationId, operation.direction, 'Node lost');
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
      // const p = this.rpcReq.destroyTransport(transportId);
    });
    return Promise.all(terminations);
  };


}

exports.QuicController = QuicController;
