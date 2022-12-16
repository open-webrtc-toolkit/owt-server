// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('../logger').logger.getLogger('VirtualController');
const {TypeController} = require('./typeController');
const {Publication, Subscription} = require('../stateTypes')

/* Events
 * 'session-established': (id, Publication|Subscription)
 * 'session-updated': (id, Publication|Subscription)
 * 'session-aborted': (id, reason)
 */
// Controller for virtual publication & subscriptioni
class VirtualController extends TypeController {
  constructor(rpc) {
    super(rpc);
    this.pubs = new Map();
    this.subs = new Map();
  }

  // config = {id, data}
  async createSession(direction, config) {
    if (direction === 'in') {
      if (this.pubs.has(config.id)) {
        throw new Error(`Session-in ${config.id} already exists`);
      }
      const pub = Publication.from(config.data);
      this.pubs.set(config.id, pub);
      this.emit('session-established', config.id, pub);
    } else if (direction === 'out') {
      if (this.subs.has(config.id)) {
        throw new Error(`Session-out ${config.id} already exists`);
      }
      const sub = Subscription.from(config.data);
      this.subs.set(config.id, sub);
      this.emit('session-established', config.id, sub);
    } else {
      throw new Error(`Invalid session direction ${direction}`);
    }
  }

  async removeSession(sessionId, direction, reason) {
    if (direction === 'in') {
      if (!this.pubs.has(sessionId)) {
        throw new Error(`Session-in ${sessionId} not found`);
      }
      this.pubs.delete(sessionId);
    } else if (direction === 'out') {
      if (this.subs.has(sessionId)) {
        throw new Error(`Session-out ${sessionId} not found`);
      }
      this.subs.delete(sessionId);
    } else {
      throw new Error(`Invalid session direction ${direction}`);
    }
    this.emit('session-aborted', sessionId, reason);
  }

  async controlSession(direction, config) {
    if (direction === 'in') {
      if (this.pubs.has(config.id)) {
        const pub = Publication.from(config.data);
        this.pubs.set(config.id, pub);
        this.emit('session-updated', config.id, pub);
      } else {
        throw new Error(`Session-in ${config.id} does not exist`);
      }
    } else if (direction === 'out') {
      if (this.subs.has(config.id)) {
        const sub = Subscription.from(config.data);
        this.subs.set(config.id, pub);
        this.emit('session-updated', config.id, sub);
      } else {
        throw new Error(`Session-out ${config.id} does not exist`);
      }
    } else {
      throw new Error(`Invalid session direction ${direction}`);
    }
  }

  destroy() {
  }
}

exports.VirtualController = VirtualController;
