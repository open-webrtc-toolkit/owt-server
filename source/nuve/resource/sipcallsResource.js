'use strict';
var dataAccess = require('../data_access');
var cloudHandler = require('../cloudHandler');
var e = require('../errors');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('SipCallsResource');

exports.getList = function (req, res, next) {
    log.debug('Representing sip calls for room ', req.params.room, 'and service', req.authData.service._id);
    cloudHandler.getSipCallsInRoom (req.params.room, function (sipcalls) {
        if (sipcalls === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(sipcalls);
    });
};

exports.add = function (req, res, next) {
    log.debug('addSipCall for room', req.params.room);
    cloudHandler.addSipCall(req.params.room, req.body, function (result, err) {
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
    cloudHandler.updateSipCall(req.params.room, sub_id, cmds, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
        return;
    });
};

exports.delete = function (req, res, next) {
    var sub_id = req.params.id;
    cloudHandler.deleteSipCall(req.params.room, sub_id, function (result) {
        log.debug('result', result);
        if (result === 'error') {
            next(new e.CloudError('Operation failed'));
        } else {
            res.send();
        }
    });
};
