/*global require, exports, GLOBAL*/
'use strict';

GLOBAL.config = GLOBAL.config || {};
GLOBAL.config.nuve = GLOBAL.config.nuve || {};
GLOBAL.config.nuve.dataBaseURL = GLOBAL.config.nuve.dataBaseURL || 'localhost/nuvedb';
GLOBAL.config.nuve.superserviceID = GLOBAL.config.nuve.superserviceID || '';

var databaseUrl = GLOBAL.config.nuve.dataBaseURL;

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
var collections = ['rooms', 'tokens', 'services'];
exports.db = require('mongojs')(databaseUrl, collections);

// Superservice ID
exports.superService = GLOBAL.config.nuve.superserviceID;

// token key
exports.nuveKey = require('crypto').randomBytes(64).toString('hex');
