/*global exports, require, console, Buffer, setTimeout, clearTimeout*/
var tokenRegistry = require('./../mdb/tokenRegistry');
var serviceRegistry = require('./../mdb/serviceRegistry');
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
    'use strict';

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
    'use strict';

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

        callback('callback', Room.genConfig(room));
    });
};

exports.addNewErizoController = function(msg, callback) {
    'use strict';
    cloudHandler.addNewErizoController(msg, function (id) {
        callback('callback', id);   
    });
};

exports.keepAlive = function(id, callback) {
    'use strict';
    cloudHandler.keepAlive(id, function(result) {
        callback('callback', result);
    });
};

exports.setInfo = function(params, callback) {
    'use strict';
    cloudHandler.setInfo(params);
    callback('callback');
};

exports.killMe = function(ip, callback) {
    'use strict';
    cloudHandler.killMe(ip);
    callback('callback');
};

exports.reschedule = function (room, callback) { // ensure room is empty before calling this
    'use strict';
    cloudHandler.reschedule(room);
    callback('callback');
};
