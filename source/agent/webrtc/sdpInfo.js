// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const transform = require('sdp-transform');
const logger = require('../logger').logger;
const log = logger.getLogger('SdpInfo');

const {
  filterH264,
  filterVP9,
} = require('./profileFilter');

const TransportCCUri =
  'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01';

const MidUri =
  'urn:ietf:params:rtp-hdrext:sdes:mid';

class SdpInfo {
  constructor(str) {
    this.obj = transform.parse(str);
    this.obj.media.forEach((media, i) => {
      if (media.mid === undefined) {
        log.warn(`Media ${i} missing mid`);
      }
    });
  }

  mids() {
    return this.obj.media.map(mediaInfo => mediaInfo.mid.toString());
  }

  media(mid) {
    const mediaInfo = this.obj.media.find(media => media.mid.toString() === mid);
    return (mediaInfo || null);
  }

  rids(mid) {
    const mediaInfo = this.media(mid);
    let rids = null;
    if (mediaInfo) {
      if (mediaInfo.rids) {
        rids = mediaInfo.rids.map(r => (r.id + ''));
      } else {
        // Assign [0,1...n] to legacy simulcast
        const sim = this.getLegacySimulcast(mid);
        if (sim) {
          rids = sim.map((v, i) => (i + ''));
        }
      }
    }

    return rids;
  }

  mediaType(mid) {
    const mediaInfo = this.media(mid);
    if (mediaInfo) {
      return mediaInfo.type;
    } else {
      return null;
    }
  }

  mediaDirection(mid) {
    const mediaInfo = this.media(mid);
    if (mediaInfo) {
      return mediaInfo.direction;
    } else {
      return null;
    }
  }

