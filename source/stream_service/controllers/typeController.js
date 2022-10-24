// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const { EventEmitter } = require('events');
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
   */
  constructor(rpc) {
    super();
    this.makeRPC = rpc.makeRPC.bind(rpc);
    this.selfId = rpc.selfId;
    this.clusterId = rpc.clusterId;
  }

  // Return Promise<{agent: string, node: string}>
  async getWorkerNode(purpose, domain, taskId, preference) {
    log.debug('getWorkerNode:', purpose, domain, taskId, preference);
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

exports.TypeController = TypeController;
// exports.LocalState = LocalState;
