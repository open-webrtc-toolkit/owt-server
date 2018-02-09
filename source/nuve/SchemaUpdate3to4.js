#!/usr/bin/env node
'use strict';
var Fraction = require('fraction.js');

var databaseUrl = 'localhost/nuvedb';
var collections = ['rooms', 'services'];

var databaseUrl = process.env.DB_URL;
if (!databaseUrl) {
  databaseUrl = 'localhost/nuvedb';
}

var db = require('mongojs')(databaseUrl, ['rooms', 'services']);

function makeFraction(num) {
  if (typeof num === 'number') return num.toString();
  if (typeof num.denominator === 'number' && typeof num.numerator === 'number')
    return makeFraction(num.numerator, num.denominator);
  return '1';
}

function translateCustom(custom) {
  custom.forEach(function (tpl) {
    tpl.region.forEach(function (region) {
      if (typeof region.relativesize === 'number') {
        region.shape = 'rectangle';
        region.area = {
          left: makeFraction(region.left),
          top: makeFraction(region.top),
          width: makeFraction(region.relativesize),
          height: makeFraction(region.relativesize)
        };
        delete region.relativesize;
      } else {
        region.area = {
          left: makeFraction(region.area.left),
          top: makeFraction(region.area.top),
          width: makeFraction(region.area.width),
          height: makeFraction(region.area.height)
        };
      }
    });
  });
  return custom;
}

