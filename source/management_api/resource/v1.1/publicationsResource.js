// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
const e = require('../../errors');

// Logger
const log = require('../../logger').logger.getLogger('PublicationsResource');
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
  log.debug('Representing publications for service', req.authData.service._id);
  const query = req.body?.query || {};
  callStreamService('getPublications', [{query}], (err, pubs) => {
    if (err) {
      next(new e.CloudError('Failed to get publications'));
    } else {
      res.send(pubs);
    }
  });
};

exports.get = function (req, res, next) {
  log.debug('Representing publication:', req.params.publication,
            ' for domain ', req.params.domain);
  const query = {id: req.params.publication};
  callStreamService('getPublications', [{query}], (err, pubs) => {
    if (err) {
      next(new e.CloudError('Failed to get publications'));
    } else {
      res.send(pubs[0]);
    }
  });
};

exports.add = function (req, res, next) {
  log.debug('Add publication for domain ', req.params.domain);
  callStreamService('publish', [req.body], (err, ret) => {
    if (err) {
      next(new e.CloudError('Failed to publish'));
    } else {
      res.send(ret);
    }
  });
};

exports.patch = function (req, res, next) {
  log.debug('Update publication:', req.params.publication,
            ' for domain ', req.params.domain);
  const stream = req.params.stream;
  const cmds = req.body;
  next(new e.CloudError('No implementation'));
};

exports.delete = function (req, res, next) {
  log.debug('Delete publication:', req.params.publication,
            ' for domain ', req.params.domain);
  callStreamService('unpublish', [{id: req.params.publication}], (err, ret) => {
    if (err) {
      next(new e.CloudError('Failed to unpublish'));
    } else {
      res.send(ret);
    }
  });
};

exports.getNodes = function (req, res, next) {
  log.debug('Representing publications for domain ', req.params.domain,
            'and service', req.authData.service._id);
  callStreamService('getNodes', [{}], (err, pubs) => {
    if (err) {
      next(new e.CloudError('Failed to get nodes'));
    } else {
      res.send(pubs);
    }
  });
}
