/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
var cloudHandler = require('../cloudHandler');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('UsersResource');

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

/*
 * Get Users. Represent a list of users of a determined room. This is consulted to cloudHandler.
 */
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

        log.info('Representing users for room ', currentRoom._id, 'and service', currentService._id);
        cloudHandler.getParticipantsInRoom (currentRoom._id + '', function (users) {
            if (users === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            res.send(users);
        });
    });
};
