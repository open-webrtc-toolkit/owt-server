// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var webrtcAddon = require('../webrtcLib/build/Release/webrtc');
var AudioFrameConstructor = webrtcAddon.AudioFrameConstructor;
var VideoFrameConstructor = webrtcAddon.VideoFrameConstructor;
var AudioFramePacketizer = webrtcAddon.AudioFramePacketizer;
var VideoFramePacketizer = webrtcAddon.VideoFramePacketizer;

var path = require('path');
var logger = require('../logger').logger;
var cipher = require('../cipher');
// Logger
var log = logger.getLogger('WrtcConnection');

const { Connection } = require('./connection');
const {
  processOffer,
  getAudioSsrc,
  getVideoSsrcList,
  getSimulcastInfo,
  getLegacySimulcastInfo,
  hasCodec,
  getExtId,
  addAudioSSRC,
  addVideoSSRC,
} = require('./sdp');

var addon = require('../webrtcLib/build/Release/webrtc');

const TransportSeqNumUri =
    'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01';

/*
 * WrtcStream represents a stream object
 * of WrtcConnection. It has media source
 * functions (addDestination) and media sink
 * functions (receiver) which will be used
 * in connection link-up. Each rtp-stream-id
 * in simulcast refers to one WrtcStream.
 */
class WrtcStream {
  constructor(config = {}) {
    this.id = config.id;
    this.audioFrameConstructor = config.audioFrameConstructor;
    this.audioFramePacketizer = config.audioFramePacketizer;
    this.videoFrameConstructor = config.videoFrameConstructor;
    this.videoFramePacketizer = config.videoFramePacketizer;
  }

  addDestination(track, dest) {
    if (track === 'audio' && this.audioFrameConstructor) {
      this.audioFrameConstructor.addDestination(dest);
    } else if (track === 'video' && this.videoFrameConstructor) {
      this.videoFrameConstructor.addDestination(dest);
    } else {
      log.warn('Wrong track:', track);
    }
  }

  removeDestination(track, dest) {
    if (track === 'audio' && this.audioFrameConstructor) {
      this.audioFrameConstructor.removeDestination(dest);
    } else if (track === 'video' && this.videoFrameConstructor) {
      this.videoFrameConstructor.removeDestination(dest);
    } else {
      log.warn('Wrong track:' + track);
    }
  }

  receiver(track) {
    var dest;
    if (track === 'audio') {
      dest = this.audioFramePacketizer;
    }
    if (track === 'video') {
      dest = this.videoFramePacketizer;
    }
    if (!dest) {
      log.error('receiver error');
    }
    return dest;
  }

  onTrackControl(track, dir, action, onOk, onError) {
    if (['audio', 'video', 'av'].indexOf(track) < 0) {
      onError('Invalid track.');
      return;
    }

    var trackUpdate = false;
    if (track === 'av' || track === 'audio') {
      if (dir === 'in' && this.audioFrameConstructor) {
        this.audioFrameConstructor.enable(action === 'on');
      } else if (dir === 'out' && this.audioFramePacketizer) {
        this.audioFramePacketizer.enable(action === 'on');
      } else {
        onError('Ambiguous audio direction.');
        return;
      }
      trackUpdate = true;
    }
    if (track === 'av' || track === 'video') {
      if (dir === 'in' && this.videoFrameConstructor) {
        this.videoFrameConstructor.enable(action === 'on');
      } else if (dir === 'out' && this.videoFramePacketizer) {
        this.videoFramePacketizer.enable(action === 'on');
      } else {
        onError('Ambiguous video direction.');
        return;
      }
      trackUpdate = true;
    }

    if (trackUpdate) {
      onOk();
    } else {
      onError('No track found');
    }
  }

  setVideoBitrate(bitrateKBPS, onOk, onError) {
    if (this.videoFrameConstructor) {
      this.videoFrameConstructor.setBitrate(bitrateKBPS);
      return onOk();
    }
    return onError('no video track');
  };

