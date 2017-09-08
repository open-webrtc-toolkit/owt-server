/*global require, exports, global*/
'use strict';

global.config = global.config || {};
global.config.mongo = global.config.mongo || {};
global.config.mongo.dataBaseURL = global.config.mongo.dataBaseURL || 'localhost/nuvedb';

var databaseUrl = global.config.mongo.dataBaseURL;

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

/* Copied from nuve/resource/room.js,
 * we may use other ways to get room config
 * data from nuve or make database related code
 * a common module used by both nuve and other components.
 * Following functions: getRoomConfig, deleteToken,
 * getKey, getSipRooms are from nuve rpc server
 * (nuve/rpc/rpcPublic.js).
 */
var genConfig = function (room) {
    function Rational(num, den) {
        this.numerator = num;
        this.denominator = den;
    }

    const ZERO = new Rational(0, 1);
    const ONE = new Rational(1, 1);
    const ONE_THIRD = new Rational(1, 3);
    const TWO_THIRDs = new Rational(2, 3);

    function Rectangle({id = '', left, top, relativesize} = {}) {
        this.id = id;
        this.shape = 'rectangle',
        this.area = {
            left: left || ZERO,
            top: top || ZERO,
            width: relativesize || ZERO,
            height: relativesize || ZERO
        };
    }

    function generateLectureTemplates (maxInput) {
        var result = [
            {region:[new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: ONE })]},
            {region:[
                new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: TWO_THIRDs}),
                new Rectangle({id: '2', left: TWO_THIRDs, top: ZERO, relativesize: ONE_THIRD}),
                new Rectangle({id: '3', left: TWO_THIRDs, top: ONE_THIRD, relativesize: ONE_THIRD}),
                new Rectangle({id: '4', left: TWO_THIRDs, top: TWO_THIRDs, relativesize: ONE_THIRD}),
                new Rectangle({id: '5', left: ONE_THIRD, top: TWO_THIRDs, relativesize: ONE_THIRD}),
                new Rectangle({id: '6', left: ZERO, top: TWO_THIRDs, relativesize: ONE_THIRD})
            ]}
        ];
        if (maxInput > 6) { // for maxInput: 8, 10, 12, 14
            var maxDiv = maxInput / 2;
            maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
            maxDiv = maxDiv > 7 ? 7 : maxDiv;

            for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
                var mainReginRelative = new Rational(divFactor - 1, divFactor);
                var minorRegionRelative = new Rational(1, divFactor);

                var regions = [{id: '1', left: ZERO, top: ZERO, relativesize: mainReginRelative}];
                var id = 2;
                for (var y = 0; y < divFactor; y++) {
                    regions.push(new Rectangle({
                        id: ''+(id++),
                        left: mainReginRelative,
                        top: new Rational(y, divFactor),
                        relativesize: minorRegionRelative,
                    }));
                }

                for (var x = divFactor - 2; x >= 0; x--) {
                    regions.push(new Rectangle({
                        id: ''+(id++),
                        left: new Rational(x, divFactor),
                        top: mainReginRelative,
                        relativesize: minorRegionRelative,
                    }));
                }
                result.push({region: regions});
            }

            if (maxInput > 14) { // for maxInput: 17, 21, 25
                var maxDiv = (maxInput + 3) / 4;
                maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
                maxDiv = maxDiv > 7 ? 7 : maxDiv;

                for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
                    var mainReginRelative = new Rational(divFactor - 2, divFactor);
                    var minorRegionRelative = new Rational(1, divFactor);

                    var regions = [{id: '1', left: ZERO, top: ZERO, relativesize: mainReginRelative}];
                    var id = 2;
                    for (var y = 0; y < divFactor - 1; y++) {
                        regions.push(new Rectangle({
                            id: ''+(id++),
                            left: mainReginRelative,
                            top: new Rational(y, divFactor),
                            relativesize: minorRegionRelative,
                        }));
                    }

                    for (var x = divFactor - 3; x >= 0; x--) {
                        regions.push(new Rectangle({
                            id: ''+(id++),
                            left: new Rational(x, divFactor),
                            top: mainReginRelative,
                            relativesize: minorRegionRelative,
                        }));
                    }

                    for (var y = 0; y < divFactor; y++) {
                        regions.push(new Rectangle({
                            id: ''+(id++),
                            left: new Rational(divFactor - 1, divFactor),
                            top: new Rational(y, divFactor),
                            relativesize: minorRegionRelative
                        }));
                    }

                    for (var x = divFactor - 2; x >= 0; x--) {
                        regions.push(new Rectangle({
                            id: ''+(id++),
                            left: new Rational(x, divFactor),
                            top: new Rational(divFactor - 1, divFactor),
                            relativesize: minorRegionRelative
                        }));
                    }
                    result.push({region: regions});
                }
            }
        }
        return result;
    }

    function generateFluidTemplates (maxInput) {
        var result = [];
        var maxDiv = Math.sqrt(maxInput);
        if (maxDiv > Math.floor(maxDiv))
            maxDiv = Math.floor(maxDiv) + 1;
        else
            maxDiv = Math.floor(maxDiv);

        for (var divFactor = 1; divFactor <= maxDiv; divFactor++) {
            var regions = [];
            var relativeSize = new Rational(1, divFactor);
            var id = 1;
            for (var y = 0; y < divFactor; y++)
                for(var x = 0; x < divFactor; x++) {
                    var region = new Rectangle({id: String(id++),
                                  left: new Rational(x, divFactor),
                                  top: new Rational(y, divFactor),
                                  relativesize: relativeSize});
                    regions.push(region);
                }

            result.push({region: regions});
        }
        return result;
    }

    function isTemplatesValid (templates) {
        if (!(templates instanceof Array)) {
            return false;
        }

        for (var i in templates) {
            var region = templates[i].region;
            if (!(region instanceof Array))
                return false;
        }
        return true;
    }

    var genMediaMixing = function(mediaMixing) {
        var layoutType = mediaMixing.video.layout.base;
        var maxInput = mediaMixing.video.maxInput || 16;
        var layoutTemplates;

        if (layoutType === 'fluid') {
        layoutTemplates = generateFluidTemplates(maxInput);
        } else if (layoutType === 'lecture') {
            layoutTemplates = generateLectureTemplates(maxInput);
        } else if (layoutType === 'void') {
            layoutTemplates = [];
        } else {
            layoutTemplates = generateFluidTemplates(maxInput);
        }

        var custom = mediaMixing.video.layout.custom;
        if (isTemplatesValid(custom)) {
            custom.map(function (tpl) {
                var len = tpl.region.length;
                var pos;
                for (var j in layoutTemplates) {
                    if (layoutTemplates.hasOwnProperty(j)) {
                        if (layoutTemplates[j].region.length >= len) {
                            pos = j;
                            break;
                        }
                    }
                }
                if (pos === undefined) {
                    layoutTemplates.push(tpl);
                } else if (layoutTemplates[pos].region.length === len) {
                    layoutTemplates.splice(pos, 1, tpl);
                } else {
                    layoutTemplates.splice(pos, 0, tpl);
                }
            });
        }

        return {
            video: {
                avCoordinated: mediaMixing.video.avCoordinated === 1,
                multistreaming: mediaMixing.video.multistreaming === 1,
                maxInput: maxInput,
                quality_level: mediaMixing.video.quality_level || 'standard',
                resolution: mediaMixing.video.resolution || 'vga',
                bkColor: mediaMixing.video.bkColor || 'black',
                crop: mediaMixing.video.crop === 1,
                layout: layoutTemplates
            },
            audio: null
        };
    };

    var config = {
        mode: room.mode,
        publishLimit: room.publishLimit,
        userLimit: room.userLimit,
        enableMixing: room.enableMixing === 1,
    };

    if (room.enableMixing && room.views) {
        config.views = {};
        for (var view in room.views) {
            config.views[view] = { mediaMixing: genMediaMixing(room.views[view].mediaMixing) };
        }
    } else if (room.enableMixing && room.mediaMixing) {
        // Old style configuration
        config.views = { "common": { mediaMixing: genMediaMixing(room.mediaMixing) } };
    }

    if (room.sipInfo) {
        config.sip = room.sipInfo;
    }

    return config;
};

