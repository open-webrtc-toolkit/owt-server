// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const standardBitrate = (width, height, framerate) => {
  let bitrate = -1;
  let prev = 0;
  let next = 0;
  let portion = 0.0;
  let def = width * height * framerate / 30;

  const partial_linear_bitrate = [
    {size: 0, bitrate: 0},
    {size: 76800, bitrate: 400},  //320*240, 30fps
    {size: 307200, bitrate: 800}, //640*480, 30fps
    {size: 921600, bitrate: 2000},  //1280*720, 30fps
    {size: 2073600, bitrate: 4000}, //1920*1080, 30fps
    {size: 8294400, bitrate: 16000}, //3840*2160, 30fps
    {size: 33177600, bitrate: 64000} //7680*4320, 30fps
  ];
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
    bitrate = 64000;
  }
  return bitrate;
}

const calcDefaultBitrate = (codec, resolution, framerate, motionFactor) => {
  const codecFactorMap = {
    'h264': 1.0,
    'vp8': 1.2,
    'vp9': 0.8,
    'h265': 0.9,
  };
  const codecFactor = (codecFactorMap[codec] || 1.0);
  return standardBitrate(resolution.width, resolution.height, framerate) * codecFactor * motionFactor;
};

const resolution2String = (r) => {
  const resolutionValue2Name = {
    'r352x288': 'cif',
    'r640x480': 'vga',
    'r800x600': 'svga',
    'r1024x768': 'xga',
    'r640x360': 'r640x360',
    'r1280x720': 'hd720p',
    'r320x240': 'sif',
    'r480x320': 'hvga',
    'r480x360': 'r480x360',
    'r176x144': 'qcif',
    'r192x144': 'r192x144',
    'r1920x1080': 'hd1080p',
    'r3840x2160': 'uhd_4k',
    'r360x360': 'r360x360',
    'r480x480': 'r480x480',
    'r720x720': 'r720x720'
  };
  const k = 'r' + r.width + 'x' + r.height;
  return resolutionValue2Name[k] ? resolutionValue2Name[k] : k;
};

const isResolutionEqual = (r1, r2) => {
  return (typeof r1.width === 'number')
    && (typeof r1.height === 'number')
    && (r1.width === r2.width)
    && (r1.height === r2.height);
};

/*
 *@param {object} videoOption -The video part of Object(MediaSubOptions)
*/
const getVideoParameterForAddon = (videoOption) => {
  const defaultResolution = {width:640, height:480};
  const defaultFramerate = 30;
  const defaultKfi = 1000;

  const format = (videoOption.format || {});
  const parameters = (videoOption.parameters || {});

  const resolution = (parameters.resolution || defaultResolution);
  const framerate = (parameters.framerate || defaultFramerate);
  const keyFrameInterval = (parameters.keyFrameInterval || defaultKfi);

  let bitrate = calcDefaultBitrate(format.codec, resolution, framerate, 0.8);
  if (typeof parameters.bitrate === 'string') {
    let bitrateFactor = (Number(parameters.bitrate.replace('x', '')) || 1);
    bitrate *= bitrateFactor;
  } else if (typeof parameters.bitrate === 'number') {
    bitrate = parameters.bitrate;
  }

  return {
    resolution: resolution2String(resolution),
    framerate,
    keyFrameInterval,
    bitrate
  };
};

module.exports = {
  resolution2String,
  isResolutionEqual,
  calcDefaultBitrate,
  getVideoParameterForAddon,
}
