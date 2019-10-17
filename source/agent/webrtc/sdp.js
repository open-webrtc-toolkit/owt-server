// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const transform = require('sdp-transform');
const logger = require('../logger').logger;
const log = logger.getLogger('Sdp');

const h264ProfileOrder = ['E', 'H', 'M', 'B', 'CB'];

function translateProfile (profLevId) {
  var profile_idc = profLevId.substr(0, 2);
  var profile_iop = parseInt(profLevId.substr(2, 2), 16);
  var profile;
  switch (profile_idc) {
    case '42':
      if (profile_iop & (1 << 6)) {
        // x1xx0000
        profile = 'CB';
      } else {
        // x0xx0000
        profile = 'B';
      }
      break;
    case '4D':
      if (profile_iop && (1 << 7)) {
        // 1xxx0000
        profile = 'CB';
      } else if (!(profile_iop && (1 << 5))) {
        profile = 'M';
      }
      break;
    case '58':
      if (profile_iop && (1 << 7)) {
        if (profile_iop && (1 << 6)) {
          profile = 'CB';
        } else {
          profile = 'B';
        }
      } else if (!(profile_iop && (1 << 6))) {
        profile = 'E';
      }
      break;
    case '64':
      (profile = 'H');
      break;
    case '6E':
      (profile_iop === 0) && (profile = 'H10');
      (profile_iop === 16) && (profile = 'H10I');
      break;
    case '7A':
      (profile_iop === 0) && (profile = 'H42');
      (profile_iop === 16) && (profile = 'H42I');
      break;
    case 'F4':
      (profile_iop === 0) && (profile = 'H44');
      (profile_iop === 16) && (profile = 'H44I');
      break;
    case '2C':
      (profile_iop === 16) && (profile = 'C44I');
      break;
    default:
      break;
  }
  return profile;
}

function filterVP9Payload(sdpObj) {
  var removePayloads = [];
  var vp9Payloads = new Set();

  sdpObj.media.forEach((mediaInfo) => {
    var i, rtp, fmtp;
    if (mediaInfo.type === 'video') {
      for (i = 0; i < mediaInfo.rtp.length; i++) {
        rtp = mediaInfo.rtp[i];
        if (rtp.codec.toLowerCase() === 'vp9') {
          vp9Payloads.add(rtp.payload);
        }
      }
      for (i = 0; i < mediaInfo.fmtp.length; i++) {
        fmtp = mediaInfo.fmtp[i];
        if (vp9Payloads.has(fmtp.payload) > -1) {
          // Remove profile-id=2 vp9
          if (fmtp.config.indexOf('profile-id=2') > -1)
            removePayloads.push(fmtp.payload);
        }
      }

      // Remove related vp9 payload
      mediaInfo.rtp = mediaInfo.rtp.filter((rtp) => {
        return removePayloads.findIndex((pl) => (pl === rtp.payload)) === -1;
      });
      mediaInfo.fmtp = mediaInfo.fmtp.filter((fmtp) => {
        return removePayloads.findIndex((pl) => (pl === fmtp.payload)) === -1;
      });
    }
  });
}

function filterH264Payload(sdpObj, formatPreference = {}, direction) {
  var removePayloads = [];
  var selectedPayload = -1;
  var preferred = null;
  var optionals = [];
  var finalProfile = null;
  var firstPayload = -1;

  if (formatPreference.video) {
    preferred = formatPreference.video.preferred;
    if (preferred && preferred.codec !== 'h264') {
      preferred = null;
    }
    optionals = formatPreference.video.optional || [];
    optionals = optionals.filter((fmt) => (fmt.codec === 'h264'));
  }

  sdpObj.media.forEach((mediaInfo) => {
    var i, rtp, fmtp;
    if (mediaInfo.type === 'video') {
      for (i = 0; i < mediaInfo.rtp.length; i++) {
        rtp = mediaInfo.rtp[i];
        if (rtp.codec.toLowerCase() === 'h264') {
          removePayloads.push(rtp.payload);
        }
      }

      for (i = 0; i < mediaInfo.fmtp.length; i++) {
        fmtp = mediaInfo.fmtp[i];
        if (removePayloads.indexOf(fmtp.payload) > -1) {
          // Skip packetization-mode=0
          if (fmtp.config.indexOf('packetization-mode=0') > -1)
            continue;

          if (fmtp.config.indexOf('profile-level-id') >= 0) {
            var params = transform.parseParams(fmtp.config);
            var prf = translateProfile(params['profile-level-id'].toString());
            if (firstPayload < 0) {
              firstPayload = fmtp.payload;
            }
            if (preferred) {
              if (!preferred.profile || preferred.profile === prf) {
                // Use preferred profile
                selectedPayload = fmtp.payload;
                finalProfile = prf;
                break;
              }
            }
            if (direction === 'in') {
              // For publish connection
              finalProfile = prf;
              selectedPayload = fmtp.payload;
              break;
            } else {
              // For subscribe connection
              if (optionals.findIndex((fmt) => (fmt.profile === prf)) > -1) {
                finalProfile = prf;
                selectedPayload = fmtp.payload;
                break;
              }
            }
          } else {
            selectedPayload = fmtp.payload;
          }
        }
      }

      if (direction === 'out' && selectedPayload === -1) {
        if (!global.config.webrtc.strict_profile_match && firstPayload > -1) {
          log.warn('No matched H264 profiles');
          selectedPayload = firstPayload;
        }
      }
      if (selectedPayload > -1) {
        i = removePayloads.indexOf(selectedPayload);
        removePayloads.splice(i, 1);
      }

      // Remove non-selected h264 payload
      mediaInfo.rtp = mediaInfo.rtp.filter((rtp) => {
        return removePayloads.findIndex((pl) => (pl === rtp.payload)) === -1;
      });
      mediaInfo.fmtp = mediaInfo.fmtp.filter((fmtp) => {
        return removePayloads.findIndex((pl) => (pl === fmtp.payload)) === -1;
      });
    }
  });

  return finalProfile;
}

