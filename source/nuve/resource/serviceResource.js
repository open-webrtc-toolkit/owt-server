/*global exports, require*/
'use strict';
var serviceRegistry = require('./../mdb/serviceRegistry');
var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('ServiceResource');

/*
 * Gets the service and checks if it is superservice. Only superservice can do actions about services.
 */

var superService = require('./../mdb/dataBase').superService;

var doInit = function (serv, callback) {
    var service = require('./../auth/nuveAuthenticator').service;
    service._id = service._id + '';
    if (service._id !== superService) {
        callback('error');
    } else {
        serviceRegistry.getService(serv, function (ser) {
            callback(ser);
        });
    }
};

/*
 * Get Service. Represents a determined service.
 */
exports.represent = function (req, res) {
    doInit(req.params.service, function (serv) {
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
    doInit(req.params.service, function (serv) {
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
        serviceRegistry.removeService(id);
        log.info('Serveice ', id, ' deleted');
        res.send('Service deleted');
    });
};
