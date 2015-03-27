/*global require, exports */
'use strict';
var superServiceId = require('./../mdb/dataBase').superService;
var serviceRegistry = require('./../mdb/serviceRegistry');
var mauthParser = require('./mauthParser');
var cipher = require('../../../common/cipher');
var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('NuveAuthenticator');

var cache = {};

var checkTimestamp = function (ser, params) {
    var serviceId = ser._id + '';
    if (serviceId === superServiceId) {
        // disable timestamp checking for superService;
        return true;
    }

    var lastParams = cache[serviceId],
        lastTS,
        newTS = (new Date(parseInt(params.timestamp, 10))).getTime(),
        lastC,
        newC = params.cnonce;

    if (isNaN(newTS)) {
        log.debug('Invalid timestamp:', params.timestamp);
        return false;
    }

    if (lastParams === undefined) {
        return true;
    }

    lastTS = (new Date(parseInt(lastParams.timestamp, 10))).getTime();
    lastC = lastParams.cnonce;

    if (newTS < lastTS || (lastTS === newTS && lastC === newC)) {
        log.info('Last timestamp: ', lastTS, ' and new: ', newTS);
        log.info('Last cnonce: ', lastC, ' and new: ', newC);
        return false;
    }

    return true;
};

var checkSignature = function (params, key) {
    if (params.signature_method !== 'HMAC_SHA256') {
        return false;
    }

    var calculatedSignature = mauthParser.calculateClientSignature(params, key);

    if (calculatedSignature !== params.signature) {
        return false;
    } else {
        return true;
    }
};

/*
 * This function has the logic needed for authenticate a nuve request. 
 * If the authentication success exports the service and the user and role (if needed). Else send back 
 * a response with an authentication request to the client.
 */
exports.authenticate = function (req, res, next) {
    var authHeader = req.header('Authorization'),
        challengeReq = 'MAuth realm="http://marte3.dit.upm.es"',
        params;

    if (authHeader !== undefined) {

        params = mauthParser.parseHeader(authHeader);

        // Get the service from the data base.
        serviceRegistry.getService(params.serviceid, function (serv) {
            if (serv === undefined || serv === null) {
                log.info('[Auth] Unknow service:', params.serviceid);
                res.status(401).send({'WWW-Authenticate': challengeReq});
                return;
            }

            var key = serv.key;
            if (serv.encrypted === true) {
                key = cipher.decrypt(cipher.k, key);
            }

            // Check if timestam and cnonce are valids in order to avoid duplicate requests.
            if (!checkTimestamp(serv, params)) {
                log.info('[Auth] Invalid timestamp or cnonce');
                res.status(401).send({'WWW-Authenticate': challengeReq});
                return;
            }

            // Check if the signature is valid.
            if (checkSignature(params, key)) {

                if (params.username !== undefined && params.role !== undefined) {
                    exports.user = (new Buffer(params.username, 'base64').toString('utf8'));
                    exports.role = params.role;
                }

                cache[serv._id+''] =  params;
                exports.service = serv;

                // If everything in the authentication is valid continue with the request.
                next();

            } else {
                log.info('[Auth] Wrong credentials');
                res.status(401).send({'WWW-Authenticate': challengeReq});
                return;
            }

        });

    } else {
        log.info('[Auth] MAuth header not presented');
        res.status(401).send({'WWW-Authenticate': challengeReq});
        return;
    }
};