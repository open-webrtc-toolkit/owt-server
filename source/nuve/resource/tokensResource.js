/*global exports, require, Buffer*/
'use strict';
var tokenRegistry = require('./../mdb/tokenRegistry');
var serviceRegistry = require('./../mdb/serviceRegistry');
var dataBase = require('./../mdb/dataBase');
var crypto = require('crypto');
var cloudHandler = require('../cloudHandler');
var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('TokensResource');

/*
 * Gets the service and the room for the proccess of the request.
 */
var doInit = function (currentService, roomId, callback) {
    serviceRegistry.getRoomForService(roomId, currentService, function (room) {
        //log.info(room);
        callback(room);
    });
};

var getTokenString = function (id, token) {
    var toSign = id + ',' + token.host,
        hex = crypto.createHmac('sha256', dataBase.nuveKey).update(toSign).digest('hex'),
        signed = (new Buffer(hex)).toString('base64'),

        tokenJ = {
            tokenId: id,
            host: token.host,
            secure: token.secure,
            signature: signed
        },
        tokenS = (new Buffer(JSON.stringify(tokenJ))).toString('base64');

    return tokenS;
};

/*
 * Generates new token. 
 * The format of a token is:
 * {tokenId: id, host: erizoController host, signature: signature of the token};
 */
var generateToken = function (currentRoom, authData, callback) {
    var currentService = authData.service,
        user = authData.user,
        role = authData.role,
        r,
        tr,
        token,
        tokenS;

    if (user === undefined || user === '') {
        callback(undefined);
        return;
    }

    token = {};
    token.userName = user;
    token.room = currentRoom._id;
    token.role = role;
    token.service = currentService._id;
    token.creationDate = new Date();

    // Values to be filled from the erizoController
    token.secure = false;

    r = currentRoom._id;
    tr = undefined;

    if (currentService.testRoom !== undefined) {
        tr = currentService.testRoom._id;
    }

    if (tr === r) {
        if (currentService.testToken === undefined) {
            token.use = 0;
            token.host = dataBase.testErizoController;

            log.info('Creating testToken');

            tokenRegistry.addToken(token, function (id) {
                token._id = id;
                currentService.testToken = token;
                serviceRegistry.updateService(currentService);
                tokenS = getTokenString(id, token);
                callback(tokenS);
                return;
            });
        } else {
            token = currentService.testToken;
            log.info('TestToken already exists, sending it', token);
            tokenS = getTokenString(token._id, token);
            callback(tokenS);
            return;
        }
    } else {

        cloudHandler.getPortalForRoom (currentRoom, function (ec) {

            if (ec === 'timeout') {
                callback('error');
                return;
            }

            token.secure = ec.ssl;
            if (ec.hostname !== '') {
                token.host = ec.hostname;
            } else {
                token.host = ec.ip;
            }

            token.host += ':' + ec.port;

            tokenRegistry.addToken(token, function (id) {
                var tokenS = getTokenString(id, token);
                callback(tokenS);
            });
        });
    }
};

/*
 * Post Token. Creates a new token for a determined room of a service.
 */
exports.create = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    doInit(authData.service, req.params.room, function (currentRoom) {
         if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
            return;
        }

        generateToken(currentRoom, authData, function (tokenS) {
            if (tokenS === undefined) {
                log.info('Name and role?');
                res.status(401).send('Name and role?');
                return;
            }
            if (tokenS === 'error') {
                log.info('CloudHandler does not respond');
                res.status(401).send('CloudHandler does not respond');
                return;
            }
            log.info('Created token for room ', currentRoom._id, 'and service ', authData.service._id);
            res.send(tokenS);
        });
    });
};
