// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

'use strict';
var dataAccess = require('../data_access');
var mauthParser = require('./mauthParser');
var cipher = require('../cipher');
var log = require('./../logger').logger.getLogger('ServerAuthenticator');
var e = require('../errors');

var cache = {};

var checkTimestamp = function (ser, params) {
    if ((global.config||{}).timestampCheck !== true) {
        return true;
    }
    var serviceId = ser._id + '';
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
 * This function has the logic needed for authenticate a request.
 * If the authentication success exports the service and the user and role (if needed). Else send back
 * a response with an authentication request to the client.
 */
exports.authenticate = function (req, res, next) {
    var authHeader = req.header('Authorization'),
        challengeReq = 'MAuth realm="http://marte3.dit.upm.es"',
        params;

    var authErr = new e.AuthError('WWW-Authenticate: ' + challengeReq);
    var randomDelay = Math.round(Math.random() * 1000);
    var sendErrorResponse = function () {
        next(authErr);
    };

    if (authHeader !== undefined) {
        params = mauthParser.parseHeader(authHeader);

        // Get the service from the data base.
        dataAccess.service.get(params.serviceid, function (err, serv) {
            if (err) return next(err);
            if (!serv) {
                log.info('[Auth] Unknow service:', params.serviceid);
                setTimeout(sendErrorResponse, randomDelay);
                return;
            }

            var key = serv.key;
            if (serv.encrypted === true) {
                key = cipher.decrypt(cipher.k, key);
            }

            // Check if timestamp and cnonce are valids in order to avoid duplicate requests.
            /* This will affact concurrent performance under same service.
            if (!checkTimestamp(serv, params)) {
                log.info('[Auth] Invalid timestamp or cnonce');
                res.status(401).send({'WWW-Authenticate': challengeReq});
                return;
            }
            */

            // Check if the signature is valid.
            if (checkSignature(params, key)) {
                var authData = {};

                if (params.username !== undefined && params.role !== undefined) {
                    authData.user = (new Buffer(params.username, 'base64').toString('utf8'));
                    authData.role = params.role;
                }
                cache[serv._id+''] =  params;
                authData.service = serv;
                // Put auth data into request for further use
                req.authData = authData;
                // If everything in the authentication is valid continue with the request.
                next();
            } else {
                log.info('[Auth] Wrong credentials');
                setTimeout(sendErrorResponse, randomDelay);
            }
        });
    } else {
        log.info('[Auth] MAuth header not presented');
        setTimeout(sendErrorResponse, randomDelay);
    }
};
