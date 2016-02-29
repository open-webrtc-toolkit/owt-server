#!/usr/bin/env node
'use strict';

if (!process.env.LD_LIBRARY_PATH) {
  process.env.LD_LIBRARY_PATH = require('path').resolve(__dirname, '../../dist/lib');
  require('child_process').fork(__dirname+'/module_test.js');
} else {
  ['audioMixer',
   'internalIO',
   'mediaFileIO',
   'mediaFrameMulticaster',
   'rtspIn',
   'rtspOut',
   'videoMixer',
   'webrtc'].map(function (module) {
      try {
        require(__dirname+'/'+module+'/build/Release/'+module);
        console.log('[PASS]', module);
      } catch (e) {
        console.log('[FAIL]', module, e);
      }
  });
}