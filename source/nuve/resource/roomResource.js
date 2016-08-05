/*global exports, require*/
'use strict';
var roomRegistry = require('./../mdb/roomRegistry');
var serviceRegistry = require('./../mdb/serviceRegistry');
var cloudHandler = require('../cloudHandler');
var Room = require('./room');
var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('RoomResource');

/*
 * Gets the service and the room for the proccess of the request.
 */
var doInit = function (currentService, roomId, callback) {
    serviceRegistry.getRoomForService(roomId, currentService, callback);
};

/*
 * Get Room. Represents a determined room.
 */
exports.represent = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(401).send('Client unathorized');
        return;
    }

    doInit(authData.service, req.params.room, function (room) {
        if (room === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
        } else {
            log.info('Representing room ', room._id, 'of service ', authData.service._id);
            res.send(room);
        }
    });
};

/*
 * Delete Room. Removes a determined room from the data base and asks cloudHandler to remove it from erizoController.
 */
exports.deleteRoom = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(401).send('Client unathorized');
        return;
    }
    var currentService = authData.service;

    doInit(currentService, req.params.room, function (room) {
        if (room === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
        } else {
            log.info('Preparing deleting room', room._id);
            var id = room._id + '';
            roomRegistry.removeRoom(id, function (removed) {
                if (!removed) {
                    log.info('Room ', req.params.room, ' does not exist');
                    res.status(404).send('Room does not exist');
                } else {
                    var found = false;
                    currentService.rooms.map(function (r, index) {
                        if (r._id === room._id) {
                            found = true;
                            currentService.rooms.splice(index, 1);
                            serviceRegistry.updateService(currentService, function () {
                                log.info('Room ', id, ' deleted for service ', currentService._id);
                                cloudHandler.deleteRoom(id, function () {});
                                res.send('Room deleted');
                            }, function () {
                                log.info('Room ', req.params.room, ' does not exist');
                                res.status(404).send('Room does not exist');
                            });

                            // Notify SIP portal if SIP room deleted
                            if (room.sipInfo) {
                                log.info('Notify SIP Portal on delete Room');
                                cloudHandler.notifySipPortal('delete', room, function(){});
                            }
                        }
                    });

                    if (!found) {
                        log.info('Room ', req.params.room, ' does not exist');
                        res.status(404).send('Room does not exist');
                    }
                }
            });
        }
    });
};

exports.updateRoom = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(401).send('Client unathorized');
        return;
    }
    var currentService = authData.service;

    doInit(currentService, req.params.room, function (room) {
        if (currentService === undefined) {
            res.status(401).send('Client unathorized');
        } else if (room === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
        } else {
            var updates = req.body;
            if (typeof updates === 'object' && updates !== null) {
                log.info('Room', 'updateRoom updates', updates);
                var newRoom = Room.create(updates);
                if (newRoom === null) {
                    return res.status(400).send('Bad room configuration');
                }

                var hasSip = !!room.sipInfo;
                Object.keys(updates).map(function (k) {
                    if (newRoom.hasOwnProperty(k)) {
                        if (k !== 'mediaMixing') {
                            room[k] = newRoom[k];
                        } else if (typeof updates.mediaMixing.video === 'object') {
                            room.mediaMixing = room.mediaMixing || {};
                            room.mediaMixing.video = room.mediaMixing.video || {};
                            Object.keys(updates.mediaMixing.video).map(function (k) {
                                if (newRoom.mediaMixing.video.hasOwnProperty(k)) {
                                    if (k !== 'layout') {
                                        room.mediaMixing.video[k] = newRoom.mediaMixing.video[k];
                                    } else if (typeof updates.mediaMixing.video.layout === 'object') {
                                        room.mediaMixing.video.layout = room.mediaMixing.video.layout || {};
                                        Object.keys(updates.mediaMixing.video.layout).map(function (k) {
                                            if (newRoom.mediaMixing.video.layout.hasOwnProperty(k)) {
                                                room.mediaMixing.video.layout[k] = newRoom.mediaMixing.video.layout[k];
                                            }
                                        });
                                    }
                                }
                            });
                        }
                    }
                });

                roomRegistry.addRoom(room, function (result) {
                    currentService.rooms.map(function (r, index) {
                        if (r._id === room._id) {
                            currentService.rooms.splice(index, 1, room);
                            serviceRegistry.updateService(currentService);
                        }
                    });
                    if (result == 1) {
                        res.send(room);
                    } else {
                        // Notify SIP portal if SIP room updated
                        if (updates.hasOwnProperty('sipInfo')) {
                            log.info('Notify SIP Portal on update Room', updates);
                            var changeType = 'update';
                            if (!hasSip && result.sipInfo) {
                                changeType = 'create';
                            } else if (hasSip && !result.sipInfo) {
                                changeType = 'delete';
                            }
                            log.info('Change type', changeType);
                            cloudHandler.notifySipPortal(changeType, result, function(){});
                        }
                        res.send(result);
                    }
                });

            } else {
                res.status(400).send('Bad room configuration');
            }
        }
    });
};
