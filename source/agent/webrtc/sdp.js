// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const transform = require('sdp-transform');

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
      (profile_iop === 0) && (profile = 'H');
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

function filterH264Payload(sdpObj, formatPreference = {}, direction) {
  var removePayloads = [];
  var selectedPayload = -1;
  var preferred = null;
  var optionals = [];
  var finalProfile = null;

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
            if (preferred) {
              if (preferred.profile && preferred.profile === prf) {
                // Use preferred profile
                selectedPayload = fmtp.payload;
                finalProfile = prf;
                break;
              }
            }
            if (direction === 'in') {
              // For publish connection
              if (h264ProfileOrder.indexOf(finalProfile) < h264ProfileOrder.indexOf(prf)) {
                finalProfile = prf;
                selectedPayload = fmtp.payload;
              }
            } else {
              // For subscribe connection
              if (optionals.findIndex((fmt) => (fmt.profile === prf)) > -1) {
                if (h264ProfileOrder.indexOf(finalProfile) < h264ProfileOrder.indexOf(prf)) {
                  finalProfile = prf;
                  selectedPayload = fmtp.payload;
                }
              }
            }
          }
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
  var finalFmt = null;
  var selectedPayload = -1;
  var relatedPayloads = new Set();
  var reservedCodecs = ['red', 'ulpfec'];
  var codecMap = new Map();

  sdpObj.media.forEach((mediaInfo) => {
    var i, rtp, fmtp;
    if (mediaInfo.type === 'video') {
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
          if (selectedPayload < 0) {
            selectedPayload = rtp.payload;
          }
        }
      }
      // Get related video payload rtx
      for (i = 0; i < mediaInfo.fmtp.length; i++) {
        fmtp = mediaInfo.fmtp[i];
        if (fmtp.config.indexOf(`apt=${selectedPayload}`) > -1) {
          relatedPayloads.add(fmtp.payload);
        }
      }

      relatedPayloads.add(selectedPayload);
      // Remove non-selected video payload
      mediaInfo.rtp = mediaInfo.rtp.filter(
        (rtp) => relatedPayloads.has(rtp.payload));
      mediaInfo.fmtp = mediaInfo.fmtp.filter(
        (fmtp) => relatedPayloads.has(fmtp.payload));
    }
  });

  return { codec: codecMap.get(selectedPayload) };
}

function filterAudioPayload(sdpObj, audioPreference = {}) {
  var preferred = audioPreference.preferred;
  var optionals = audioPreference.optional || [];
  var finalFmt = null;
  var selectedPayload = -1;
  var reservedCodecs = ['telephone-event', 'cn'];
  var relatedPayloads = new Set();
  var rtpMap = new Map();

  sdpObj.media.forEach((mediaInfo) => {
    var i, rtp, fmtp;
    if (mediaInfo.type === 'audio') {
      for (i = 0; i < mediaInfo.rtp.length; i++) {
        rtp = mediaInfo.rtp[i];
        rtpMap.set(rtp.payload, rtp);
        const codec = rtp.codec.toLowerCase();
        if (preferred && preferred.codec === codec) {
          selectedPayload = rtp.payload;
          finalFmt = preferred;
        }
        optionals.forEach((fmt) => {
          if (fmt.codec === codec && selectedPayload < 0) {
            selectedPayload = rtp.payload;
            finalFmt = fmt;
          }
        });
      }

      if (rtpMap.has(selectedPayload)) {
        const selectedRtp = rtpMap.get(selectedPayload);
        rtpMap.forEach((rtp) => {
          const codecName = rtp.codec.toLowerCase();
          if (reservedCodecs.includes(codecName) &&
              rtp.rate === selectedRtp.rate) {
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
      if (media.simulcast) {
        const simInfo = media.simulcast;
        if (simInfo.dir1 === 'send') {
          simulcast = transform.parseSimulcastStreamList(simInfo.list1);
        }
      }
    }
  }
  return simulcast;
}

exports.processOffer = function (sdp, preference = {}, direction) {
  var sdpObj = transform.parse(sdp);
  var finalProfile = filterH264Payload(sdpObj, preference, direction);
  var audioFormat = filterAudioPayload(sdpObj, preference.audio);
  var videoFormat = filterVideoPayload(sdpObj, preference.video);
  sdp = transform.write(sdpObj);

  return { sdp, audioFormat, videoFormat};
};
