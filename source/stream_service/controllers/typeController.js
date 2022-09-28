// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const { EventEmitter } = require('events');
const _ = require('lodash');
const log = require('../logger').logger.getLogger('TypeController');

/* Events
 * 'session-established': (id, Publication|Subscription)
 * 'session-updated': (id, Publication|Subscription)
 * 'session-aborted': (id, reason)
 */
class TypeController extends EventEmitter {
  static methods = [
    'createSession',
    'removeSession',
    'controlSession',
    'destroy',
  ]

  /*
   * rpc: {
   *   makeRPC: (string, string, Array<any>)=>Promise<any>,
   *   selfId: string,
   *   clusterId: string,
   * }
   * stateManage: {
   *   create: (key: string, any)=>Promise<any>,
   *   read: (key: string, any)=>Promise<void>,
   *   update: (key: string, any, condition: any)=>Promise<void>
   *   delete: (key: string)=>Promise<void>
   *   createTransaction: ()=> Transaction {
   *     run: (function)=>Promise<boolean>,
   *     end: ()=>Promise<void>
   *   }
   * }
   */
  constructor(rpc) {
    super();
    log.debug('rpc:', rpc);
    this.makeRPC = rpc.makeRPC.bind(rpc);
    log.debug('this.makeRPC:', this.makeRPC);
    this.selfId = rpc.selfId;
    this.clusterId = rpc.clusterId;
  }

  // Return Promise<{agent: string, node: string}>
  async getWorkerNode(purpose, domain, taskId, preference) {
    log.debug('getWorkerNode:', purpose, domain, taskId, preference);
    log.debug('this:', this);
    const args =  [purpose, taskId, preference, 30 * 1000];
    return this.makeRPC(this.clusterId, 'schedule', args)
      .then((workerAgent) => {
        const taskConfig = {room: domain, task: taskId};
        return this.makeRPC(workerAgent.id, 'getNode', [taskConfig])
          .then((workerNode) => {
            return {agent: workerAgent.id, node: workerNode};
          });
      });
  }

  // locality: {agent: string, node: string}
  async recycleWorkerNode(locality, domain, taskId) {
    log.debug('recycleWorkerNode:', locality, domain, taskId);
    const args = [locality.node, {room: domain, task: taskId}];
    return this.makeRPC(locality.agent, 'recycleNode', args);
  }
}

class LocalState {
  constructor(type) {
    this.type = type ? type + '.' : '';
    this.tsc = false;
    this.state = new Map();
  }
  async create(key, value) {
    key = type + key;
    if (this.state.has(key)) {
      throw new Error('Duplicate key');
    }
    this.state.set(key, value);
    return value;
  }
  async read(key, value) {
    key = type + key;
    return this.state.get(key, value);
  }
  // condition: {op: string, path: string, value: any}
  async update(key, value, condition) {
    key = type + key;
    if (!this.state.has(key)) {
      throw new Error(`${key} not found`);
    }
    const prev = this.state.get(key);
    if (condition) {
      if (condition.op === 'eq') {
        if (_.get(prev, condition.path) !== condition.value) {
          throw new Error('Update condition not match');
        }
      } else if (condition.op === 'exists') {
        if (!_.get(prev, condition.path)) {
          throw new Error('Update condition not match');
        }
      }
    }
    this.state.set(key, value);
  }
  createTransaction() {
    throw new Error('Transaction not supported');
  }
}


exports.TypeController = TypeController;
exports.LocalState = LocalState;