/**
 *@function getRoomConfig
 *@param {string} roomId
 *@return {promise} resolve(roomConfig)
 */
function getRoomConfig(roomId) {
    return new Promise((resolve, reject) => {
        db.rooms.findOne({_id: db.ObjectId(roomId)}, function (err, room) {
            if (err || !room) {
                reject(err || `No Room Configuration ${roomId}`);
            } else {
                resolve(genConfig(room));
            }
        });
    });
}

/**
 *@function deleteToken
 *@param {string} tokenId
 *@return {promise} resolve(token)
 */
function deleteToken(tokenId) {
    return new Promise((resolve, reject) => {
        // Expire time is 3 minutes, from nuve/mdb/tokenRegistry.js
        var expireDate = new Date(new Date().getTime() - 1000 * 60 * 3);
        var resultToken;
        db.tokens.findOne({_id: db.ObjectId(tokenId)}, function (errFind, token) {
            db.tokens.remove({
                $or: [
                    {_id: db.ObjectId(tokenId)},
                    {creationDate: {$lt: expireDate}}
                ]},
                function (errRemove, remove) {
                    if (errFind || !token) {
                        reject(errFind || 'WrongToken');
                    } else {
                        if (token.creationDate < expireDate) {
                            reject('ExpiredToken');
                        } else {
                            resolve(token);
                        }
                    }
                });
        });
    });
};

/**
 *@function getKey
 *@param {void}
 *@return {promise} resolve(key)
 */
function getKey() {
    return new Promise((resolve, reject) => {
        db.key.findOne({_id: 'one'}, function (err, key) {
            if (err || !key) {
                reject(err);
            } else {
                resolve(key.nuveKey);
            }
        });
    });
};

/**
 *@function getSipRooms
 *@param {void}
 *@return {promise} resolve([room])
 */
function getSipRooms() {
    return new Promise((resolve, reject) => {
        db.rooms.find({'sipInfo.sipServer': {$ne: null}}, function(err, rooms) {
            if (err || !rooms) {
                reject(err);
            } else {
                var result = rooms.map((room) => {
                    return {roomId: room._id.toString(), sipInfo: room.sipInfo};
                });
                resolve(result);
            }
        });
    });
};

module.exports = {
    getRolesOfRoom,
    saveRolesOfRoom,
    getPermissionOfUser,
    savePermissionOfUser,
    clearPermissionOfRoom,
    getRoomConfig,
    deleteToken,
    getKey,
    getSipRooms,
};