/*global exports, require*/
'use strict';
var roomRegistry = require('./../mdb/roomRegistry');
var serviceRegistry = require('./../mdb/serviceRegistry');
var Room = require('./room');
var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('RoomsResource');

var currentService;

/*
 * Gets the service for the proccess of the request.
 */
var doInit = function () {
    currentService = require('./../auth/nuveAuthenticator').service;
};

/*
 * Post Room. Creates a new room for a determined service.
 */
exports.createRoom = function (req, res) {
    doInit();
    if (currentService === undefined) {
        res.status(404).send('Service not found');
        return;
    }
    if (typeof req.body !== 'object' || req.body === null || typeof req.body.name !== 'string' || req.body.name === '') {
        log.info('Invalid room');
        res.status(400).send('Invalid room');
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
            serviceRegistry.updateService(currentService);
            log.info('Room created:', req.body.name, 'for service', currentService.name);
            res.send(result);
        });
    }
};

/*
 * Get Rooms. Represent a list of rooms for a determined service.
 */
exports.represent = function (req, res) {
    doInit();
    if (currentService === undefined) {
        res.status(404).send('Service not found');
        return;
    }
    log.info('Representing rooms for service ', currentService._id);
    res.send(currentService.rooms);
};
