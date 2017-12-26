/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
var cloudHandler = require('../cloudHandler');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('RecordingsResource');

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

        log.debug('Representing recordings for room ', currentRoom._id, 'and service', currentService._id);
        cloudHandler.getSubscriptionsInRoom (currentRoom._id, 'recording', function (recordings) {
            if (recordings === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            res.send(recordings);
        });
    });
};

exports.add = function (req, res) {
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

        var sub_req = {
          type: 'recording',
          connection: {
            container: req.body.container
          },
          media: req.body.media
        };
        cloudHandler.addServerSideSubscription(currentRoom._id, sub_req, function (result) {
            if (result === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            res.send(result);
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

        var sub_id = req.params.id,
            cmds = req.body;
        cloudHandler.controlSubscription(currentRoom._id, sub_id, cmds, function (result) {
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

        var sub_id = req.params.id;
        cloudHandler.deleteSubscription(currentRoom._id, sub_id, function (result) {
            log.debug('result', result);
            if (result === 'error') {
                res.status(404).send('Operation failed');
            } else {
                res.send();
            }
        });
    });
};
