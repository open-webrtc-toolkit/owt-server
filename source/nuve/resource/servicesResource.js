'use strict';

var dataAccess = require('../data_access');
var logger = require('./../logger').logger;
var cipher = require('../cipher');
var e = require('../errors');

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
exports.create = function (req, res, next) {
    var authData = req.authData || {};
    if (!doInit(authData.service)) {
        log.info('Service ', authData.service._id, ' not authorized for this action');
        return next(new e.AccessError('Permission denied'));
    }
    var service = req.body;

    // Check the request body as service
    if (typeof service.name !== 'string' || typeof service.key !== 'string') {
        return next(new e.BadRequestError('Service name and key do not have string type'));
    }

    service.encrypted = true;
    service.key = cipher.encrypt(cipher.k, service.key);
    dataAccess.service.create(service, function(err, result) {
        if (err) {
            log.warn('Failed to create service:', err.message);
            return next(err);
        }
        log.info('Service created: ', service.name);
        res.send(result);
    });
};

/*
 * Get Service. Represents a determined service.
 */
exports.represent = function (req, res, next) {
    var authData = req.authData || {};

    if (!doInit(authData.service)) {
        log.info('Service ', authData.service, ' not authorized for this action');
        return next(new e.AccessError('Permission denied'));
    }

    dataAccess.service.list(function(err, list) {
        if (err) {
            log.warn('Failed to list services:', err.message);
            return next(err);
        }
        log.info('Representing services');
        res.send(list);
    });
};
