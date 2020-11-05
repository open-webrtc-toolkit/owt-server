// SPDX-License-Identifier: Apache-2.0

'use strict';

/*
 * Media Formats Util
 */

const isAudioFmtEqual = (fmt1, fmt2) => {
  return (fmt1.codec === fmt2.codec) && (fmt1.sampleRate === fmt2.sampleRate) && (fmt1.channelNum === fmt2.channelNum);
};

const isVideoFmtEqual = (fmt1, fmt2) => {
  return (fmt1.codec === fmt2.codec) && (fmt1.profile === fmt2.profile);
};

const h264ProfileDict = {
  'CB': 1,
  'B': 2,
  'M': 3,
  'H': 4
};

const isVideoProfileCompatible = (curProfile, reqProfile) => {
  let curP = h264ProfileDict[curProfile],
    reqP = h264ProfileDict[reqProfile];

  return !curP || !reqP || (curP <= reqP);
};

const isVideoFmtCompatible = (curFmt, reqFmt) => {
  return (curFmt.codec === reqFmt.codec && isVideoProfileCompatible(curFmt.profile, reqFmt.profile));
};

const isAudioFmtAcceptable = (audioIn, audioInList) => {
  for (var i in audioInList) {
    if (isAudioFmtEqual(audioIn, audioInList[i])) {
      return true;
    }
  }
  return false;
};

const isVideoFmtAcceptable = (videoIn, videoInList) => {
  for (var i in videoInList) {
    if (isVideoFmtCompatible(videoIn, videoInList[i])) {
      return true;
    }
  }
  return false;
};

const NamedResolution = {
  'xga': {width: 1024, height: 768},
  'svga': {width: 800, height: 600},
  'vga': {width: 640, height: 480},
  'hvga': {width: 480, height: 320},
  'cif': {width: 352, height: 288},
  'qvga': {width: 320, height: 240},
  'qcif': {width: 176, height: 144},
  'hd720p': {width: 1280, height: 720},
  'hd1080p': {width: 1920, height: 1080}
};

const calcResolution = (x, baseResolution) => {
  if (NamedResolution[x]) {
    return NamedResolution[x];
  }
  if (x.indexOf('r') === 0 && x.indexOf('x') > 0) {
    // resolution 'r{width}x{height}'
    const xpos = x.indexOf('x');
    const width = parseInt(x.substr(1, xpos - 1)) || 65536;
    const height = parseInt(x.substr(xpos + 1)) || 65536;
    return {width, height};
  }
  if (x.indexOf('x') === 0 && x.indexOf('/') > 0) {
    // multiplier 'x{d}/{n}'
    const spos = x.indexOf('/');
    const d = parseInt(x.substr(1, spos - 1)) || 1;
    const n = parseInt(x.substr(spos + 1)) || 1;
    const floatToSize = (n) => {
      var x = Math.floor(n);
      return (x % 2 === 0) ? x : (x - 1);
    };
    return {
      width: floatToSize(baseResolution.width * d / n),
      height: floatToSize(baseResolution.height * d / n)
    };
  }

  return {width: 65536, height: 65536};
};

const calcBitrate = (x, baseBitrate) => {
  return Number(x.substring(1)) * baseBitrate;
};

const isResolutionEqual = (r1, r2) => {
  return r1 && r2 && (r1.width === r2.width) && (r1.height === r2.height);
};

module.exports = {
  isAudioFmtEqual,
  isVideoFmtEqual,
  isVideoFmtCompatible,
  isAudioFmtAcceptable,
  isVideoFmtAcceptable,
  calcResolution,
  calcBitrate,
  isResolutionEqual,
};
