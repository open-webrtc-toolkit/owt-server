/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
var cloudHandler = require('../cloudHandler');
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
 * Delete Room. Removes a determined room from the data base and asks cloudHandler to remove it from erizoController.
 */
exports.deleteRoom = function (req, res, next) {
    var authData = req.authData;
    dataAccess.room.delete(authData.service._id, req.params.room, function(err, room) {
        if (err) return next(err);
        if (!room) {
            log.info('Room ', req.params.room, ' does not exist');
            next(new e.NotFoundError('Room not found'));
        } else {
            var id = req.params.room;
            log.debug('Room ', id, ' deleted for service ', authData.service._id);
            cloudHandler.deleteRoom(id, function () {});
            res.send('Room deleted');

            // Notify SIP portal if SIP room deleted
            if (room.sip) {
                log.debug('Notify SIP Portal on delete Room');
                cloudHandler.notifySipPortal('delete', room, function(){});
            }
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
                        cloudHandler.notifySipPortal(changeType, result, function(){});
                    }
                } else {
                    next(new e.BadRequestError('Bad room configuration'));
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
