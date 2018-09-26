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
  // { codec: 'g722', sampleRate: 16000, channelNum: 2 },
  { codec: 'pcma' },
  { codec: 'pcmu' },
  { codec: 'aac' },
  { codec: 'ac3' },
  { codec: 'nellymoser' },
  { codec: 'ilbc' },
];
var DEFAULT_AUDIO_OUT = [
  { codec: 'opus', sampleRate: 48000, channelNum: 2 },
  { codec: 'isac', sampleRate: 16000 },
  { codec: 'isac', sampleRate: 32000 },
  { codec: 'g722', sampleRate: 16000, channelNum: 1 },
  // { codec: 'g722', sampleRate: 16000, channelNum: 2 },
  { codec: 'pcma' },
  { codec: 'pcmu' },
  { codec: 'aac', sampleRate: 48000, channelNum: 2 },
  { codec: 'ac3' },
  { codec: 'nellymoser' },
  { codec: 'ilbc' },
];
var DEFAULT_VIDEO_IN = [
  { codec: 'h264' },
  { codec: 'vp8' },
  { codec: 'vp9' },
];
var DEFAULT_VIDEO_OUT = [
  { codec: 'h264', profile: 'CB' },
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
  if (!roomOption.mediaOut) {
    roomOption.mediaOut = {
      audio: DEFAULT_AUDIO_OUT,
      video: {
        format: DEFAULT_VIDEO_OUT,
        parameters: DEFAULT_VIDEO_PARA
      }
    };
  }
  if (!roomOption.mediaIn) {
    roomOption.mediaIn = {
      audio: DEFAULT_AUDIO,
      video: DEFAULT_VIDEO_IN
    };
  }
  if (!roomOption.views) {
    roomOption.views = [{}];
  }
  if (!roomOption.roles) {
    roomOption.roles = DEFAULT_ROLES;
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
    options: { sort: '-createdAt' }
  };
  if (options) {
    if (typeof options.per_page === 'number' && options.per_page > 0) {
      popOption.options.limit = options.per_page;
      if (typeof options.page === 'number' && options.page > 0) {
        popOption.options.skip = (options.page - 1) * options.per_page;
      }
    }
  }

  Service.findById(serviceId).populate(popOption).lean().exec(function (err, service) {
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
