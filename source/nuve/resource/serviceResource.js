/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
var logger = require('./../logger').logger;
var e = require('../errors');

// Logger
var log = logger.getLogger('ServiceResource');

/*
 * Gets the service and checks if it is superservice. Only superservice can do actions about services.
 */

var superService = global.config.nuve.superserviceID;

var doInit = function (currentService, serv, callback) {
    currentService._id = currentService._id + '';
    if (currentService._id !== superService) {
        callback('error');
    } else {
        dataAccess.service.get(serv, function (err, ser) {
            if (err) callback('error');
            callback(ser);
        });
    }
};

/*
 * Get Service. Represents a determined service.
 */
exports.represent = function (req, res, next) {
    var authData = req.authData || {};

    var curId = authData.service._id.toString();
    if (curId !== superService && curId !== req.params.service) {
        log.info('Service ', req.params.service, ' not authorized for this action');
        return next(new e.AccessError('Permission denied'));
    }

    dataAccess.service.get(req.params.service, function (err, ser) {
        if (err) {
            log.warn('Failed to get service:', err.message);
            return next(err);
        }
        log.info('Representing service ', ser._id);
        res.send(ser);
    });
};

/*
 * Delete Service. Removes a determined service from the data base.
 */
exports.deleteService = function (req, res, next) {
    var authData = req.authData || {};

    doInit(authData.service, req.params.service, function (serv) {
        if (serv === 'error') {
            log.info('Service ', req.params.service, ' not authorized for this action');
            return next(new e.AccessError('Permission denied'));
        }
        if (serv === undefined || serv === null) {
            return next(new e.NotFoundError('Service not found'));
        }
        var id = '';
        id += serv._id;
        if (id === superService) {
            res.status(401).send('Super service not permitted to be deleted');
            return next(new e.AccessError('Permission denied to delete super service'));
        }
        dataAccess.service.delete(id, function(err, result) {
            if (err) {
                log.warn('Failed to delete service:', err.message);
                return next(err);
            }
            log.info('Serveice ', id, ' deleted');
            res.send('Service deleted');
        });
    });
};
