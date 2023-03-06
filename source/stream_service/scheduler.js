// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('./logger').logger.getLogger('Scheduler');

const HASH_SIZE = 512;
function stringHash(str) {
  let hash = 0;
  if (str.length === 0) return hash;
  for (let i = 0; i < str.length; i++) {
    const ch = str.charCodeAt(i);
    hash = ((hash << 5) - hash) + ch;
    hash |= 0;
  }
  return hash % HASH_SIZE + HASH_SIZE;
}

const CHECK_INTERVAL = 60 * 1000; // 1 min
const MAX_RPC_FAILS = 5;

class ServiceScheduler {
  static supportedMethods = [
    'join',
    'leave',
    'publish',
    'unpublish',
    'getPublications',
    'subscribe',
    'unsubscribe',
    'getSubscriptions',
    'streamControl',
    'subscriptionControl',
    'onStatus',
    'addProcessor',
    'removeProcessor',
    'getProcessors',
    'onSessionSignaling',
    'getParticipants',
  ];
  constructor(rpcChannel, stateStores) {
    this.rpcChannel = rpcChannel;
    this.stateStores = stateStores;
    this.checkAliveTimer = setInterval(() => {
      this.checkService();
    }, CHECK_INTERVAL);
    this.failureCounts = new Map(); // Node => count
  }

  async scheduleService(req) {
    const key = (req.participant || '') + (req.domain || '');
    const hash = stringHash(key);
    log.debug('hash:', key, hash);
    const ret = await this.stateStores.readMany('streamEngineNodes', {});
    const serviceNodes = ret.data || [];
    if (serviceNodes.length === 0) {
      throw new Error('No available stream engine nodes');
    }
    const scheduled = await this.stateStores.read('scheduleMaps', {_id: hash});
    if (scheduled) {
      log.debug('scheduled:', scheduled);
      if (!serviceNodes.find((node) => (node.id === scheduled.node))) {
        // Out of date
        log.debug('Node out of date:', scheduled);
        this.stateStores.delete('scheduleMaps', {_id: hash})
        .catch((e) => log.debug('Failed to delete scheduleMaps:', hash));
      } else {
        return scheduled.node;
      }
    }
    const node = serviceNodes[hash % serviceNodes.length].id;
    log.debug('node:', node);
    try {
      const map = {_id: hash, node};
      await this.stateStores.create('scheduleMaps', map);
    } catch (e) {
      log.debug('Failed to update schedule map:', e?.message);
    }
    return node;
  }

  // Check service availability
  checkService() {
    log.debug('Check service availability');
    this.stateStores.readMany('streamEngineNodes', {})
      .then(async (ret) => {
        const serviceNodes = ret.data || [];
        const req = {query: {_id: 'non-existent'}};
        for (const service of serviceNodes) {
          try {
            await this.rpcChannel.makeRPC(
              service.id, 'getNodes', [req]);
            this.failureCounts.delete(service.id);
          } catch (e) {
            log.warn('Failed to call service node:', service.id);
            if (!this.failureCounts.has(service.id)) {
              this.failureCounts.set(service.id, 1);
            }
            this._handleCheckFailure(service.id);
          }
        }
      }).catch((e) => {
        log.debug('Read service error:', e?.message);
      });
  }

  getAPI() {
    const api = {};
    const self = this;
    for (const method of ServiceScheduler.supportedMethods) {
      api[method] = async function (req, callback) {
        let serviceNode = null;
        try {
          serviceNode = await self.scheduleService(req);
          log.debug('Schedule req:', req, serviceNode);
          const ret = await self.rpcChannel.makeRPC(
            serviceNode, method, [req]);
          log.debug('Schedule ret:', ret, serviceNode);
          callback('callback', ret);
        } catch (e) {
          if (serviceNode) {
            self._handleCheckFailure(serviceNode);
          }
          callback('callback', 'error', e?.message || e);
        }
      };
    }
    return api;
  }

  _handleCheckFailure(id) {
    if (!this.failureCounts.has(id)) {
      return;
    }
    let count = this.failureCounts.get(id) + 1;
    if (count >= MAX_RPC_FAILS) {
      this.stateStores.delete('streamEngineNodes', {id})
        .catch((e) => log.info('Fail to clean service node:', id))
      this.failureCounts.delete(id);
    } else {
      this.failureCounts.set(service.id, count);
    }
  }
}

module.exports.ServiceScheduler = ServiceScheduler;
