// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

const isHWAccAppliable = () => {
  // Query the hardware capability only if we want to try it.
  var info = '';
  try {
    info = require('child_process').execSync('vainfo', {env: process.env, stdio: ['ignore', 'pipe', 'pipe']}).toString();
  } catch (error) {
    if (error && error.code !== 0) {
      return false;
    } else {
      info = error.stderr.toString();
    }
  }
  return (info.indexOf('VA-API version') != -1) ? true : false;
}

module.exports.detected = (requireHWAcc) => {
  /*FIXME: should be double checked whether hardware acceleration is actually running*/
  var useHW = false;
  var codecs = {
    decode: ['vp8', 'vp9', 'h264', 'h265'],
    encode: ['vp8', 'vp9']
  };
  
  if (requireHWAcc && isHWAccAppliable()) {
    useHW = true;
    codecs.encode.push('h265');
    codecs.encode.push('h264_CB');
    codecs.encode.push('h264_B');
    codecs.encode.push('h264_M');
    codecs.encode.push('h264_H');
  } else {
    const fs = require('fs');
    if (fs.existsSync('./lib/libopenh264.so.4') && (fs.statSync('./lib/libopenh264.so.4').size > 100000)) {
      //FIXME: The detection of installation of openh264 is not accurate here.
      codecs.encode.push('h264_CB');
      codecs.encode.push('h264_B'); //FIXME: This is a workround for the profile compability issue, should be removed and fix it by adding accurate profile selecting logic in conference controller.
    }

    if (fs.existsSync('./lib/libSvtHevcEnc.so.1') && (fs.statSync('./lib/libSvtHevcEnc.so.1').size > 100000)) {
      codecs.encode.push('h265');
    }
  }

  return {
    hw: useHW,
    codecs: codecs
  };
}
