/*global require, exports, global*/
'use strict';

global.config = global.config || {};
global.config.nuve = global.config.nuve || {};
global.config.nuve.dataBaseURL = global.config.nuve.dataBaseURL || 'localhost/nuvedb';
global.config.nuve.superserviceID = global.config.nuve.superserviceID || '';

var databaseUrl = global.config.nuve.dataBaseURL;

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
exports.superService = global.config.nuve.superserviceID;

// token key
exports.nuveKey = require('crypto').randomBytes(64).toString('hex');
