/*global exports, require*/
'use strict';

var serviceRegistry = require('./../mdb/serviceRegistry');
var logger = require('./../logger').logger;
var cipher = require('../../../common/cipher');

// Logger
var log = logger.getLogger('ServicesResource');

var currentService;

/*
 * Gets the service and checks if it is superservice. Only superservice can do actions about services.
 */
var doInit = function () {
    currentService = require('./../auth/nuveAuthenticator').service;
    var superService = require('./../mdb/dataBase').superService;
    currentService._id = currentService._id + '';
    return (currentService._id === superService);
};

/*
 * Post Service. Creates a new service.
 */
exports.create = function (req, res) {
    if (!doInit()) {
        log.info('Service ', currentService._id, ' not authorized for this action');
        res.status(401).send('Service not authorized for this action');
        return;
    }
    var service = req.body;
    service.encrypted = true;
    service.key = cipher.encrypt(cipher.k, service.key);
    serviceRegistry.addService(service, function (result) {
        log.info('Service created: ', service.name);
        res.send(result);
    });
};

/*
 * Get Service. Represents a determined service.
 */
exports.represent = function (req, res) {
    if (!doInit()) {
        log.info('Service ', currentService, ' not authorized for this action');
        res.status(401).send('Service not authorized for this action');
        return;
    }

    serviceRegistry.getList(function (list) {
        log.info('Representing services');
        res.send(list);
    });
};