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
      return scheduled.node;
    } else {
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
  }

  // Check service availability
  checkService() {
    log.debug('Check service availability');
    this.stateStores.readMany('streamEngineNodes', {})
      .then(async (ret) => {
        const serviceNodes = ret.data || [];
        const req = {id: 'non-existent'};
        for (const service of serviceNodes) {
          try {
            await this.rpcChannel.makeRPC(
              service.id, 'getNodes', [req]);
          } catch (e) {
            log.warn('Failed to call service node:', service.id);
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
        try {
          const serviceNode = await self.scheduleService(req);
          log.debug('Schedule req:', req, serviceNode);
          const ret = await self.rpcChannel.makeRPC(
            serviceNode, method, [req]);
          log.debug('Schedule ret:', ret, serviceNode);
          callback('callback', ret);
        } catch (e) {
          callback('callback', 'error', e?.message || e);
        }
      };
    }
    return api;
  }
}

module.exports.ServiceScheduler = ServiceScheduler;
