// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file was initially copied from rtcController. But a lot of changes were made for QuicTransport.
// The design for *Controllers is a little bit confused. It would be better to have a single controller manages all sessions.

'use strict';

const { EventEmitter } = require('events');
const log = require('./logger').logger.getLogger('QuicController');

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

class Operation {
  constructor(id, transport, direction, tracks, data) {
    if(tracks){
      throw new Error('QUIC agent does not support media stream tracks so far.');
    }
    this.id = id;
    this.transport = transport;
    this.transportId = transport.id;
    this.direction = direction;
    this.data = data;
    this.promise = Promise.resolve();
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
class QuicController extends EventEmitter {

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
          log.error('onSessionProgress is called by QUIC connections.');
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

  _createTransportIfNeeded(ownerId, sessionId, origin, tId) {
    if (!this.transports.has(tId)) {
      this.transports.set(tId, new Transport(tId, ownerId, origin));
      const taskConfig = {room: this.roomId, task: sessionId};
      log.debug(`getWorkerNode ${this.clusterRpcId}, ${taskConfig}, ${origin}`);
      return this.rpcReq.getWorkerNode(this.clusterRpcId, 'quic', taskConfig, origin)
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
  initiate(ownerId, sessionId, direction, origin, {transportId, tracks, data, legacy}) {
    log.debug(`initiate, ownerId:${ownerId}, sessionId:${sessionId}, origin:${origin}, ` +
      `transportId:${transportId}, tracks:${JSON.stringify(tracks)}, legacy:${legacy}`);

    return this._createTransportIfNeeded(ownerId, sessionId, origin, transportId)
    .then(transport => {
      log.debug('Success create transport.');
      if (transport.state !== PENDING && transport.state !== COMPLETED) {
        return Promise.reject(`Transport ${transportId} is not ready`);
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
      const options = {transport:{id:transportId, type:'quic'}, tracks, controller: this.roomRpcId, data};
      op.promise = this.rpcReq.initiate(locality.node, sessionId, 'quic', direction, options);
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


}

exports.QuicController = QuicController;
