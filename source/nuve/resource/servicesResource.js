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
