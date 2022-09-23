// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('./logger').logger.getLogger('DomainHandler');
const {Publication, Subscription, Processor} = require('./session');

const FORMAT_PREFERENCE = {
  audio: {optional: [{codec: 'opus'}]},
  video: {optional: [{codec: 'vp8'}, {codec: 'h264'}]},
};

// Up level controller for certain domain streams
class DomainHandler {
  constructor() {
    this.domain = null;
    this.participants = new Map(); // id => participant
    this.streams = new Map();
    this.subscriptions = new Map();
  }

  // Join from portal
  // req = {id, domain}
  async join(req) {
    log.debug('join:', req);
    if (!this.domain) {
      // Init
      log.debug('Init handler:', req.domain);
      this.domain = req.domain;
    }
    if (req.domain !== this.domain) {
      throw new Error('Invalid domain');
    }
    if (this.participants.has(req.id)) {
      throw new Error('Duplicate join ID');
    }
    // Build join response
    const ret = {
      permission: {},
      room: {
        id: this.domain,
        participants: [],
        streams: [],
      },
    };
    for (const [id, ppt] of this.participants) {
      ret.room.participants.push(ppt);
    }
    for (const [id, st] of this.streams) {
      ret.room.streams.push(st.toSignalingFormat());
    }
    this.participants.set(req.id, req);
    return ret;
  }

  // Leave from portal
  async leave(req) {
    log.debug('leave:', req);
    if (!this.participants.has(req.id)) {
      throw new Error('Invalid leave ID');
    }
    // Clean up streams & subscriptions
    this.participants.delete(req.id);
  }

  // Publish from portal
  async publish(req) {
    log.debug('publish:', req);
    // Validate request
    if (!this.participants.has(req.participant)) {
      throw new Error('Invalid participant ID');
    }
    const id = req.id || randomUUID().replace(/-/g, '');
    const info = {
      owner: this.participant,
      type: req.type,
      attributes: req.attributes,
      formatPreference: FORMAT_PREFERENCE,
      origin: {domain: this.domain},
    };
    return {id, info};
  }
  // Subscribe from portal
  async subscribe(req) {
    log.debug('subscribe:', req);
    // Validate request
    if (!this.participants.has(req.participant)) {
      throw new Error('Invalid pariticpant ID');
    }
    const id = req.id || randomUUID().replace(/-/g, '');
    const info = {
      owner: this.participant,
      type: req.type,
      formatPreference: FORMAT_PREFERENCE,
      origin: {domain: this.domain},
    };
    return {id, info};
  }
  // StreamControl from portal
  async streamControl(req) {
    log.debug('streamControl:', req);
    // Validate request
    if (!this.participants.has(req.participant)) {
      throw new Error('Invalid pariticpant ID');
    }
    return {id: req.id};
  }
  // SubscriptionControl from portal
  async subscriptionControl(req) {
    log.debug('subscriptionControl:', req);
    // Validate request
    if (!this.participants.has(req.participant)) {
      throw new Error('Invalid pariticpant ID');
    }
    return {id: req.id};
  }

  // Status of publication/subscription
  /*
  req = {
    type: 'publication/subscription',
    status: 'update|add|remove',
    data: publication | subscription
  }
  */
  async onStatus(req) {
    log.debug('onStatus:', req);
    if (req.type === 'publication') {
      const pub = req.data;
      if (req.status === 'add') {
        this.streams.set(pub.id, Publication.create(pub));
      } else if (req.status === 'remove') {
        this.streams.delete(pub.id);
      }
    } else if (req.type === 'subscription') {
      const sub = req.data;
      if (req.status === 'add') {
        this.subscriptions.set(sub.id, sub);
      } else if (req.status === 'remove') {
        this.subscriptions.delete(sub.id);
      }
    }
  }
}

class RemoteDomainHandler {
  static supportedMethods = [
    'join',
    'leave',
    'publish',
    'subscribe',
    'streamControl',
    'subscriptionControl',
    'onStatus',
  ];

  constructor(locality, rpcChannel) {
    this.locality = locality;
    this.rpcChannel = rpcChannel;
    // Call rpc for supported methods
    for (const method of RemoteDomainHandler.supportedMethods) {
      this[method] = async function (req) {
        return this._rpc(method, req);
      };
    }
  }
  // Make RPC to remote node
  async _rpc(method, req) {
    if (!RemoteDomainHandler.supportedMethods.includes(method)) {
      throw new Error(`Method ${method} is not supported`);
    }
    if (!this.locality?.node) {
      throw new Error(`Locality ${locality} is invalid`);
    }
    return this.rpcChannel.makeRPC(
      this.locality.node, method, [req]);
  }
}

module.exports.DomainHandler = DomainHandler;
module.exports.RemoteDomainHandler = RemoteDomainHandler;