  //FIXME: Temporarily add this interface to workround the hardware mode's absence of feedback mechanism.
  requestKeyFrame() {
    if (this.videoFrameConstructor) {
      this.videoFrameConstructor.requestKeyFrame();
    }
  };
}


module.exports = function (spec, on_status, on_mediaUpdate) {
  var that = {},
    direction = spec.direction,
    wrtcId = spec.connectionId,
    threadPool = spec.threadPool,
    ioThreadPool = spec.ioThreadPool,
    // preferredAudioCodecs = spec.preferred_audio_codecs,
    // preferredVideoCodecs = spec.preferred_video_codecs,
    networkInterfaces = spec.network_interfaces,
    audio = !!spec.media.audio,
    video = !!spec.media.video,
    formatPreference = spec.formatPreference,
    audio_fmt,
    video_fmt,
    audioFrameConstructor,
    audioFramePacketizer,
    videoFrameConstructor,
    videoFramePacketizer,
    simulcastConstructors = [],
    stream,
    simStreams = [],
    ridStreamMap = new Map(),
    isSimulcast = false,
    wrtc;

  /*
   * Given a WebRtcConnection waits for the state CANDIDATES_GATHERED for set remote SDP.
   */
  var initWebRtcConnection = function (wrtc) {
    wrtc.on('status_event', (evt, status) => {
      if (evt.type === 'answer') {
        let message = evt.sdp;
        networkInterfaces.forEach((i) => {
          if (i.ip_address && i.replaced_ip_address) {
            message = message.replace(new RegExp(i.ip_address, 'g'), i.replaced_ip_address);
          }
        });
        if (audioFramePacketizer) {
          const aSsrc = audioFramePacketizer.ssrc();
          message = addAudioSSRC(message, aSsrc);
        }
        if (videoFramePacketizer) {
          const vSsrc = videoFramePacketizer.ssrc();
          message = addVideoSSRC(message, vSsrc);
        }
        log.debug('Answer SDP', message);
        on_status({type: 'answer', sdp: message});

      } else if (evt.type === 'candidate') {
        let message = evt.candidate;
        networkInterfaces.forEach((i) => {
          if (i.ip_address && i.replaced_ip_address) {
            message = message.replace(new RegExp(i.ip_address, 'g'), i.replaced_ip_address);
          }
        });
        on_status({type: 'candidate', candidate: message});

      } else if (evt.type === 'failed') {
        log.warn('ICE failed, ', status, wrtc.id);
        on_status({type: 'failed', reason: 'Ice procedure failed.'});

      } else if (evt.type === 'ready') {
        log.debug('Connection ready, ', wrtc.wrtcId);
        on_status({
          type: 'ready',
          audio: audio_fmt ? audio_fmt : false,
          video: video_fmt ? video_fmt : false,
          simulcast: isSimulcast ? { video: true } : false
        });
      } else if (evt.type === 'rid') {
        log.info('discover id:', evt.rid, evt.ssrc);
        // bind video frame constructor here for simulcast stream
        if (video) {
          log.debug('start binding video frame constructor:', evt.rid);
          const rid = evt.rid;
          const vfc = new VideoFrameConstructor((mediaUpdate) => {
            const data = {
              rid,
              info: JSON.parse(mediaUpdate)
            };
            on_mediaUpdate(JSON.stringify(data));
          });
          simulcastConstructors.push(vfc);
          const simStream = new WrtcStream({
            audioFramePacketizer,
            videoFrameConstructor: vfc
          });
          vfc.bindTransport(wrtc.getMediaStream(rid));
          ridStreamMap.set(rid, simStream)
          // notify room about the simulfcast stream
          on_mediaUpdate(JSON.stringify({rid}));
        }
      } else if (evt.type === 'firstrid') {
        on_mediaUpdate(JSON.stringify({firstrid: evt.rid}));
      }
    });
    wrtc.init(wrtcId);
  };

  var unbindFrameConstructors = function () {
    if (audio && audioFrameConstructor) {
      audioFrameConstructor.unbindTransport();
      audioFrameConstructor.close();
      audioFrameConstructor = undefined;
    }

    if (video && simulcastConstructors.length > 0) {
      for (const vfc of simulcastConstructors) {
        vfc.unbindTransport();
        vfc.close();
      }
      simulcastConstructors = [];
    }
    if (video && videoFrameConstructor) {
      videoFrameConstructor.unbindTransport();
      videoFrameConstructor.close();
      videoFrameConstructor = undefined;
    }
  };

  var unbindFramePacketizers = function () {
    if (audio && audioFramePacketizer) {
      audioFramePacketizer.unbindTransport();
      audioFramePacketizer.close();
      audioFramePacketizer = undefined;
    }

    if (video && videoFramePacketizer) {
      videoFramePacketizer.unbindTransport();
      videoFramePacketizer.close();
      videoFramePacketizer = undefined;
    }
  };

  var checkOffer = function (sdp, on_ok, on_error) {
    var contains_audio = /(m=audio).*/g.test(sdp);
    var contains_video = /(m=video).*/g.test(sdp);
    //log.debug('checkOffer, sdp:', sdp, 'contains_audio:', contains_audio, 'contains_video:', contains_video);
    if (audio && !contains_audio) {
      return on_error('audio is required but not contained by offer sdp');
    } else if (!audio && contains_audio) {
      return on_error('audio is not required but contained by offer sdp');
    } else if (video && !contains_video) {
      return on_error('video is required but not contained by offer sdp');
    } else if (!video && contains_video) {
      return on_error('video is not required but contained by offer sdp');
    } else {
      on_ok();
    }
  };

  var processLegacySimulcast = function (sdp) {
    // Process legacy simulcast info for Safari
    log.debug('Process legacy simulcast');
    const simInfo = getLegacySimulcastInfo(sdp);
    if (simInfo.length > 1 && video) {
      isSimulcast = true;
      stream = new WrtcStream({
        audioFrameConstructor,
        videoFrameConstructor,
        audioFramePacketizer,
        videoFramePacketizer
      });
      const aSsrc = getAudioSsrc(sdp);
      wrtc.setRemoteSsrc(aSsrc, [simInfo[0]], '');
      wrtc.on('status_event', (evt, status) => {
        if (evt.type === 'ready') {
          on_mediaUpdate(JSON.stringify({firstrid: simInfo[0] + ''}));
        }
      });

      for (let i = 1; i < simInfo.length; i++) {
        const simLabel = simInfo[i] + '';
        const vfc = new VideoFrameConstructor((mediaUpdate) => {
          const data = {
            rid: simLabel,
            info: JSON.parse(mediaUpdate)
          };
          on_mediaUpdate(JSON.stringify(data));
        });
        simulcastConstructors.push(vfc);
        const simStream = new WrtcStream({
          audioFramePacketizer,
          videoFrameConstructor: vfc
        });
        wrtc.addMediaStream(simLabel, {label: simLabel}, true);
        wrtc.wrtc.setVideoSsrcList(simLabel, [simInfo[i]]);
        wrtc.wrtc.setRemoteSdp(sdp, simLabel);
        vfc.bindTransport(wrtc.getMediaStream(simLabel));
        ridStreamMap.set(simLabel, simStream)
      }
      return true;
    }
    return false;
  };

  var setupTransport = function (sdp) {
    if (direction === 'in') {
      if (audio) {
        audioFrameConstructor = new AudioFrameConstructor();
        audioFrameConstructor.bindTransport(wrtc.getMediaStream(wrtcId));
      }
      if (video) {
        const transportSeqNumExt = getExtId(sdp, TransportSeqNumUri);
        videoFrameConstructor = new VideoFrameConstructor(on_mediaUpdate, transportSeqNumExt);
        videoFrameConstructor.bindTransport(wrtc.getMediaStream(wrtcId));
      }

      if (processLegacySimulcast(sdp)) {
        return;
      }
      const aSsrc = getAudioSsrc(sdp);
      const vSsrc = getVideoSsrcList(sdp);
      const simulcastInfo = getSimulcastInfo(sdp);
      if (simulcastInfo.length > 0) {
        isSimulcast = true;
      }
      log.debug('SDP ssrc:', aSsrc, vSsrc);
      wrtc.setRemoteSsrc(aSsrc, vSsrc, '');
      wrtc.setSimulcastInfo(simulcastInfo);
    } else {
      if (audio) {
        audioFramePacketizer = new AudioFramePacketizer();
        audioFramePacketizer.bindTransport(wrtc.getMediaStream(wrtcId));
      }
      if (video) {
        const hasRed = hasCodec(sdp, 'red');
        const hasUlpfec = hasCodec(sdp, 'ulpfec');
        const transportSeqNumExt = getExtId(sdp, TransportSeqNumUri);
        videoFramePacketizer = new VideoFramePacketizer(hasRed, hasUlpfec, transportSeqNumExt);
        videoFramePacketizer.bindTransport(wrtc.getMediaStream(wrtcId));
      }
    }

    stream = new WrtcStream({
      audioFrameConstructor,
      videoFrameConstructor,
      audioFramePacketizer,
      videoFramePacketizer
    });
  };

  var callOnDefaultStream = function (funcName, ...args) {
    if (stream) {
      return stream[funcName](...args);
    } else {
      log.error('WrtcStream not ready');
      return;
    }
  };

  that.addDestination = function (...args) {
    return callOnDefaultStream('addDestination', ...args);
  };

  that.removeDestination = function (...args) {
    return callOnDefaultStream('removeDestination', ...args);
  };

  that.receiver = function (...args) {
    return callOnDefaultStream('receiver', ...args);
  };

  that.onTrackControl = function (...args) {
    return callOnDefaultStream('onTrackControl', ...args);
  };

  that.setVideoBitrate = function (...args) {
    return callOnDefaultStream('setVideoBitrate', ...args);
  };

  that.getAlternative = function (rid) {
    if (!ridStreamMap.has(rid)) {
      log.warn('No simulcast rid:', wrtcId, rid);
      return that;
    }
    return ridStreamMap.get(rid);
  };

  that.close = function () {
    if (wrtc) {
      wrtc.wrtc.stop();
      unbindFramePacketizers();
      unbindFrameConstructors();
      ridStreamMap.clear();
      wrtc.close();
      wrtc = undefined;
    }
  };

  that.onSignalling = function (msg) {
    var processSignalling = function () {
      if (msg.type === 'offer') {
        log.debug('on offer:', msg.sdp);
        checkOffer(msg.sdp, function () {
          var {
            sdp,
            audioFormat,
            videoFormat
          } = processOffer(msg.sdp, formatPreference, direction);
          msg.sdp = sdp;
          audio_fmt = audioFormat;
          video_fmt = videoFormat;
          log.debug('offer after processing:', msg.sdp);
          setupTransport(msg.sdp);
          wrtc.setRemoteSdp(msg.sdp);
        }, function (reason) {
          log.error('offer error:', reason);
          on_status({type: 'failed',reason: reason});
        });
      } else if (msg.type === 'candidate') {
        wrtc.addRemoteCandidate(msg.candidate);
      } else if (msg.type === 'removed-candidates') {
        wrtc.removeRemoteCandidates(msg.candidates);
      }
    };

    if (wrtc) {
      processSignalling();
    } else {
      // should not reach here
      log.error('wrtc is not ready');
    }
  };

  // Libnice collects candidates on |ipAddresses| only.
  var ipAddresses = [];
  networkInterfaces.forEach((i) => {
    if (i.ip_address) {
      ipAddresses.push(i.ip_address);
    }
  });
  wrtc = new Connection(wrtcId, threadPool, ioThreadPool, { ipAddresses });
  wrtc.addMediaStream(wrtcId, {label: ''}, direction === 'in');

  initWebRtcConnection(wrtc);
  return that;
};
