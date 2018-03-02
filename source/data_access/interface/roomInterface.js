/*global exports, require*/
'use strict';
var mongoose = require('mongoose');
var Room = require('./../model/roomModel');
var Service = require('./../model/serviceModel');

var DEFAULT_AUDIO = [
  { codec: 'opus', sampleRate: 48000, channelNum: 2 },
  { codec: 'isac', sampleRate: 16000 },
  { codec: 'isac', sampleRate: 32000 },
  { codec: 'g722', sampleRate: 16000, channelNum: 1 },
  { codec: 'g722', sampleRate: 16000, channelNum: 2 },
  { codec: 'pcma' },
  { codec: 'pcmu' },
  { codec: 'aac' },
  { codec: 'ac3' },
  { codec: 'nellymoser' },
];
var DEFAULT_VIDEO = [
  { codec: 'h264' },
  { codec: 'h265' },
  { codec: 'vp8' },
  { codec: 'vp9' },
];
var DEFAULT_VIDEO_PARA = {
  resolution: ['x3/4', 'x2/3', 'x1/2', 'x1/3', 'x1/4', 'hd1080p', 'hd720p', 'svga', 'vga', 'cif'],
  framerate: [6, 12, 15, 24, 30, 48, 60],
  bitrate: ['x0.8', 'x0.6', 'x0.4', 'x0.2'],
  keyFrameInterval: [100, 30, 5, 2, 1]
};
var DEFAULT_ROLES = [
  {
    role: 'presenter',
    publish: { audio: true, video: true },
    subscribe: { audio: true, video: true }
  },
  {
    role: 'viewer',
    publish: {audio: false, video: false },
    subscribe: {audio: true, video: true }
  },
  {
    role: 'audio_only_presenter',
    publish: {audio: true, video: false },
    subscribe: {audio: true, video: false }
  },
  {
    role: 'video_only_viewer',
    publish: {audio: false, video: false },
    subscribe: {audio: false, video: true }
  },
  {
    role: 'sip',
    publish: { audio: true, video: true },
    subscribe: { audio: true, video: true }
  }
];

/*
 * Create Room.
 */
exports.create = function (serviceId, roomOption, callback) {
  if (!roomOption.mediaOut) {
    roomOption.mediaOut = {
      audio: DEFAULT_AUDIO,
      video: {
        format: DEFAULT_VIDEO,
        parameters: DEFAULT_VIDEO_PARA
      }
    };
  }
  if (!roomOption.mediaIn) {
    roomOption.mediaIn = {
      audio: DEFAULT_AUDIO,
      video: DEFAULT_VIDEO
    };
  }
  if (!roomOption.views) {
    roomOption.views = [{}];
  }
  if (!roomOption.roles) {
    roomOption.roles = DEFAULT_ROLES;
  }

  var room = new Room(roomOption);
  room.save().then((saved) => {
    Service.findById(serviceId).then((service) => {
      service.rooms.push(saved._id);
      service.save().then(() => {
        callback(null, saved.toObject());
      });
    });
  }).catch((err) => {
    callback(err, null);
  });
};

/*
 * List Rooms.
 */
exports.list = function (serviceId, callback) {
  Service.findById(serviceId).populate('rooms').lean().exec(function (err, service) {
    if (err) {
      callback(err);
      return;
    }
    callback(null, service.rooms);
  });
};

/*
 * Get Room. Represents a determined room.
 */
exports.get = function (serviceId, roomId, callback) {
  Service.findById(serviceId).populate('rooms').lean().exec(function (err, service) {
    if (err) return callback(err, null);

    var i;
    for (i = 0; i < service.rooms.length; i++) {
      if (service.rooms[i]._id.toString() === roomId) {
        callback(null, service.rooms[i]);
        return;
      }
    }
    callback(null, null);
  });
};

/*
 * Delete Room. Removes a determined room from the data base.
 */
exports.delete = function (serviceId, roomId, callback) {
  Room.remove({_id: roomId}, function(err0) {
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
  Room.findOneAndUpdate({ _id: roomId }, updates, { overwrite: true }, function (err, ret) {
    if (err) return callback(err, null);
    callback(null, ret.toObject());
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
    Room.find({'sipInfo.sipServer': {$ne: null}}, function(err, rooms) {
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
