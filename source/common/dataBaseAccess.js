/*global require, exports, global*/
'use strict';

global.config = global.config || {};
global.config.nuve = global.config.nuve || {};
global.config.nuve.dataBaseURL = global.config.nuve.dataBaseURL || 'localhost/nuvedb';
global.config.nuve.superserviceID = global.config.nuve.superserviceID || '';

var databaseUrl = global.config.nuve.dataBaseURL;

/**
 * Data base collections and its fields are:
 *
 * room { name: '', [data: {}], _id: ObjectId }
 *
 * service { name: '', key: '', rooms: Array[room], testRoom: room, testToken: token, _id: ObjectId }
 *
 * token { host: '', userName: '', room: '', role: '', service: '', creationDate: Date(), [use: int], _id: ObjectId }
 *
 * roles { roleName': {}, _id: roomId }
 *
 * permissions { room: '', role: '', customize: {}, _id: 'userID' }
 */

var collections = ['rooms', 'tokens', 'services', 'roles', 'permissions'];
var db = require('mongojs')(databaseUrl, collections);

/**
 *@function getRolesOfRoom
 *@param {string} roomId
 *@return {promise} resolve(roles)
 */
function getRolesOfRoom(roomId) {
    return new Promise((resolve, reject) => {
        db.roles.findOne({_id: db.ObjectId(roomId)}, function (err, roles) {
            if (err) {
                reject(err);
            } else {
                resolve(roles);
            }
        });
    });
}

/**
 *@function saveRolesOfRoom
 *@param {string} roomId
 *@param {object} roles
 *@return {promise} resolve(roles)
 */
function saveRolesOfRoom(roomId, roles) {
    return new Promise((resolve, reject) => {
        roles._id = db.ObjectId(roomId);
        db.roles.save(roles, function(err, saved) {
            if (err) {
                reject(err);
            } else {
                resolve(saved);
            }
        });
    });
}

/**
 *@function getPermissionOfUser
 *@param {string} userId
 *@param {string} roomId
 *@return {promise} resolve(permission)
 */
function getPermissionOfUser(userId, roomId) {
    return new Promise((resolve, reject) => {
        db.permissions.findOne({_id: userId, room: roomId}, function (err, permission) {
            if (err) {
                reject(err);
            } else {
                resolve(permission);
            }
        });
    });
}

/**
 *@function savePermissionOfUser
 *@param {string} userId
 *@param {string} roomId
 *@param {object} permission
 *@return {promise} resolve(permission)
 */
function savePermissionOfUser(userId, roomId, permission) {
    return new Promise((resolve, reject) => {
        permission._id = userId;
        permission.room = roomId;
        db.permissions.save(permission, function(err, saved) {
            if (err) {
                reject(err);
            } else {
                resolve(saved);
            }
        });
    });
}

/**
 *@function clearPermissionOfRoom
 *@param {string} roomId
 *@return {promise} resolve(permissions)
 */
function clearPermissionOfRoom(roomId) {
    return new Promise((resolve, reject) => {
        db.permissions.remove({room: roomId}, function(err, removed) {
            if (err) {
                reject(err);
            } else {
                resolve(removed);
            }
        });
    });
}

module.exports = {
    getRolesOfRoom,
    saveRolesOfRoom,
    getPermissionOfUser,
    savePermissionOfUser,
    clearPermissionOfRoom,
};