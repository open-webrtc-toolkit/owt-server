#!/usr/bin/env node
'use strict';

var path = require('path');
if (!process.env.LD_LIBRARY_PATH) {
  process.env.LD_LIBRARY_PATH = ['woogeen_base', 'mcu', 'erizo/src/erizo'].map(function (dir) {
    return path.resolve(__dirname, '../../core/build/'+dir);
  }).join(':');
  require('child_process').fork(__dirname+'/module_test.js');
} else {
  ['../audio/audioMixer',
   'internalIO',
   '../access/mediaFileIO',
   'mediaFrameMulticaster',
   '../access/rtspIn',
   '../access/rtspOut',
   '../video/videoMixer',
   '../access/webrtc'].map(function (module) {
      try {
        require(__dirname+'/'+module+'/build/Release/'+path.basename(module));
        console.log('[PASS]', path.basename(module));
      } catch (e) {
        console.log('[FAIL]', path.basename(module), e);
      }
  });
}