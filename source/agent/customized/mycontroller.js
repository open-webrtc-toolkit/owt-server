// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('./logger').logger.getLogger('MyController');
const {Conference} = require('./conferenceLite');


function RPCInterface(rpcClient, rpcID) {
  const controller = new Conference(rpcClient, rpcID);
  const API = {};
  for (const method of Conference.supportedMethods) {
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
