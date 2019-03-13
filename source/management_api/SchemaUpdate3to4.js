#!/usr/bin/env node

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var databaseUrl = process.env.DB_URL;
if (!databaseUrl) {
  databaseUrl = 'localhost/owtdb';
}

var db = require('mongojs')(databaseUrl, ['rooms', 'services']);
var Fraction = require('fraction.js');
var DefaultUtil = require('./data_access/defaults');
var Room = require('./data_access/model/roomModel');

function fracString(num) {
  var frac = Fraction(0);
  try {
    if (typeof num.denominator === 'number' && typeof num.numerator === 'number') {
      frac = Fraction(num.numerator, num.denominator);
    } else {
      frac = Fraction(num);
    }
  } catch (e) {
    // use 0 as default
  }
  return frac.toFraction();
}

function upgradeLayoutCustom(custom) {
  custom.forEach(function (tpl) {
    tpl.region.forEach(function (region) {
      if (typeof region.relativesize === 'number') {
        region.shape = 'rectangle';
        region.area = {
          left: fracString(region.left),
          top: fracString(region.top),
          width: fracString(region.relativesize),
          height: fracString(region.relativesize)
        };
        delete region.relativesize;
      } else {
        if (!region.area) {
          region.area = {};
        }
        region.area = {
          left: fracString(region.area.left),
          top: fracString(region.area.top),
          width: fracString(region.area.width),
          height: fracString(region.area.height)
        };
      }
    });
  });
  return custom;
}

function upgradeColor(oldColor) {
  var colorMap = {
    'white': { r: 255, g: 255, b: 255 },
    'black': { r: 0, g: 0, b: 0 }
  };
  if (typeof oldColor === 'string' && colorMap[oldColor]) {
    return colorMap[oldColor];
  }
  if (typeof oldColor !== 'object') {
    return { r: 0, g: 0, b: 0};
  }
  return oldColor;
}

function upgradeResolution(oldResolution) {
  //translate view definition.
  var resolutionName2Value = {
    'cif': {width: 352, height: 288},
    'vga': {width: 640, height: 480},
    'svga': {width: 800, height: 600},
    'xga': {width: 1024, height: 768},
    'r640x360': {width: 640, height: 360},
    'hd720p': {width: 1280, height: 720},
    'sif': {width: 320, height: 240},
    'hvga': {width: 480, height: 320},
    'r480x360': {width: 480, height: 360},
    'qcif': {width: 176, height: 144},
    'r192x144': {width: 192, height: 144},
    'hd1080p': {width: 1920, height: 1080},
    'uhd_4k': {width: 3840, height: 2160},
    'r360x360': {width: 360, height: 360},
    'r480x480': {width: 480, height: 480},
    'r720x720': {width: 720, height: 720},
    'r720x1280': {width: 720, height: 1280},
    'r1080x1920': {width: 1080, height: 1920},
  };
  return resolutionName2Value[oldResolution] || {width: 640, height: 480};
}


function upgradeRoom(oldConfig) {
  var config = (new Room(DefaultUtil.ROOM_CONFIG)).toObject();
  config._id = oldConfig._id;
  config.name = oldConfig.name;
  config.inputLimit = oldConfig.publishLimit;
  config.participantLimit = oldConfig.userLimit;
  // TO work with 3.4
  config.roles.push({
    role: 'admin',
    publish: { audio: true, video: true },
    subscribe: { audio: true, video: true }
  });
  config.sip = oldConfig.sip ? oldConfig.sip : config.sip;
  if (oldConfig.mediaMixing && !oldConfig.views) {
    oldConfig.views = { common: { mediaMixing: oldConfig.mediaMixing }};
  }

  var oldName, oldMedia, newView;
  config.views = [];
  for (oldName in oldConfig.views) {
    oldMedia = oldConfig.views[oldName].mediaMixing;
    newView = (new Room.ViewSchema()).toObject();
    newView.label = oldName;
    newView.audio.vad = !!(oldMedia.video && oldMedia.video.avCoordinated);
    newView.video.parameters.resolution = upgradeResolution(oldMedia.video.resolution);
    newView.video.bgColor = upgradeColor(oldMedia.video.bkColor);
    newView.video.maxInput = oldMedia.video.maxInput;
    newView.video.layout.fitPolicy = oldMedia.video.crop ? 'crop' : 'letterbox';
    newView.video.layout.templates.base = oldMedia.video.layout.base;
    newView.video.layout.templates.custom = upgradeLayoutCustom(oldMedia.video.layout.custom);

    config.views.push(newView);
  }

  return config;
};

function processRoom(rooms, i, onFinishRoom) {
  if (i >= rooms.length) {
    if (typeof onFinishRoom === 'function')
      onFinishRoom();
    return;
  }

  var room = upgradeRoom(rooms[i]);
  db.rooms.save(room, function (err, saved) {
    if (err) {
      console.log('Error in updating room:', room._id, err.message);
    } else {
      console.log('Room:', room._id, 'converted.');
    }
    processRoom(rooms, i + 1, onFinishRoom);
  });
}

function processServices(services, i, onFinishService) {
  if (i >= services.length) {
    if (typeof onFinishService === 'function')
      onFinishService();
    return;
  }

  var service = services[i];
  if (!service.rooms) {
    service.rooms = [];
  }
  var processNext = function() {
    service.__v = 0;
    service.rooms = service.rooms.map((room) => (room._id ? room._id : room));
    db.services.save(service, function (e, saved) {
      if (e) {
        console.log('Error in updating service:', service._id, e.message);
      } else {
        console.log('Service:', service._id, 'converted.');
      }
      processServices(services, i + 1, onFinishService);
    });
  };

  if (service.rooms.length > 0) {
    if (service.rooms[0]._id) {
      // Not 3.5.2
      processRoom(service.rooms, 0, processNext);
    } else {
      // For 3.5.2
      db.rooms.find({_id: {$in: service.rooms}}).toArray(function (err, rooms) {
        if (!err && rooms && rooms.length > 0) {
            processRoom(rooms, 0, processNext);
        }
      });
    }
  } else {
    processNext();
  }
}

function processAll(finishCb) {
  db.services.find({}).toArray(function (err, services) {
    console.log('Starting');
    if (err) {
      console.log('Error in getting services:', err.message);
      db.close();
      return;
    }
    processServices(services, 0, function() {
      db.close();
      if (typeof finishCb === 'function')
        finishCb();
    });
  });
}

function main(next) {
  var rl = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout
  });

  rl.question('This operation will overwrite the old database, which ' +
      'is irreversible and may cause incompatibility between old version ' +
      'MCU\'s and the converted database. It is strongly suggested to ' +
      'backup the database files before continuing. Are you sure you want ' +
      'to proceed this operation anyway?[y/n]', (answer) => {
    rl.close();

    answer = answer.toLowerCase();
    if (answer === 'y' || answer === 'yes') {
      processAll(function() {
        console.log('Finish');
        next();
      });
    } else {
      process.exit(0);
    }
  });
}


if (require.main === module) {
    main();
} else {
  exports.update = function (cb) {
    main(cb);
  };
}