function translateColor(oldColor) {
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

function translateOldRoom(oldConfig) {
  const partial_linear_bitrate = [
    {size: 0, bitrate: 0},
    {size: 76800, bitrate: 400},  //320*240, 30fps
    {size: 307200, bitrate: 800}, //640*480, 30fps
    {size: 921600, bitrate: 2000},  //1280*720, 30fps
    {size: 2073600, bitrate: 4000}, //1920*1080, 30fps
    {size: 8294400, bitrate: 16000} //3840*2160, 30fps
  ];

  const standardBitrate = (width, height, framerate) => {
    let bitrate = -1;
    let prev = 0;
    let next = 0;
    let portion = 0.0;
    let def = width * height * framerate / 30;

    // find the partial linear section and calculate bitrate
    for (var i = 0; i < partial_linear_bitrate.length - 1; i++) {
      prev = partial_linear_bitrate[i].size;
      next = partial_linear_bitrate[i+1].size;
      if (def > prev && def <= next) {
        portion = (def - prev) / (next - prev);
        bitrate = partial_linear_bitrate[i].bitrate + (partial_linear_bitrate[i+1].bitrate - partial_linear_bitrate[i].bitrate) * portion;
        break;
      }
    }

    // set default bitrate for over large resolution
    if (-1 == bitrate) {
      bitrate = 16000;
    }

    return bitrate;
  }

  const calcDefaultBitrate = (codec, resolution, framerate, motionFactor) => {
    let codec_factor = 1.0;
    switch (codec) {
      case 'h264':
        codec_factor = 1.0;
        break;
      case 'vp8':
        codec_factor = 1.0;
        break;
      case 'vp9':
        codec_factor = 0.8;//FIXME: Theoretically it should be 0.5, not appliable before encoder is improved.
        break;
      case 'h265':
        codec_factor = 0.9;//FIXME: Theoretically it should be 0.5, not appliable before encoder is improved.
        break;
      default:
        break;
    }
    return standardBitrate(resolution.width, resolution.height, framerate) * codec_factor * motionFactor;
  };

  var config = {
    _id: oldConfig._id,
    name: oldConfig.name,
    inputLimit: oldConfig.publishLimit,
    participantLimit: oldConfig.userLimit,
    roles: [
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
    ],
    views: [],
    mediaIn: {
      audio: [{
          codec: 'opus',
          sampleRate: 48000,
          channelNum: 2
        }, {
          codec: 'pcmu'
        }, {
          codec: 'pcma'
        }, {
          codec: 'isac',
          sampleRate: 16000
        }, {
          codec: 'isac',
          sampleRate: 32000
        }, {
          codec: 'ilbc'
        }, {
          codec: 'g722',
          sampleRate: 16000,
          channelNum: 1
        }, {
          codec: 'g722',
          sampleRate: 16000,
          channelNum: 2
        }, {
          codec: 'aac'
        }, {
          codec: 'ac3'
        }, {
          codec: 'nellymoser'
        }
      ],
      video: [{
          codec: 'h264'
        }, {
          codec: 'vp8'
        }, {
          codec: 'h265'
        }, {
          codec: 'vp9'
        }
      ]
    },
    mediaOut: {
      audio: [{
          codec: 'opus',
          sampleRate: 48000,
          channelNum: 2
        }, {
          codec: 'pcmu'
        }, {
          codec: 'pcma'
        }, {
          codec: 'isac',
          sampleRate: 16000
        }, {
          codec: 'isac',
          sampleRate: 32000
        }, {
          codec: 'ilbc'
        }, {
          codec: 'g722',
          sampleRate: 16000,
          channelNum: 1
        }, {
          codec: 'g722',
          sampleRate: 16000,
          channelNum: 2
        }, {
          codec: 'aac',
          sampleRate: 48000,
          channelNum: 2
        }, {
          codec: 'ac3'
        }, {
          codec: 'nellymoser'
        }
      ],
      video: {
        format: [{
            codec: 'h264',
            /*
            profile: 'constrained-baseline'
          }, {
            codec: 'h264',
            profile: 'high'
            */
          }, {
            codec: 'vp8'
          }, {
            codec: 'h265'
          }, {
            codec: 'vp9'
          }
        ],
        parameters: {
          resolution: ['x3/4', 'x2/3', 'x1/2', 'x1/3', 'x1/4', 'hd1080p', 'hd720p', 'svga', 'vga', 'cif'/*, 'xvga', 'hvga', 'cif', 'sif', 'qcif'*/],
          framerate: [6, 12, 15, 24, 30, 48, 60],
          bitrate: ['x0.8', 'x0.6', 'x0.4', 'x0.2'],
          keyFrameInterval: [100, 30, 5, 2, 1]
        }
      }
    },
    transcoding: {
      audio: true,
      video: {
        format: true,
        //parameters: false
        parameters: {
          resolution: true,
          framerate: true,
          bitrate: true,
          keyFrameInterval: true
        }
      }
    },
    notifying: {
      participantActivities: true,
      streamChange: true
    },
    sip: !!oldConfig.sip ? oldConfig.sip : undefined
  };

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

  var qualityLevel2Factor = {
    'best_quality': 1.4,
    'better_quality': 1.2,
    'standard': 1.0,
    'better_speed': 0.8,
    'best_speed': 0.6
  };
  function getResolution(old_resolution) {
     return resolutionName2Value[old_resolution] || {width: 640, height: 480};
  }

  function calcBaseBitrate(codec, old_resolution, framerate, motionFactor, old_quality_level) {
    var standard_bitrate = calcDefaultBitrate(codec, getResolution(old_resolution), framerate, motionFactor);
    var mul_factor = qualityLevel2Factor[old_quality_level] || 1.0;
    return standard_bitrate * mul_factor;
  }

  for (var v in oldConfig.views) {
    var old_view = oldConfig.views[v].mediaMixing;
    config.views.push({
      label: v,
      audio: {
        format: {
          codec: 'opus',
          sampleRate: 48000,
          channelNum: 2
        },
        vad: (!!old_view.video && old_view.video.avCoordinated) ? true : false
      },
      video: (!!old_view.video ? {
        format: {
          codec: 'h264',
          profile: 'high'
        },
        motionFactor: 0.8,
        parameters: {
          resolution: getResolution(old_view.video.resolution),
          framerate: 24,
          bitrate: calcBaseBitrate('h264', old_view.video.resolution, 24, 0.8, old_view.video.quality_level),
          keyFrameInterval: 100
        },
        maxInput: old_view.video.maxInput,
        bgColor: translateColor(old_view.video.bkColor),
        layout: {
          fitPolicy: old_view.video.crop ? 'crop' : 'letterbox',
          setRegionEffect: 'insert',
          templates: {
            base: old_view.video.layout.base,
            custom: translateCustom(old_view.video.layout.custom)
          }
        }
      } : undefined)
    });
  }

  return config;
};

var todo = 10;
var rCount = 0;
var sCount = 0;

function checkClose() {
  if (rCount === todo && sCount === todo) {
    console.log('Finish');
    db.close();
  }
}

function processRoom(rooms, i) {
  if (i >= rooms.length) {
    rCount++;
    return checkClose();
  }

  var room = translateOldRoom(rooms[i]);
  db.rooms.save(room, function (err, saved) {
    if (err) {
      console.log('Error in updating room:', room._id, err.message);
    } else {
      console.log('Room:', room._id, 'converted.');
    }
    processRoom(rooms, i + 1);
  });
}

function processServices() {
  db.services.find({}).toArray(function (err, services) {
    console.log('Starting');
    if (err) {
      console.log('Error in getting services:', err.message);
      db.close();
      return;
    }

    todo = services.length;
    var i, service;
    for (i = 0; i < todo; i++) {
      service = services[i];
      if (typeof service.__v === 'number') {
        // Already New Version
        sCount++;
        rCount++;
        checkClose();
        continue;
      }
      if (service.rooms && service.rooms.length > 0) {
         processRoom(service.rooms, 0);
      } else {
         rCount++;
      }

      service.__v = 0;
      service.rooms = service.rooms.map((room) => room._id);
      db.services.save(service, function (e, saved) {
        if (e) {
          console.log('Error in updating service:', service._id, e.message);
        } else {
          console.log('Service:', service._id, 'converted.');
        }
        sCount++;
        checkClose();
      });
    }
  });
}

var rl = require('readline').createInterface({
  input: process.stdin,
  output: process.stdout
});

rl.question('This script will overwrite the old schema data, you may want backup your database before continuing. Continue?[y/n]' ,(answer) => {
  rl.close();

  answer = answer.toLowerCase();
  if (answer === 'y' || answer === 'yes') {
    processServices();
  }
});

