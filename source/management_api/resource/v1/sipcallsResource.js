// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var dataAccess = require('../../data_access');
var requestHandler = require('../../requestHandler');
var e = require('../../errors');

var logger = require('../../logger').logger;

// Logger
var log = logger.getLogger('SipCallsResource');

exports.getList = function (req, res, next) {
    log.debug('Representing sip calls for room ', req.params.room, 'and service', req.authData.service._id);
    requestHandler.getSipCallsInRoom (req.params.room, function (sipcalls) {
        if (sipcalls === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(sipcalls);
    });
};

exports.add = function (req, res, next) {
    log.debug('addSipCall for room', req.params.room);
    requestHandler.addSipCall(req.params.room, req.body, function (result, err) {
        if (result === 'error') {
            return next(err);
        }
        res.send(result);
        return;
    });
};

exports.patch = function (req, res, next) {
    var sub_id = req.params.id,
        cmds = req.body;
    requestHandler.updateSipCall(req.params.room, sub_id, cmds, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
        return;
    });
};

exports.delete = function (req, res, next) {
    var sub_id = req.params.id;
    requestHandler.deleteSipCall(req.params.room, sub_id, function (result) {
        log.debug('result', result);
        if (result === 'error') {
            next(new e.CloudError('Operation failed'));
        } else {
            res.send();
        }
    });
};
