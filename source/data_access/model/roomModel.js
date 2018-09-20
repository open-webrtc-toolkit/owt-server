'use strict';
var mongoose = require('mongoose');
var Fraction = require('fraction.js');
var Schema = mongoose.Schema;
var Layout = require('./layoutTemplate');

var ColorRGB = {
  type: Number,
  min: 0,
  max: 255,
  default: 0
};

var RNumber = {
  type: String,
  validate: {
    validator: function(v) {
      try {
        new Fraction(v);
      } catch (e) {
        return false;
      }
      return true;
    },
    message: '{VALUE} is not a valid fraction string!'
  },
  required: true
};

var Region = {
  _id: false,
  id: { type: String, required: true },
  shape: { type: String, enum: ['rectangle'], default: 'rectangle' },
  area: {
    left: RNumber,
    top: RNumber,
    width: RNumber,
    height: RNumber
  }
};

var Resolution = {
  width: { type: Number, default: 640 },
  height: { type: Number, default: 480 }

};

var ViewSchema = new Schema({
  label: { type: String, default: 'common' },
  audio: {
    format: {
      codec: { type: String, default: 'opus' },
      sampleRate: { type: Number, default: 48000 },
      channelNum: { type: Number, default: 2 }
    },
    vad: { type: Boolean, default: true },
  },
  video: {
    format: {
      codec: { type: String, default: 'vp8' },
      profile: { type: String }
    },
    parameters: {
      resolution: Resolution,
      framerate: { type: Number, default: 24 },
      bitrate: { type: Number },
      keyFrameInterval: { type: Number, default: 100 },
    },
    maxInput: { type: Number, default: 16, min: 1, max: 256 },
    motionFactor: { type: Number, default: 0.8 },
    bgColor: { r: ColorRGB, g: ColorRGB, b: ColorRGB },
    keepActiveInputPrimary: { type: Boolean, default: false },
    layout: {
      //TODO: stretched?
      fitPolicy: { type: String, enum: ['letterbox', 'crop'], default: 'letterbox' },
      setRegionEffect: { type: String },
      templates: {
        base: { type: String, enum: ['fluid', 'lecture', 'void'], default: 'fluid' },
        custom: [{
          _id: false,
          primary: { type: String },
          region: [ Region ]
        }]
      }
    }
  }
},
{ _id: false });


var RoomSchema = new Schema({
  name: {
    type: String,
    required: true
  },
  inputLimit: {
    type: Number,
    default: -1
  },
  participantLimit: {
    type: Number,
    default: -1
  },
  roles: [],
  views: [ViewSchema],
  mediaIn: {
    audio: [],
    video: []
  },
  mediaOut: {
    audio: [],
    video: {
      format: [],
      parameters: {
        resolution: [],
        framerate: [],
        bitrate: [],
        keyFrameInterval: []
      }
    }
  },
  transcoding: {
    audio: { type: Boolean, default: true },
    video: {
      format: { type: Boolean, default: true },
      parameters: {
        resolution: { type: Boolean, default: true },
        framerate: { type: Boolean, default: true },
        bitrate: { type: Boolean, default: true },
        keyFrameInterval: { type: Boolean, default: true }
      }
    }
  },
  sip: {
    sipServer: String,
    username: String,
    password: String
  },
  notifying: {
    participantActivities: { type: Boolean, default: true },
    streamChange: { type: Boolean, default: true }
  }
});

RoomSchema.statics.ViewSchema = mongoose.model('View', ViewSchema);

RoomSchema.statics.processLayout = function(room) {
  if (room && room.views) {
    room.views.forEach(function (view) {
      if (view.video) {
        view.video.layout.templates = Layout.applyTemplate(
          view.video.layout.templates.base,
          view.video.maxInput,
          view.video.layout.templates.custom);
      }
    });
  }
  return room;
};

module.exports = mongoose.model('Room', RoomSchema);
