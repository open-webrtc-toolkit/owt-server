// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
const requestHandler = require('../../requestHandler');
const e = require('../../errors');

// Logger
const log = require('../../logger').logger.getLogger('ProcessorsResource');

const rpc = require('../../rpc/rpc');

const STREAM_SERVICE_ID = global.config.cluster.stream_engine;

function callStreamService(methodName, args, callback) {
  rpc.callRpc(STREAM_SERVICE_ID, methodName, args, {callback: function(ret) {
    if (ret === 'timeout' || ret === 'error') {
      callback(ret);
    } else {
      callback(null, ret);
    }
  }});
}

exports.getList = function (req, res, next) {
  log.debug('Representing processors for domain ', req.params.domain,
            'and service', req.authData.service._id);
  callStreamService('getProcessors', [{}], (err, pubs) => {
    if (err) {
      next(new e.CloudError('Failed to get subscriptions'));
    } else {
      res.send(pubs);
    }
  });
};

exports.get = function (req, res, next) {
  log.debug('Representing processor:', req.params.processor,
            ' for domain ', req.params.domain);
  const query = {id: req.params.processor};
  callStreamService('getProcessors', [query], (err, pubs) => {
    if (err) {
      next(new e.CloudError('Failed to get processors'));
    } else {
      res.send(pubs[0]);
    }
  });
};

exports.add = function (req, res, next) {
  log.debug('Add processor for domain ', req.params.domain);
  callStreamService('addProcessor', [req.body], (err, ret) => {
    if (err) {
      log.debug('Add failure:', err, err.stack);
      next(new e.CloudError('Failed to add processor'));
    } else {
      res.send(ret);
    }
  });
};

exports.delete = function (req, res, next) {
  log.debug('Delete processor:', req.params.processor,
            ' for domain ', req.params.domain);
  callStreamService('removeProcessor', [{id: req.params.processor}], (err, ret) => {
    if (err) {
      next(new e.CloudError('Failed to remove processor'));
    } else {
      res.send(ret);
    }
  });
};
