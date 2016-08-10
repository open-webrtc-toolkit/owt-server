/*global exports, require*/
'use strict';
var serviceRegistry = require('./../mdb/serviceRegistry');
var cloudHandler = require('../cloudHandler');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('UserResource');

/*
 * Gets the service and the room for the proccess of the request.
 */
var doInit = function (currentService, roomId, callback) {
    serviceRegistry.getRoomForService(roomId, currentService, function (room) {
        callback(room);
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
        cloudHandler.getUsersInRoom(currentRoom._id, function (users) {
            if (users === 'error') {
                res.status(404).send('User does not exist');
                return;
            }
            for (var index in users) {
                if (users[index].name === user) {
                    log.info('Found user', user);
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
        cloudHandler.deleteUser(user, currentRoom._id, function (result) {
            if (typeof result === 'number') {
                if (result === 0) {
                    res.status(404).send('User does not exist');
                } else {
                    res.send(result + ' users deleted');
                }
            } else {
                res.status(404).send('Operation failed');
            }
        });
    });
};
