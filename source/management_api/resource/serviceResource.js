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
var e = require('../errors');

// Logger
var log = logger.getLogger('ServiceResource');

/*
 * Gets the service and checks if it is superservice. Only superservice can do actions about services.
 */

var superService = global.config.server.superserviceID;

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