function filterVideoPayload(sdpObj, videoPreference = {}) {
  var preferred = videoPreference.preferred;
  var optionals = videoPreference.optional || [];
  var selectedPayload = -1;
  var relatedPayloads = new Set();
  var reservedCodecs = ['red', 'ulpfec'];
  var codecMap = new Map();
  var payloadOrder = new Map();

  sdpObj.media.forEach((mediaInfo) => {
    var i, rtp, fmtp;
    var payloads;
    if (mediaInfo.type === 'video') {
      // Keep payload order in m line
      if (typeof mediaInfo.payloads === 'string') {
        mediaInfo.payloads.split(' ')
          .forEach((p, index) => {
            payloadOrder.set(parseInt(p), index);
          });
      }

      for (i = 0; i < mediaInfo.rtp.length; i++) {
        rtp = mediaInfo.rtp[i];
        const codec = rtp.codec.toLowerCase();
        if (reservedCodecs.includes(codec)) {
          relatedPayloads.add(rtp.payload);
        }
        codecMap.set(rtp.payload, codec);
        if (preferred && preferred.codec === codec) {
          selectedPayload = rtp.payload;
        }
        if (optionals.findIndex((fmt) => (fmt.codec === codec)) > -1) {
          if (selectedPayload < 0 ||
              payloadOrder.get(rtp.payload) < payloadOrder.get(selectedPayload)) {
            selectedPayload = rtp.payload;
          }
        }
      }
      // TODO: uncomment following code after register rtx in video receiver
      // if (!mediaInfo.simulcast) {
      //   for (i = 0; i < mediaInfo.fmtp.length; i++) {
      //     fmtp = mediaInfo.fmtp[i];
      //     if (fmtp.config.indexOf(`apt=${selectedPayload}`) > -1) {
      //       relatedPayloads.add(fmtp.payload);
      //     }
      //   }
      // }

      relatedPayloads.add(selectedPayload);
      // Remove non-selected video payload
      mediaInfo.rtp = mediaInfo.rtp.filter(
        (rtp) => relatedPayloads.has(rtp.payload));
      mediaInfo.fmtp = mediaInfo.fmtp.filter(
        (fmtp) => relatedPayloads.has(fmtp.payload));
    }
  });

  if (selectedPayload !== -1) {
    return { codec: codecMap.get(selectedPayload) };
  } else {
    return null;
  }
}

function filterAudioPayload(sdpObj, audioPreference = {}) {
  var preferred = audioPreference.preferred;
  var optionals = audioPreference.optional || [];
  var finalFmt;
  var selectedPayload = -1;
  var reservedCodecs = ['telephone-event', 'cn'];
  var relatedPayloads = new Set();
  var rtpMap = new Map();
  var payloadOrder = new Map();

  const isAudioMatchRtp = (fmt, rtp) => {
    if (fmt && fmt.codec === rtp.codec.toLowerCase()) {
      // codec matched
      if ((!fmt.sampleRate || fmt.codec === 'g722' ||
          fmt.sampleRate === rtp.rate)) {
        // rate matched
        if (!fmt.channelNum ||
            (fmt.channelNum === 1 && !rtp.encoding) ||
            (fmt.channelNum === rtp.encoding)) {
          // channel matched
          return true;
        }
      }
    }
    return false;
  };

  sdpObj.media.forEach((mediaInfo) => {
    var i, rtp, fmtp;
    if (mediaInfo.type === 'audio') {
      // Keep payload order in m line
      mediaInfo.payloads.split(' ')
        .forEach((p, index) => {
          payloadOrder.set(parseInt(p), index);
        });

      for (i = 0; i < mediaInfo.rtp.length; i++) {
        rtp = mediaInfo.rtp[i];
        rtpMap.set(rtp.payload, rtp);
        if (isAudioMatchRtp(preferred, rtp)) {
          selectedPayload = rtp.payload;
          finalFmt = preferred;
        }
        optionals.forEach((fmt) => {
          if (isAudioMatchRtp(fmt, rtp)) {
            if (selectedPayload < 0 ||
                payloadOrder.get(rtp.payload) < payloadOrder.get(selectedPayload)) {
              selectedPayload = rtp.payload;
              finalFmt = fmt;
            }
          }
        });
      }

      if (rtpMap.has(selectedPayload)) {
        const selectedRtp = rtpMap.get(selectedPayload);
        rtpMap.forEach((rtp) => {
          const codecName = rtp.codec.toLowerCase();
          if (reservedCodecs.includes(codecName) &&
              rtp.rate === selectedRtp.rate &&
              rtp.encoding === rtp.encoding) {
            relatedPayloads.add(rtp.payload);
          }
        });
        relatedPayloads.add(selectedPayload);
      }

      // Remove non-selected audio payload
      mediaInfo.rtp = mediaInfo.rtp.filter(
        (rtp) => relatedPayloads.has(rtp.payload));
      mediaInfo.fmtp = mediaInfo.fmtp.filter(
        (fmtp) => relatedPayloads.has(fmtp.payload));
    }
  });

  return finalFmt;
}

