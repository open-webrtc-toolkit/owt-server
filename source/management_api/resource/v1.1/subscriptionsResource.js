// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
const requestHandler = require('../../requestHandler');
const e = require('../../errors');

// Logger
const log = require('../../logger').logger.getLogger('SubscriptionsResource');

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
  log.debug('Representing subscriptions for domain ', req.params.domain,
            'and service', req.authData.service._id);
  callStreamService('getSubscriptions', [{}], (err, pubs) => {
    if (err) {
      next(new e.CloudError('Failed to get subscriptions'));
    } else {
      res.send(pubs);
    }
  });
};

exports.get = function (req, res, next) {
  log.debug('Representing subscription:', req.params.subscription,
            ' for domain ', req.params.domain);
  const query = {id: req.params.subscription};
  callStreamService('getSubscriptions', [query], (err, pubs) => {
    if (err) {
      next(new e.CloudError('Failed to get subscriptions'));
    } else {
      res.send(pubs[0]);
    }
  });
};

exports.add = function (req, res, next) {
  log.debug('Add subscription for domain ', req.params.domain);
  callStreamService('subscribe', [req.body], (err, ret) => {
    if (err) {
      log.debug('Add failure:', err, err.stack);
      next(new e.CloudError('Failed to subscribe'));
    } else {
      res.send(ret);
    }
  });
};

exports.patch = function (req, res, next) {
  log.debug('Update subscription:', req.params.subscription,
            ' for domain ', req.params.domain);
  const stream = req.params.stream;
  const cmds = req.body;
  next(new e.CloudError('No implementation'));
};

exports.delete = function (req, res, next) {
  log.debug('Delete subscription:', req.params.subscription,
            ' for domain ', req.params.domain);
  callStreamService('unsubscribe', [{id: req.params.subscription}], (err, ret) => {
    if (err) {
      next(new e.CloudError('Failed to unsubscribe'));
    } else {
      res.send(ret);
    }
  });
};
