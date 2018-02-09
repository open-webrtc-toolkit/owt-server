/*global exports, require*/
'use strict';

var dataAccess = require('../data_access');
var logger = require('./../logger').logger;
var cipher = require('../cipher');

// Logger
var log = logger.getLogger('ServicesResource');

/*
 * Gets the service and checks if it is superservice. Only superservice can do actions about services.
 */
var doInit = function (currentService) {
    var superService = global.config.nuve.superserviceID;
    currentService._id = currentService._id + '';
    return (currentService._id === superService);
};

/*
 * Post Service. Creates a new service.
 */
exports.create = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    if (!doInit(authData.service)) {
        log.info('Service ', authData.service._id, ' not authorized for this action');
        res.status(401).send('Service not authorized for this action');
        return;
    }
    var service = req.body;

    // Check the request body as service
    if (typeof service.name !== 'string' || typeof service.key !== 'string') {
        res.status(401).send('Service name and key do not have string type');
        return;
    }

    service.encrypted = true;
    service.key = cipher.encrypt(cipher.k, service.key);
    dataAccess.service.create(service, function(err, result) {
        if (err) {
            log.warn('Failed to create service:', err.message);
            res.status(401).send('Operation failed');
            return;
        }
        log.info('Service created: ', service.name);
        res.send(result);
    });
};

/*
 * Get Service. Represents a determined service.
 */
exports.represent = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    if (!doInit(authData.service)) {
        log.info('Service ', authData.service, ' not authorized for this action');
        res.status(401).send('Service not authorized for this action');
        return;
    }

    dataAccess.service.list(function(err, list) {
        if (err) {
            log.warn('Failed to list services:', err.message);
            res.status(401).send('Operation failed');
            return;
        }
        log.info('Representing services');
        res.send(list);
    });
};
