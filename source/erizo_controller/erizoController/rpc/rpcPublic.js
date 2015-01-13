var erizoController = require('./../erizoController');

/*
 * This function is called remotely from nuve to get a list of the users in a determined room.
 */
exports.getUsersInRoom = function (id, callback) {
    'use strict';
    erizoController.getUsersInRoom(id, function (users) {
        if (users === undefined) {
            callback('callback', 'error');
        } else {
            callback('callback', users);
        }
    });
};

exports.deleteRoom = function (roomId, callback) {
    'use strict';
    erizoController.deleteRoom(roomId, function (result) {
        callback('callback', result);
    });
};

exports.deleteUser = function (args, callback) {
    'use strict';
    var user = args.user;
    var roomId = args.roomId;
    erizoController.deleteUser(user, roomId, function (result) {
        callback('callback', result);
    });
    
};

exports.updateConfig = function (conf, callback) {
    'use strict';
    erizoController.updateConfig(conf, function (result) {
        callback('callback', result);
    });
};
