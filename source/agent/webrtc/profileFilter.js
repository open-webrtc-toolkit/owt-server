// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const transform = require('sdp-transform');
const logger = require('../logger').logger;
const log = logger.getLogger('Sdp');

const h264ProfileOrder = ['E', 'H', 'M', 'B', 'CB'];

function translateProfile (profLevId) {
  const profile_idc = profLevId.substr(0, 2);
  const profile_iop = parseInt(profLevId.substr(2, 2), 16);
  let profile;
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

exports.filterVP9 = function (sdpObj, videoPreference = {}, mid) {
  const removePayloads = [];
  const vp9Payloads = new Set();

  const mediaInfo = sdpObj.media.find(
    media => media.mid.toString() === mid.toString());
  if (mediaInfo) {
    let i, rtp, fmtp;
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
  }
};

exports.filterH264 = function (sdpObj, videoPreference = {}, mid) {
  const removePayloads = [];
  let selectedPayload = -1;
  let preferred = null;
  let optionals = [];
  let finalProfile = null;
  let firstPayload = -1;

  if (videoPreference) {
    preferred = videoPreference.preferred;
    if (preferred && preferred.codec !== 'h264') {
      preferred = null;
    }
    optionals = videoPreference.optional || [];
    optionals = optionals.filter((fmt) => (fmt.codec === 'h264'));
  }

  const mediaInfo = sdpObj.media.find(
    media => media.mid.toString() === mid.toString());
  if (mediaInfo) {
    let i, rtp, fmtp;
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
            const params = transform.parseParams(fmtp.config);
            const prf = translateProfile(params['profile-level-id'].toString());
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
            if (mediaInfo.direction === 'sendonly') {
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

      if (mediaInfo.direction === 'recvonly' && selectedPayload === -1) {
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
  }

  return finalProfile;
}
