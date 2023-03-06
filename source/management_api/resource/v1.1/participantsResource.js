// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
const { query } = require('express');
const e = require('../../errors');

// Logger
const log = require('../../logger').logger.getLogger('ParticipantsResource');
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
  log.debug('Representing participants for service ', req.authData.service._id);
  const query = req.body?.query || {};
  callStreamService('getParticipants', [{query}], (err, rets) => {
    if (err) {
      next(new e.CloudError('Failed to get participants'));
    } else {
      res.send(rets);
    }
  });
};

exports.get = function (req, res, next) {
  log.debug('Representing publication:', req.params.participant);
  const query = {id: req.params.participant};
  callStreamService('getParticipants', [{query}], (err, rets) => {
    if (err) {
      next(new e.CloudError('Failed to get participants'));
    } else {
      res.send(rets[0]);
    }
  });
};

exports.add = function (req, res, next) {
  log.debug('Join for service ', req.authData.service._id);
  const data = {
    id: req.body?.id,
    domain: req.body?.domain,
    portal: req.body?.portal,
    notifying: req.body?.notifying
  };
  callStreamService('join', [data], (err, ret) => {
    if (err) {
      next(new e.CloudError('Failed to join'));
    } else {
      res.send(ret);
    }
  });
};

exports.delete = function (req, res, next) {
  log.debug('Leave for service ', req.params.participant, req.authData.service._id);
  callStreamService('leave', [{id: req.params.participant}], (err, ret) => {
    if (err) {
      next(new e.CloudError('Failed to leave'));
    } else {
      res.send(ret);
    }
  });
};
