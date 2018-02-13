/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
var cloudHandler = require('../cloudHandler');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('UserResource');

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
 * Get User. Represent a determined user of a room. This is consulted to erizoController using RabbitMQ RPC call.
 */
exports.getUser = function (req, res) {
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

        var user = req.params.user;
        cloudHandler.getParticipantsInRoom(currentRoom._id + '', function (users) {
            if (users === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            for (var index in users) {
                if (users[index].id === user) {
                    log.debug('Found user', user);
                    res.send(users[index]);
                    return;
                }
            }
            log.error('User', req.params.user, 'does not exist');
            res.status(404).send('User does not exist');
            return;
        });
    });
};

/*
 * Delete User. Removes a determined user from a room. This order is sent to erizoController using RabbitMQ RPC call.
 */
exports.deleteUser = function (req, res) {
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

        var user = req.params.user;
        cloudHandler.deleteParticipant(currentRoom._id + '', user, function (result) {
            log.debug('result', result);
            if (result === 'error') {
                res.status(404).send('Operation failed');
            } else {
                res.send('User deleted');
            }
        });
    });
};
