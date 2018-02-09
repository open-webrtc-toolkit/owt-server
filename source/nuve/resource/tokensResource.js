/*global exports, require, Buffer*/
'use strict';

var dataAccess = require('../data_access');
var crypto = require('crypto');
var cloudHandler = require('../cloudHandler');
var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('TokensResource');

/*
 * Gets the service and the room for the proccess of the request.
 */
var doInit = function (currentService, roomId, callback) {
    dataAccess.room.get(currentService._id, roomId, function (err, room) {
        if (room) {
            callback(room);
        } else {
            callback();
        }
    });
};

var getTokenString = function (id, token) {
    return dataAccess.token.key().then(function(nuveKey) {
        var toSign = id + ',' + token.host,
            hex = crypto.createHmac('sha256', nuveKey).update(toSign).digest('hex'),
            signed = (new Buffer(hex)).toString('base64'),

            tokenJ = {
                tokenId: id,
                host: token.host,
                secure: token.secure,
                signature: signed
            },
            tokenS = (new Buffer(JSON.stringify(tokenJ))).toString('base64');

        return tokenS;

    }).catch(function(err) {
        log.error('Get nuveKey error:', err);
    });
};

/*
 * Generates new token. 
 * The format of a token is:
 * {tokenId: id, host: erizoController host, signature: signature of the token};
 */
var generateToken = function (currentRoom, authData, origin, callback) {
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
    token.user = user;
    token.room = currentRoom._id;
    token.role = role;
    token.service = currentService._id;
    token.creationDate = new Date();
    token.origin = origin;
    token.code = Math.floor(Math.random() * 100000000000) + '';

    // Values to be filled from the erizoController
    token.secure = false;

    cloudHandler.schedulePortal (token.code, origin, function (ec) {
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

        dataAccess.token.create(token, function(id) {
            getTokenString(id, token)
                .then((tokenS) => {
                    callback(tokenS);
                });
        });
    });
};

/*
 * Post Token. Creates a new token for a determined room of a service.
 */
exports.create = function (req, res) {
    var authData = (req.authData || {});
      authData.user = (req.authData.user || (req.body && req.body.user));
      authData.role = (req.authData.role || (req.body && req.body.role));
    var origin = ((req.body && req.body.preference) || {isp: 'isp', region: 'region'});

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

        generateToken(currentRoom, authData, origin, function (tokenS) {
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
            log.debug('Created token for room ', currentRoom._id, 'and service ', authData.service._id);
            res.send(tokenS);
        });
    });
};

