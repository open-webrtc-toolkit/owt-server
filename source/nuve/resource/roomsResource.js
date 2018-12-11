'use strict';
var dataAccess = require('../data_access');
var logger = require('./../logger').logger;
var cloudHandler = require('../cloudHandler');
var e = require('../errors');

// Logger
var log = logger.getLogger('RoomsResource');

/*
 * Post Room. Creates a new room for a determined service.
 */
exports.createRoom = function (req, res, next) {
    var authData = req.authData;

    if (typeof req.body !== 'object' || req.body === null || typeof req.body.name !== 'string' || req.body.name === '') {
        return next(new e.BadRequestError('Invalid request body'));
    }

    if (req.body.options && typeof req.body.options !== 'object') {
        return next(new e.BadRequestError('Invalid room option'));
    }
    req.body.options = req.body.options || {};

    var options = req.body.options;
    options.name = req.body.name;
    dataAccess.room.create(authData.service._id, options, function(err, result) {
        if (!err && result) {
            log.debug('Room created:', req.body.name, 'for service', authData.service.name);
            res.send(result);

            // Notify SIP portal if SIP room created
            if (result && result.sip) {
                log.info('Notify SIP Portal on create Room');
                cloudHandler.notifySipPortal('create', result, function(){});
            }
        } else {
            log.info('Room creation failed', err ? err.message : options);
            next(err || new e.AppError('Create room failed'));
        }
    });
};

/*
 * Get Rooms. Represent a list of rooms for a determined service.
 */
exports.represent = function (req, res, next) {
    var authData = req.authData;

    req.query.page = Number(req.query.page) || undefined;
    req.query.per_page = Number(req.query.per_page) || undefined;

    dataAccess.room.list(authData.service._id, req.query, function (err, rooms) {
        if (rooms) {
            log.debug('Representing rooms for service ', authData.service._id);
            res.send(rooms);
        } else {
            next(err || new e.AppError('Get rooms failed'));
        }
    });
};
