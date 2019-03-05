// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*FIXME: should be double checked whether hardware acceleration is actually running*/
var videoCapability = {
  decode: ['vp8', 'vp9', 'h264', 'h265'],
  encode: ['vp8', 'vp9']
};

const fs = require('fs');

const detectSWModeCapability = function () {
    if (fs.existsSync('./lib/libopenh264.so.4') && (fs.statSync('./lib/libopenh264.so.4').size > 100000)) {
        //FIXME: The detection of installation of openh264 is not accurate here.
        videoCapability.encode.push('h264_CB');
        videoCapability.encode.push('h264_B'); //FIXME: This is a workround for the profile compability issue, should be removed and fix it by adding accurate profile selecting logic in conference controller.
    }

    if (fs.existsSync('./lib/libSvtHevcEnc.so.1') && (fs.statSync('./lib/libSvtHevcEnc.so.1').size > 100000)) {
        videoCapability.encode.push('h265');
    }
};

if (global.config.video.hardwareAccelerated) {
    // Query the hardware capability only if we want to try it.
    var info = '';
    try {
        info = require('child_process').execSync('vainfo', {env: process.env, stdio: ['ignore', 'pipe', 'pipe']}).toString();
        global.config.video.hardwareAccelerated = (info.indexOf('VA-API version') != -1);
    } catch (error) {
        if (error && error.code !== 0) {
            global.config.video.hardwareAccelerated = false;
        } else {
            info = error.stderr.toString();
        }
    }
    global.config.video.hardwareAccelerated = (info.indexOf('VA-API version') != -1);
}
if (global.config.video.hardwareAccelerated) {
    videoCapability.encode.push('h265');
    videoCapability.encode.push('h264_CB');
    videoCapability.encode.push('h264_B');
    videoCapability.encode.push('h264_M');
    videoCapability.encode.push('h264_H');
} else {
    detectSWModeCapability();
}

module.exports = videoCapability;
