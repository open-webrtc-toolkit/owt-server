/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
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

    var options = req.body.options;
    options.name = req.body.name;
    dataAccess.room.create(currentService._id, options, function(result) {
        if (result) {
            log.debug('Room created:', req.body.name, 'for service', currentService.name);
            res.send(result);

            // Notify SIP portal if SIP room created
            if (result && result.sipInfo) {
                log.info('Notify SIP Portal on create Room');
                cloudHandler.notifySipPortal('create', result, function(){});
            }
        } else {
            log.info('Room creation failed', options);
            res.status(400).send();
        }
    });
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

    dataAccess.room.list(currentService._id, function (rooms) {
        if (rooms) {
            log.debug('Representing rooms for service ', currentService._id);
            res.send(currentService.rooms);
        } else {
            log.info('Failed to representing rooms');
            res.status(400).send();
        }
    });
};