  filterAudioPayload(mid, preference) {
    const preferred = preference.preferred;
    const optionals = preference.optional || [];
    let finalFmt = null;
    let selectedPayload = -1;
    const reservedCodecs = ['telephone-event', 'cn'];
    const relatedPayloads = new Set();
    const rtpMap = new Map();
    const payloadOrder = new Map();

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

    const mediaInfo = this.media(mid);
    if (mediaInfo && mediaInfo.type === 'audio') {
      let rtp, fmtp;
      // Keep payload order in m line
      if (typeof mediaInfo.payloads === 'string') {
        mediaInfo.payloads.split(' ')
          .forEach((p, index) => {
            payloadOrder.set(parseInt(p), index);
          });
      }

      for (let i = 0; i < mediaInfo.rtp.length; i++) {
        rtp = mediaInfo.rtp[i];
        rtpMap.set(rtp.payload, rtp);
        if (isAudioMatchRtp(preferred, rtp)) {
          selectedPayload = rtp.payload;
          finalFmt = preferred;
          break;
        }
        let optIndex = optionals.findIndex(fmt => isAudioMatchRtp(fmt, rtp));
        if (optIndex > -1) {
          if (selectedPayload < 0 ||
              payloadOrder.get(rtp.payload) < payloadOrder.get(selectedPayload)) {
            selectedPayload = rtp.payload;
            finalFmt = optionals[optIndex];
          }
        }
      }

      if (rtpMap.has(selectedPayload)) {
        const selectedRtp = rtpMap.get(selectedPayload);
        rtpMap.forEach(rtp => {
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
      if (mediaInfo.fmtp) {
        mediaInfo.fmtp = mediaInfo.fmtp.filter(
          (fmtp) => relatedPayloads.has(fmtp.payload));
      }
      if (mediaInfo.rtcpFb) {
        mediaInfo.rtcpFb = mediaInfo.rtcpFb.filter(
          (rtcp) => relatedPayloads.has(rtcp.payload));
      }
      mediaInfo.payloads = mediaInfo.payloads.split(' ')
        .filter((p) => relatedPayloads.has(parseInt(p)))
        .filter((v, index, self) => self.indexOf(v) === index)
        .join(' ');
    }

    return finalFmt;
  }

  filterVideoPayload(mid, preference) {
    // Remove unsupported profiles
    filterVP9(this.obj, preference, mid);
    const finalPrf = filterH264(this.obj, preference, mid);
    let finalFmt = null;
    let selectedPayload = -1;
    const preferred = preference.preferred;
    const optionals = preference.optional || [];
    const relatedPayloads = new Set();
    const reservedCodecs = ['red', 'ulpfec'];
    const codecMap = new Map();
    const payloadOrder = new Map();

    const mediaInfo = this.media(mid);
    if (mediaInfo && mediaInfo.type == 'video') {
      let rtp, fmtp;
      let payloads;
      // Keep payload order in m line
      if (typeof mediaInfo.payloads === 'string') {
        mediaInfo.payloads.split(' ')
          .forEach((p, index) => {
            payloadOrder.set(parseInt(p), index);
          });
      }

      for (let i = 0; i < mediaInfo.rtp.length; i++) {
        rtp = mediaInfo.rtp[i];
        const codec = rtp.codec.toLowerCase();
        if (reservedCodecs.includes(codec)) {
          relatedPayloads.add(rtp.payload);
        }
        codecMap.set(rtp.payload, codec);
        if (preferred && preferred.codec === codec) {
          selectedPayload = rtp.payload;
          break;
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
      if (mediaInfo.fmtp) {
        mediaInfo.fmtp = mediaInfo.fmtp.filter(
          (fmtp) => relatedPayloads.has(fmtp.payload));
      }
      if (mediaInfo.rtcpFb) {
        mediaInfo.rtcpFb = mediaInfo.rtcpFb.filter(
          (rtcp) => relatedPayloads.has(rtcp.payload));
      }
      mediaInfo.payloads = mediaInfo.payloads.split(' ')
        .filter((p) => relatedPayloads.has(parseInt(p)))
        .filter((v, index, self) => self.indexOf(v) === index)
        .join(' ');
    }

    if (selectedPayload !== -1) {
      finalFmt = { codec: codecMap.get(selectedPayload) };
      if (finalFmt.codec === 'h264' && finalPrf) {
        finalFmt.profile = finalPrf;
      }
    }
    log.debug('finalFmt video:', finalFmt);
    return finalFmt;
  }

  toString() {
    return transform.write(this.obj);
  }

  getMediaPort(mid) {
    const mediaInfo = this.media(mid);
    if (mediaInfo) {
      return mediaInfo.port;
    }
    return null;
  }

  setMediaPort(mid, port) {
    const mediaInfo = this.media(mid);
    if (mediaInfo) {
      mediaInfo.port = port;
    }
  }

  getCredentials(mid) {
    const mediaInfo = this.media(mid);
    if (mediaInfo) {
      return {
        iceUfrag: mediaInfo.iceUfrag,
        icePwd: mediaInfo.icePwd,
        fingerprint: mediaInfo.fingerprint
      };
    } else {
      return {};
    }
  }

  setCredentials({iceUfrag, icePwd, fingerprint}) {
    this.obj.media.forEach(mediaInfo => {
      mediaInfo.iceUfrag = iceUfrag;
      mediaInfo.icePwd = icePwd;
      mediaInfo.fingerprint = fingerprint;
    });
  }

  getMsidSemantic() {
    return this.obj.msidSemantic;
  }

  setMsidSemantic(semantic) {
    this.obj.msidSemantic = semantic;
  }

  getCandidates(mid) {
    const mediaInfo = this.media(mid);
    if (mediaInfo) {
      return mediaInfo.candidates;
    } else {
      return [];
    }
  }

  setCandidates(candidates) {
    this.obj.media.forEach(mediaInfo => {
      mediaInfo.candidates = candidates;
    });
  }

  singleMediaSdp(mid) {
    const sdp = new SdpInfo(this.toString());
    sdp.obj.media = sdp.obj.media.filter(m => m.mid.toString() === mid);
    return sdp;
  }

  mergeMedia(sdp) {
    const mids = this.mids();
    const bundles = this.obj.groups.find(g => g.type === 'BUNDLE');
    sdp.obj.media.forEach(media => {
      if (mids.indexOf(media.mid.toString()) >= 0) {
        log.warn(`Conflict MID ${mid} to merge`);
      } else {
        this.obj.media.push(media);
        bundles.mids += ' ' + media.mid;
      }
    });
  }

  compareMedia(sdp) {
    const diffMids = [];
    this.obj.media.forEach(m1 => {
      const m2 = sdp.media(m1.mid);
      if (!m2) {
        diffMids.push(m1.mid)
      } else if (m1.port !== m2.port) {
        diffMids.push(m1.mid);
      } else if (m1.direction !== m2.direction) {
        diffMids.push(m1.mid);
      }
    });

    return diffMids.map(m => m.toString());
  }

  getMediaSettings(mid) {
    const mediaInfo = this.media(mid);
    if (!mediaInfo) {
      return null;  
    }

    const settings = {};
    const ssrcs = mediaInfo.ssrcs;
    if (mediaInfo.type === 'audio') {
      settings.audio = {};
      // Audio ssrc
      if (ssrcs && ssrcs[0] && ssrcs[0].id) {
        settings.audio.ssrc = ssrcs[0].id;
      }

    } else if (mediaInfo.type === 'video') {
      settings.video = {};
      // Video ssrcs
      if (ssrcs) {
        settings.video.ssrcs = ssrcs
          .map(s => s.id)
          .filter((v, i, self) => self.indexOf(v) === i);
      }

      // Video ulpfec, red
      if (mediaInfo.rtp) {
        if (mediaInfo.rtp.find(r => (r.codec.toLowerCase() === 'ulpfec'))) {
          settings.video.ulpfec = true;
        }
        if (mediaInfo.rtp.find(r => (r.codec.toLowerCase() === 'red'))) {
          settings.video.red = true;
        }
      }

      // Video transport-cc
      if (mediaInfo.rtcpFb && mediaInfo.ext) {
        if (mediaInfo.rtcpFb.find(r => (r.type === 'transport-cc'))) {
          const transportExt = mediaInfo.ext.find(e => e.uri === TransportCCUri);
          if (transportExt) {
            settings.video.transportcc = transportExt.value;
          }
        }
      }
    }

    const midExt = mediaInfo.ext.find(e => e.uri === MidUri);
    if (midExt) {
      settings[mediaInfo.type].mid = mid;
      settings[mediaInfo.type].midExtId = midExt.value;
    }

    return settings;
  }

  getLegacySimulcast(mid) {
    const simulcast = [];
    const media = this.media(mid);
    if (!media) {
      return null;  
    }
    if (media.ssrcGroups) {
      media.ssrcGroups.forEach(ssrcGroup => {
        // Ignore FID for legacy simulcast.
        if (ssrcGroup.semantics === 'SIM') {
          ssrcGroup.ssrcs.split(' ').forEach(ssrc => {
            const nssrc = parseInt(ssrc);
            if (nssrc > 0) {
              simulcast.push(nssrc);
            }
          });
        }
      });
    }

    if (simulcast.length > 0) {
      return simulcast;
    } else {
      return null;
    }
  }

  setSsrcs(mid, ssrcs, msid) {
    const media = this.media(mid);
    if (!media) {
      return null;
    }
    if (!msid) {
      // Generate msid
      const alphanum = '0123456789' +
        'ABCDEFGHIJKLMNOPQRSTUVWXYZ' +
        'abcdefghijklmnopqrstuvwxyz';
      const msidLength = 10;
      msid = '';
      for (let i = 0; i < msidLength; i++) {
        msid += alphanum[Math.floor(Math.random() * alphanum.length)];
      }
    }

    const mtype = (media.type === 'audio') ? 'a' : 'v';
    // Only support one ssrc now
    const ssrc = ssrcs[0];
    media.ssrcs = [
      {id: ssrc, attribute: 'cname', value: 'o/i14u9pJrxRKAsu'},
      {id: ssrc, attribute: 'msid', value: `${msid} ${mtype}0`},
      {id: ssrc, attribute: 'mslabel', value: msid},
      {id: ssrc, attribute: 'label', value: `${msid}${mtype}0`},
    ];
    log.debug('Set SSRC:', mid, JSON.stringify(media.ssrcs));
    return msid;
  }

  answer() {
    const answer = new SdpInfo(this.toString());
    answer.obj.origin = {
      username: '-',
      sessionId: '0',
      sessionVersion: 0,
      netType: 'IN',
      ipVer: 4,
      address: '127.0.0.1'
    };
    answer.obj.media.forEach(mediaInfo => {
      mediaInfo.port = 1;
      mediaInfo.rtcp = {
        port: 1,
        netType: 'IN',
        ipVer: 4,
        address: '0.0.0.0'
      };
      if (mediaInfo.setup === 'active') {
        mediaInfo.setup = 'passive';
      } else {
        mediaInfo.setup = 'active';
      }

      delete mediaInfo.iceOptions;  
      delete mediaInfo.rtcpRsize;
      if (mediaInfo.direction === 'recvonly') {
        mediaInfo.direction = 'sendonly';
      } else if (mediaInfo.direction === 'sendonly') {
        delete mediaInfo.msid;
        delete mediaInfo.ssrcGroups;
        delete mediaInfo.ssrcs;
        mediaInfo.direction = 'recvonly';
      }

      if (mediaInfo.rids && Array.isArray(mediaInfo.rids)) {
        // Reverse rids direction
        mediaInfo.rids.forEach(r => {
          r.direction = (r.direction === 'send') ? 'recv' : 'send';
        });
      }
      if (mediaInfo.simulcast) {
        // Reverse simulcast direction
        if (mediaInfo.simulcast.dir1 === 'send') {
          mediaInfo.simulcast.dir1 = 'recv';
        } else {
          mediaInfo.simulcast.dir1 = 'send';
        }
      }
    });

    return answer;
  }

}

exports.SdpInfo = SdpInfo;
