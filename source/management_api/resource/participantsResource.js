// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var dataAccess = require('../data_access');
var requestHandler = require('../requestHandler');
var e = require('../errors');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('ParticipantsResource');

exports.getList = function (req, res, next) {
    log.debug('Representing participants for room ', req.params.room, 'and service', req.authData.service._id);
    requestHandler.getParticipantsInRoom (req.params.room, function (participants) {
        if (participants === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(participants);
    });
};

exports.get = function (req, res, next) {
    var participant = req.params.participant;
    requestHandler.getParticipantsInRoom(req.params.room, function (participants) {
        if (participants === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        for (var index in participants) {
            if (participants[index].id === participant) {
                log.debug('Found participant', participant);
                res.send(participants[index]);
                return;
            }
        }
        log.error('Participant', req.params.participant, 'does not exist');
        next(new e.NotFoundError('Participant not found'));
    });
};

exports.patch = function (req, res, next) {
    var participant = req.params.participant;
    var updates = req.body;
    requestHandler.updateParticipant(req.params.room, participant, updates, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
    });
};

exports.delete = function (req, res, next) {
    var participant = req.params.participant;
    requestHandler.deleteParticipant(req.params.room, participant, function (result) {
        log.debug('result', result);
        if (result === 'error') {
            next(new e.CloudError('Operation failed'));
        } else {
            res.send();
        }
    });
};
