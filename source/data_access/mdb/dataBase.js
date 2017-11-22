/*global require, exports, global*/
//FIXEME: copy from nuve, will be removed later
'use strict';

global.config = global.config || {};
global.config.nuve = global.config.nuve || {};
global.config.nuve.superserviceID = global.config.nuve.superserviceID || '';
global.config.mongo = global.config.mongo || {};
global.config.mongo.dataBaseURL = global.config.mongo.dataBaseURL || 'localhost/nuvedb';

var databaseUrl = global.config.mongo.dataBaseURL;

/*
 * Data base collections and its fields are:
 *
 * room {name: '', [data: {}], _id: ObjectId}
 *
 * service {name: '', key: '', rooms: Array[room], testRoom: room, testToken: token, _id: ObjectId}
 *
 * token {host: '', userName: '', room: '', role: '', service: '', creationDate: Date(), [use: int], _id: ObjectId}
 *
 */
var collections = ['rooms', 'tokens', 'services', 'key'];

exports.db = require('mongojs')(databaseUrl, collections, { reconnectTries: 60 * 15,  reconnectInterval: 1000 });

// Superservice ID
exports.superService = global.config.nuve.superserviceID;

// Save token key, called by cluster master
// FIXME: should store in a cache server instead of db
exports.saveKey = function() {
    var key = require('crypto').randomBytes(64).toString('hex');
    exports.db.key.save({ _id: 'one', nuveKey: key }, function (err, saved) {
        if (err) {
            console.log('Save nuveKey error:', err);
        }
    });
};

// Get token key, called by cluster worker
exports.getKey = function() {
    return new Promise((resolve, reject) => {
        exports.db.key.findOne({_id: 'one'}, function (err, key) {
            if (err || !key) {
                reject(err);
            } else {
                resolve(key.nuveKey);
            }
        });
    });
};
