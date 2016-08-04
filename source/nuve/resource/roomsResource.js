/*global exports, require*/
'use strict';
var roomRegistry = require('./../mdb/roomRegistry');
var serviceRegistry = require('./../mdb/serviceRegistry');
var Room = require('./room');
var logger = require('./../logger').logger;
var cloudHandler = require('../cloudHandler');

// Logger
var log = logger.getLogger('RoomsResource');

/*
 * Post Room. Creates a new room for a determined service.
 */
exports.createRoom = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }
    var currentService = authData.service;

    if (currentService === undefined) {
        res.status(404).send('Service not found');
        return;
    }
    if (typeof req.body !== 'object' || req.body === null || typeof req.body.name !== 'string' || req.body.name === '') {
        log.info('Invalid room');
        res.status(400).send('Invalid room');
        return;
    }

    if (req.body.options && typeof req.body.options !== 'object') {
        log.info('Invalid room option');
        res.status(400).send('Invalid room option');
        return;
    }

    req.body.options = req.body.options || {};
    var room;

    if (req.body.options.test) {
        if (currentService.testRoom !== undefined) {
            log.info('TestRoom already exists for service', currentService.name);
            res.send(currentService.testRoom);
        } else {
            room = {name: 'testRoom'};
            roomRegistry.addRoom(room, function (result) {
                currentService.testRoom = result;
                currentService.rooms.push(result);
                serviceRegistry.updateService(currentService);
                log.info('TestRoom created for service', currentService.name);
                res.send(result);

                // Notify SIP portal if SIP room created
                if (result.sipInfo) {
                    log.info('Notify SIP Portal on create Room');
                    cloudHandler.notifySipPortal('create', result, function(){});
                }
            });
        }
    } else {
        var options = req.body.options;
        options.name = req.body.name;
        room = Room.create(options);
        if (room === null) {
            return res.status(400).send('Bad room configuration');
        }
        roomRegistry.addRoom(room, function (result) {
            currentService.rooms.push(result);
            serviceRegistry.updateService(currentService, function () {
                log.info('Room created:', req.body.name, 'for service', currentService.name);
                res.send(result);
            }, function (reason) {
                res.status(400).send(reason);
            });

            // Notify SIP portal if SIP room created
            if (result.sipInfo) {
                log.info('Notify SIP Portal on create Room');
                cloudHandler.notifySipPortal('create', result, function(){});
            }
        });
    }
};

/*
 * Get Rooms. Represent a list of rooms for a determined service.
 */
exports.represent = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }
    var currentService = authData.service;

    if (currentService === undefined) {
        res.status(404).send('Service not found');
        return;
    }
    log.info('Representing rooms for service ', currentService._id);
    res.send(currentService.rooms);
};
