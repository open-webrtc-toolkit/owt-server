/*global exports, require, console*/
var roomRegistry = require('./../mdb/roomRegistry');
var serviceRegistry = require('./../mdb/serviceRegistry');
var cloudHandler = require('../cloudHandler');
var Room = require('./room');
var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger("RoomResource");

var currentService;

/*
 * Gets the service and the room for the proccess of the request.
 */
var doInit = function (roomId, callback) {
    "use strict";

    currentService = require('./../auth/nuveAuthenticator').service;

    serviceRegistry.getRoomForService(roomId, currentService, callback);
};

/*
 * Get Room. Represents a determined room.
 */
exports.represent = function (req, res) {
    "use strict";

    doInit(req.params.room, function (room) {
        if (currentService === undefined) {
            res.status(401).send('Client unathorized');
        } else if (room === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
        } else {
            log.info('Representing room ', room._id, 'of service ', currentService._id);
            res.send(room);
        }
    });
};

/*
 * Delete Room. Removes a determined room from the data base and asks cloudHandler to remove it from erizoController.
 */
exports.deleteRoom = function (req, res) {
    "use strict";

    doInit(req.params.room, function (room) {
        if (currentService === undefined) {
            res.status(401).send('Client unathorized');
        } else if (room === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
        } else {
            log.info('Preparing deleting room', room._id);
            var id = room._id + '';
            roomRegistry.removeRoom(id);
            currentService.rooms.map(function (r, index) {
                if (r._id === room._id) {
                    currentService.rooms.splice(index, 1);
                    serviceRegistry.updateService(currentService);
                    log.info('Room ', id, ' deleted for service ', currentService._id);
                    cloudHandler.deleteRoom(id, function () {});
                }
            });
            res.send('Room deleted');
        }
    });
};

exports.updateRoom = function (req, res) {
    'use strict';
    doInit(req.params.room, function (room) {
        if (currentService === undefined) {
            res.status(401).send('Client unathorized');
        } else if (room === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
        } else {
            var updates = req.body || {};
            var newRoom = Room.create(updates);
            if (newRoom.validate()) {
                var currentRoom = Room.create(room);
                Object.keys(newRoom).map(function (k) {
                    room[k] = newRoom[k] || currentRoom[k];
                });
                roomRegistry.addRoom(room, function (result) {
                    serviceRegistry.updateService(currentService);
                    if (result == 1) {
                      res.send(room);
                    } else {
                      res.send(result);
                    }
                });
            } else {
                res.send(room);
            }
        }
    });
};
