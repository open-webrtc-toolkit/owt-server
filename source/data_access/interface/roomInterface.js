/*global exports, require*/
'use strict';
var roomRegistry = require('./../mdb/roomRegistry');
var serviceRegistry = require('./../mdb/serviceRegistry');
var Room = require('./room');
var db = require('./../mdb/dataBase').db;

/*
 * Create Room.
 */
exports.create = function (serviceId, roomOption, callback) {
  var room = Room.create(roomOption);
  if (room === null) {
    callback(null);
    return;
  }
  roomRegistry.addRoom(room, function (result) {
    serviceRegistry.addRoomInService(serviceId, result, function (err, ret) {
      if (!err) {
        callback(result);
      } else {
        callback(null);
      }
    });
  });
};

/*
 * List Rooms.
 */
exports.list = function (serviceId, callback) {
  serviceRegistry.getService(serviceId, function (service) {
    if (service) {
      callback(service.rooms);
    } else {
      callback([]);
    }
  });
};

/*
 * Get Room. Represents a determined room.
 */
exports.get = function (serviceId, roomId, callback) {
  serviceRegistry.getService(serviceId, function (service) {
    if (service) {
      serviceRegistry.getRoomForService(roomId, service, callback);
    } else {
      callback(null);
    }
  });
};

/*
 * Delete Room. Removes a determined room from the data base.
 */
exports.delete = function (serviceId, roomId, callback) {
  serviceRegistry.getService(serviceId, function (service) {
    roomRegistry.removeRoom(roomId, function (removed) {
      if (service) {
        for (let r of service.rooms) {
          if (r._id + '' === roomId) {
            serviceRegistry.deleteRoomInService(serviceId, r, function (err, ret) {
              if (err) {
                callback(null);
              } else {
                callback(r);
              }
            });
            return;
          }
        }
      }
      if (removed) {
        callback(removed);
      } else {
        callback(null);
      }
    });
  });
};

/*
 * Update Room. Update a determined room from the data base.
 */
exports.update = function (serviceId, roomId, updates, callback) {
  if (typeof updates === 'object' && updates !== null) {
    console.log('Room', 'updateRoom updates', updates);
    var newRoom = Room.create(updates);
    if (newRoom === null) {
      console.log('Room', 'invalid configuration');
      callback(null);
      return;
    }

    roomRegistry.getRoom(roomId, function (room) {
      if (!room) {
        callback(null);
        return;
      }

      Object.keys(updates).map(function (k) {
        if (newRoom.hasOwnProperty(k)) {
          if (k === 'views') {
            room[k] = newRoom[k];
            // Remove old style media mixing configuration
            delete room['mediaMixing'];
          } else if (k !== 'mediaMixing') {
            room[k] = newRoom[k];
          } else if (typeof updates.mediaMixing.video === 'object') {
            room.mediaMixing = room.mediaMixing || {};
            room.mediaMixing.video = room.mediaMixing.video || {};
            Object.keys(updates.mediaMixing.video).map(function (k) {
              if (newRoom.mediaMixing.video.hasOwnProperty(k)) {
                if (k !== 'layout') {
                  room.mediaMixing.video[k] = newRoom.mediaMixing.video[k];
              } else if (typeof updates.mediaMixing.video.layout === 'object') {
                  room.mediaMixing.video.layout = room.mediaMixing.video.layout || {};
                  Object.keys(updates.mediaMixing.video.layout).map(function (k) {
                    if (newRoom.mediaMixing.video.layout.hasOwnProperty(k)) {
                      room.mediaMixing.video.layout[k] = newRoom.mediaMixing.video.layout[k];
                    }
                  });
                }
              }
            });
          }
        }
      });

      roomRegistry.addRoom(room, function (result) {
        if (!room) {
          callback(null);
          return;
        }
        serviceRegistry.updateRoomInService(serviceId, result, function(err, updated) {
          if (err) {
            callback(null);
          } else {
            callback(result);
          }
        });
      });
    });
  } else {
    callback(null);
  }
};

/*
 * Get a room's configuration. Called by conference.
 */
exports.config = function (roomId) {
  return new Promise((resolve, reject) => {
    db.rooms.findOne({_id: db.ObjectId(roomId)}, function (err, room) {
      if (err || !room) {
        reject(err);
      } else {
        resolve(Room.genConfig(room));
      }
    });
  });
};

/*
 * Get sip rooms. Called by sip portal.
 */
exports.sips = function () {
  return new Promise((resolve, reject) => {
    db.rooms.find({'sipInfo.sipServer': {$ne: null}}, function(err, rooms) {
      if (err || !rooms) {
        resolve([]);
      } else {
        var result = rooms.map((room) => {
          return {roomId: room._id.toString(), sipInfo: room.sipInfo};
        });
        resolve(result);
      }
    });
  });
};