// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('./logger').logger.getLogger('MyController');
const {DomainHandler, RemoteDomainHandler} = require('./domainHandler');

const STREAM_ENGINE_ID = 'stream-service';

class MyController {

  constructor(rpcId, rpcClient) {
    this.rpcId = rpcId;
    this.domain = null;
    // id => JoinData
    this.clients = new Map();
    // id => count
    this.portals = new Map();
    // id => stream
    this.streams = new Map();
  }
}

function RPCInterface(rpcClient, rpcID) {
  const controller = new DomainHandler();
  const API = {};
  for (const method of RemoteDomainHandler.supportedMethods) {
    API[method] = function (req, callback) {
      controller[method](req).then((ret) => {
        callback('callback', ret ? ret : 'ok');
      }).catch((e) => {
        callback('callback', 'error', e && e.message);
      });
    }
  }
  return API;
}

module.exports = RPCInterface;
