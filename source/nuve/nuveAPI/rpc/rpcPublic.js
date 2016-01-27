/*global exports, require*/
'use strict';
var tokenRegistry = require('./../mdb/tokenRegistry');
var roomRegistry = require('./../mdb/roomRegistry');
var cloudHandler = require('./../cloudHandler');
var logger = require('../logger').logger;
var Room = require('../resource/room');

// Logger
var log = logger.getLogger('RPCPublic');

/*
 * This function is used to consume a token. Removes it from the data base and returns to erizoController.
 * Also it removes old tokens.
 */
exports.deleteToken = function (id, callback) {
    tokenRegistry.removeOldTokens();
    tokenRegistry.removeToken(id, function (err, token) {
        if (!err) {
            log.info('Consumed token ', token._id, 'from room ', token.room, ' of service ', token.service);
            callback('callback', token);
        } else {
            log.info('Consume token error:', err, id);
            callback('callback', 'error');
        }
    });
};

exports.getRoomConfig = function (roomId, callback) {

    roomRegistry.getRoom(roomId, function (room) {
        if (room === undefined) {
            return callback('callback', 'error');
        }

        room.mediaMixing = room.mediaMixing || {
            video: {
                avCoordinated: 0,
                maxInput: 16,
                resolution: 0,
                bitrate: 0,
                bkColor: 0,
                layout: {
                    base: 0,
                    custom: null
                }
            },
            audio: null
        };

        (room.enableMixing === undefined) && (room.enableMixing = 1);

        callback('callback', Room.genConfig(room));
    });
};

exports.addNewErizoController = function(msg, callback) {
    cloudHandler.addNewErizoController(msg, function (ec) {
        callback('callback', ec);
    });
};

exports.addNewErizoAgent = function(msg, callback) {
    cloudHandler.addNewErizoAgent(msg, function (ea) {
        callback('callback', ea);
    });
};

exports.getErizoAgent = function(msg, callback) {
    cloudHandler.getErizoAgent(msg, function (ea) {
        callback('callback', ea);
    });
};

exports.keepAlive = function(id, callback) {
    cloudHandler.keepAlive(id, function(result) {
        callback('callback', result);
    });
};

exports.setInfo = function(params, callback) {
    cloudHandler.setInfo(params);
    callback('callback');
};

exports.killMe = function(ip, callback) {
    cloudHandler.killMe(ip);
    callback('callback');
};

exports.reschedule = function (room, callback) { // ensure room is empty before calling this
    cloudHandler.reschedule(room);
    callback('callback');
};

exports.getKey = function (id, callback) {
    callback('callback', cloudHandler.getKey(id));
};
