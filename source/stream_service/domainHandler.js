// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const {randomUUID} = require('crypto');
const log = require('./logger').logger.getLogger('DomainHandler');

const FORMAT_PREFERENCE = {
  audio: {optional: [{codec: 'opus', sampleRate: 48000, channelNum: 2}]},
  video: {optional: [
    {codec: 'vp8'},
    {codec: 'h264', profile: 'CB'},
    {codec: 'h264', profile: 'B'}
  ]},
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
    req.notifying = true;
    this.participants.set(req.id, req);
    return {
      participant: req,
      streams: [],
    };
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
    req.id = req.id || randomUUID().replace(/-/g, '');
    req.info = {
      owner: this.participant,
      type: req.type,
      attributes: req.attributes,
      formatPreference: FORMAT_PREFERENCE,
      origin: {domain: this.domain},
    };
    return req;
  }
  // Subscribe from portal
  async subscribe(req) {
    log.debug('subscribe:', req);
    // Validate request
    if (!this.participants.has(req.participant)) {
      throw new Error('Invalid pariticpant ID');
    }
    req.id = req.id || randomUUID().replace(/-/g, '');
    req.info = {
      owner: this.participant,
      type: req.type,
      formatPreference: FORMAT_PREFERENCE,
      origin: {domain: this.domain},
    };
    return req;
  }
  // StreamControl from portal
  async streamControl(req) {
    log.debug('streamControl:', req);
    // Validate request
    if (!this.participants.has(req.participant)) {
      throw new Error('Invalid pariticpant ID');
    }
    return req;
  }
  // SubscriptionControl from portal
  async subscriptionControl(req) {
    log.debug('subscriptionControl:', req);
    // Validate request
    if (!this.participants.has(req.participant)) {
      throw new Error('Invalid pariticpant ID');
    }
    return req;
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
        this.streams.set(pub.id, pub);
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
  // Add processor request
  async addProcessor(req) {
    req.id = req.id || randomUUID().replace(/-/g, '');
    return req;
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
    'addProcessor',
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
