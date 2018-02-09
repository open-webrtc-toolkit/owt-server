/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
var logger = require('./../logger').logger;

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
            if (err) {
                log.warn('Failed to get service:', err.message);
                return callback(null);
            }
            callback(ser);
        });
    }
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

    doInit(authData.service, req.params.service, function (serv) {
        if (serv === 'error') {
            log.info('Service ', req.params.service, ' not authorized for this action');
            res.status(401).send('Service not authorized for this action');
            return;
        }
        if (serv === undefined || serv === null) {
            res.status(404).send('Service not found');
            return;
        }
        log.info('Representing service ', serv._id);
        res.send(serv);
    });
};

/*
 * Delete Service. Removes a determined service from the data base.
 */
exports.deleteService = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    doInit(authData.service, req.params.service, function (serv) {
        if (serv === 'error') {
            log.info('Service ', req.params.service, ' not authorized for this action');
            res.status(401).send('Service not authorized for this action');
            return;
        }
        if (serv === undefined || serv === null) {
            res.status(404).send('Service not found');
            return;
        }
        var id = '';
        id += serv._id;
        if (id === superService) {
            res.status(401).send('Super service not permitted to be deleted');
            return;
        }
        dataAccess.service.delete(id, function(err, result) {
            if (err) log.warn('Failed to delete service:', err.message);
            log.info('Serveice ', id, ' deleted');
            res.send('Service deleted');
        });
    });
};