exports.getAudioSsrc = function (sdp) {
  var sdpObj = transform.parse(sdp)
  for (const media of sdpObj.media) {
    if (media.type == 'audio') {
      if (media.ssrcs && media.ssrcs[0]) {
        return media.ssrcs[0].id;
      }
    }
  }
};

exports.getVideoSsrcList = function (sdp) {
  var sdpObj = transform.parse(sdp);
  var ssrcList = [];
  for (const media of sdpObj.media) {
    if (media.type == 'video') {
      if (media.ssrcs) {
        media.ssrcs.forEach((ssrc) => {
          if (!ssrcList.includes(ssrc.id)) {
            ssrcList.push(ssrc.id);
          }
        });
      }
    }
  }
  return ssrcList;
};

exports.getSimulcastInfo = function (sdp) {
  var sdpObj = transform.parse(sdp);
  var simulcast = [];
  for (const media of sdpObj.media) {
    if (media.type == 'video') {
      if (media.rids && Array.isArray(media.rids)) {
        simulcast = media.rids;
      }
    }
  }
  return simulcast;
};

exports.hasCodec = function (sdp, codecName) {
  var sdpObj = transform.parse(sdp);
  var ret = false;
  for (const mediaInfo of sdpObj.media) {
    for (const rtp of mediaInfo.rtp) {
      const codec = rtp.codec.toLowerCase();
      if (codec === codecName.toLowerCase()) {
        return true;
      }
    }
  }
  return false;
};

exports.getExtId = function (sdp, extUri) {
  var sdpObj = transform.parse(sdp);
  for (const mediaInfo of sdpObj.media) {
    for (const extInfo of mediaInfo.ext) {
      if (extInfo.uri === extUri) {
        return extInfo.value;
      }
    }
  }
  return -1;
};

exports.processOffer = function (sdp, preference = {}, direction) {
  var sdpObj = transform.parse(sdp);
  filterVP9Payload(sdpObj);
  var finalProfile = filterH264Payload(sdpObj, preference, direction);
  var audioFormat = filterAudioPayload(sdpObj, preference.audio);
  var videoFormat = filterVideoPayload(sdpObj, preference.video);
  if (videoFormat && videoFormat.codec === 'h264') {
    videoFormat.profile = finalProfile;
  }
  sdp = transform.write(sdpObj);

  return { sdp, audioFormat, videoFormat};
};

exports.addAudioSSRC = function (sdp, ssrc) {
  const sdpObj = transform.parse(sdp);
  if (sdpObj.msidSemantic) {
    const msid = sdpObj.msidSemantic.token;
    for (const media of sdpObj.media) {
      if (media.type == 'audio') {
        if (!media.ssrcs) {
          media.ssrcs = [
            {id: ssrc, attribute: 'cname', value: 'o/i14u9pJrxRKAsu'},
            {id: ssrc, attribute: 'msid', value: `${msid} a0`},
            {id: ssrc, attribute: 'mslabel', value: msid},
            {id: ssrc, attribute: 'label', value: `${msid}a0`},
          ];
        }
      }
    }
  }
  return transform.write(sdpObj);
};

exports.addVideoSSRC = function (sdp, ssrc) {
  const sdpObj = transform.parse(sdp);
  if (sdpObj.msidSemantic) {
    const msid = sdpObj.msidSemantic.token;
    for (const media of sdpObj.media) {
      if (media.type == 'video') {
        if (!media.ssrcs) {
          media.ssrcs = [
            {id: ssrc, attribute: 'cname', value: 'o/i14u9pJrxRKAsu'},
            {id: ssrc, attribute: 'msid', value: `${msid} v0`},
            {id: ssrc, attribute: 'mslabel', value: msid},
            {id: ssrc, attribute: 'label', value: `${msid}v0`},
          ];
        }
      }
    }
  }
  return transform.write(sdpObj);
};
