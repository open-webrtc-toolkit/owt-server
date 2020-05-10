// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var mongoose = require('mongoose');
var Default = require('./../defaults');
var Room = require('./../model/roomModel');
var Service = require('./../model/serviceModel');

function getAudioOnlyLabels(roomOption) {
  var labels = [];
  if (roomOption.views && roomOption.views.forEach) {
    roomOption.views.forEach((view) => {
      if (view.video === false) {
        labels.push(view.label);
      }
    });
  }
  return labels;
}

function checkMediaOut(room, roomOption) {
  var valid = true;
  var i;
  if (room && room.views) {
    room.views.forEach((view, vindex) => {
      if (!valid)
        return;

      i = room.mediaOut.audio.findIndex((afmt) => {
        return (afmt.codec === view.audio.format.codec
          && afmt.sampleRate === view.audio.format.sampleRate
          && afmt.channelNum === view.audio.format.channelNum);
      });
      if (i === -1) {
        valid = false;
        return;
      }

      if (roomOption.views && roomOption.views[vindex]
            && roomOption.views[vindex].video === false) {
        return;
      }

      i = room.mediaOut.video.format.findIndex((vfmt) => {
        return (vfmt.codec === view.video.format.codec
          && vfmt.profile === view.video.format.profile);
      });
      if (i === -1)
        valid = false;
    });
  }
  return valid;
}

function updateAudioOnlyViews(labels, room, callback) {
  if (room.views && room.views.map) {
    room.views = room.views.map((view) => {
      if (labels.indexOf(view.label) > -1) {
        view.video = false;
      }
      return view;
    });
  }
  room.save({validateBeforeSave: false}, function (err, raw) {
    if (err) return callback(err, null);
    callback(null, room.toObject());
  });
}

const removeNull = (obj) => {
  Object.keys(obj).forEach(key => {
    if (obj[key] && typeof obj[key] === 'object')
      removeNull(obj[key]);
    else if
      (obj[key] == null) delete obj[key];
  });
};

/*
 * Create Room.
 */
exports.create = function (serviceId, roomOption, callback) {
  var attr;
  for (attr in Default.ROOM_CONFIG) {
    if (!roomOption[attr]) {
      roomOption[attr] = Default.ROOM_CONFIG[attr];
    }
  }

  removeNull(roomOption);
  var labels = getAudioOnlyLabels(roomOption);
  var room = new Room(roomOption);
  if (!checkMediaOut(room, roomOption)) {
    callback(new Error('MediaOut conflicts with View Setting'), null);
    return;
  }
  room.save().then((saved) => {
    Service.findById(serviceId).then((service) => {
      service.rooms.push(saved._id);
      service.save().then(() => {
        if (labels.length > 0) {
          updateAudioOnlyViews(labels, saved, callback);
        } else {
          callback(null, saved.toObject());
        }
      });
    });
  }).catch((err) => {
    callback(err, null);
  });
};

/*
 * List Rooms.
 */
exports.list = function (serviceId, options, callback) {
  var popOption = {
    path: 'rooms',
    options: { sort: {_id: 1} }
  };
  if (options) {
    if (typeof options.per_page === 'number' && options.per_page > 0) {
      popOption.options.limit = options.per_page;
      if (typeof options.page === 'number' && options.page > 0) {
        popOption.options.skip = (options.page - 1) * options.per_page;
      }
    }
  }

  Service.findById(serviceId).populate(popOption).exec(function (err, service) {
    if (err) {
      callback(err);
      return;
    }
    // Current mongoose version has problem with lean getters
    callback(null, service.rooms.map((room) => room.toObject()));
  });
};

/*
 * Get Room. Represents a determined room.
 */
exports.get = function (serviceId, roomId, callback) {
  Service.findById(serviceId).lean().exec(function (err, service) {
    
    if (err) return callback(err, null);

    var i, match = false;
    for (i = 0; i < service.rooms.length; i++) {
      if (service.rooms[i].toString() === roomId) {
        match = true;
        break;
      }
    }

    if (!match) return callback(null, null);
    
    Room.findById(roomId).lean().exec(function (err, room) {
        return callback(err, room);
    });
  });
};

/*
 * Delete Room. Removes a determined room from the data base.
 */
exports.delete = function (serviceId, roomId, callback) {
  Room.deleteOne({_id: roomId}, function(err0) {
    Service.findByIdAndUpdate(serviceId, { '$pull' : { 'rooms' : roomId } },
      function (err1, service) {
        if (err1) console.log('Pull rooms fail:', err1.message);
        callback(err0, roomId);
      });
  });
};

/*
 * Update Room. Update a determined room from the data base.
 */
exports.update = function (serviceId, roomId, updates, callback) {
  removeNull(updates);
  var labels = getAudioOnlyLabels(updates);
  Room.findById(roomId).then((room) => {
    var newRoom = Object.assign(room, updates);
    if (!checkMediaOut(newRoom, updates)) {
      throw new Error('MediaOut conflicts with View Setting');
    }
    return newRoom.save();
  }).then((saved) => {
    if (labels.length > 0) {
      updateAudioOnlyViews(labels, saved, callback);
    } else {
      callback(null, saved.toObject());
    }
  }).catch((err) => {
    callback(err, null);
  });
};

/*
 * Get a room's configuration. Called by conference.
 */
exports.config = function (roomId) {
  return new Promise((resolve, reject) => {
    Room.findById(roomId, function (err, room) {
      if (err || !room) {
        reject(err);
      } else {
        var config = Room.processLayout(room.toObject());
        resolve(config);
      }
    });
  });
};

/*
 * Get sip rooms. Called by sip portal.
 */
exports.sips = function () {
  return new Promise((resolve, reject) => {
    Room.find({'sip.sipServer': {$ne: null}}, function(err, rooms) {
      if (err || !rooms) {
        resolve([]);
      } else {
        var result = rooms.map((room) => {
          return { roomId: room._id.toString(), sip: room.sip };
        });
        resolve(result);
      }
    });
  });
};
