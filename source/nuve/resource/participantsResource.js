/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
var cloudHandler = require('../cloudHandler');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('ParticipantsResource');

/*
 * Gets the service and the room for the proccess of the request.
 */
var doInit = function (currentService, roomId, callback) {
    dataAccess.room.get(currentService._id, roomId, function (room) {
        if (room) {
            callback(room);
        } else {
            callback();
        }
    });
};

exports.getList = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    var currentService = authData.service;

    doInit(currentService, req.params.room, function (currentRoom) {
        if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' not found');
            res.status(404).send('Room not found');
            return;
        }

        log.debug('Representing participants for room ', currentRoom._id, 'and service', currentService._id);
        cloudHandler.getParticipantsInRoom (currentRoom._id, function (participants) {
            if (participants === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            res.send(participants);
        });
    });
};

exports.get = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    doInit(authData.service, req.params.room, function (currentRoom) {
        if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
            return;
        }

        var participant = req.params.participant;
        cloudHandler.getParticipantsInRoom(currentRoom._id, function (participants) {
            if (participants === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            for (var index in participants) {
                if (participants[index].id === participant) {
                    log.debug('Found participant', participant);
                    res.send(participants[index]);
                    return;
                }
            }
            log.error('Participant', req.params.participant, 'does not exist');
            res.status(404).send('Participant does not exist');
            return;
        });
    });
};

exports.patch = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    doInit(authData.service, req.params.room, function (currentRoom) {
        if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
            return;
        }

        var participant = req.params.participant;
        var updates = req.body;
        cloudHandler.updateParticipant(currentRoom._id, participant, updates, function (result) {
            if (result === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            res.send(result);
            return;
        });
    });
};

exports.delete = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    doInit(authData.service, req.params.room, function (currentRoom) {
        if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
            return;
        }

        var participant = req.params.participant;
        cloudHandler.deleteParticipant(currentRoom._id, participant, function (result) {
            log.debug('result', result);
            if (result === 'error') {
                res.status(404).send('Operation failed');
            } else {
                res.send();
            }
        });
    });
};
