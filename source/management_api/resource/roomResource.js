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
var requestHandler = require('../requestHandler');
var logger = require('./../logger').logger;
var e = require('../errors');

// Logger
var log = logger.getLogger('RoomResource');

/*
 * Get Room. Represents a determined room.
 */
exports.represent = function (req, res, next) {
    var authData = req.authData;
    dataAccess.room.get(authData.service._id, req.params.room, function (err, room) {
        if (!room) {
            log.info('Room ', req.params.room, ' does not exist');
            next(new e.NotFoundError('Room not found'));
        } else {
            log.info('Representing room ', room._id, 'of service ', authData.service._id);
            res.send(room);
        }
    });
};

/*
 * Delete Room. Removes a determined room from the data base and asks requestHandler to remove it from erizoController.
 */
exports.deleteRoom = function (req, res, next) {
    var authData = req.authData;
    dataAccess.room.get(authData.service._id, req.params.room, function (err, room) {
        if (!room) {
            log.info('Room ', req.params.room, ' does not exist');
            next(new e.NotFoundError('Room not found'));
        } else {
            var sip_info = room.sip;
            dataAccess.room.delete(authData.service._id, req.params.room, function(err, room) {
                if (err) {
                  return next(err);
                } else {
                    var id = req.params.room;
                    log.debug('Room ', id, ' deleted for service ', authData.service._id);
                    requestHandler.deleteRoom(id, function () {});
                    res.send('Room deleted');

                    // Notify SIP portal if SIP room deleted
                    if (sip_info) {
                        log.debug('Notify SIP Portal on delete Room');
                        requestHandler.notifySipPortal('delete', {_id: id, sip: sip_info}, function(){});
                    }
                }
            });
        }
    });
};

exports.updateRoom = function (req, res, next) {
    var authData = req.authData;
    var updates = req.body;
    dataAccess.room.get(authData.service._id, req.params.room, function (err, room) {
        if (!room) {
            log.info('Room ', req.params.room, ' does not exist');
            next(new e.NotFoundError('Room not found'));
        } else {
            var updates = req.body;
            dataAccess.room.update(authData.service._id, req.params.room, updates, function(err, result) {
                if (result) {
                    res.send(result);

                    // Notify SIP portal if SIP room updated
                    var sipOld = room.sip;
                    var sipNew = result.sip;
                    var changeType;
                    var sipField;
                    if (!sipOld && sipNew) {
                        changeType = 'create';
                    } else if (sipOld && !sipNew) {
                        changeType = 'delete';
                    } else if (sipOld && sipNew) {
                        for (sipField in sipNew) {
                            if (sipOld[sipField] !== sipNew[sipField]) {
                                changeType = 'update';
                                break;
                            }
                        }
                    }
                    if (changeType) {
                        log.debug('Change type', changeType);
                        requestHandler.notifySipPortal(changeType, result, function(){});
                    }
                } else {
                    next(new e.BadRequestError(err && err.message || 'Bad room configuration'));
                }
            });
        }
    });
};

exports.validate = function (req, res, next) {
    var authData = req.authData;
    dataAccess.room.get(authData.service._id, req.params.room, function (err, room) {
        if (err) {
            return next(err);
        }
        if (!room) {
            next(new e.NotFoundError('Room not found'));
        } else {
            req.authData.room = room;
            next();
        }
    });
}
