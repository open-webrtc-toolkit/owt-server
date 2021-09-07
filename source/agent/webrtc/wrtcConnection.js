// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const { EventEmitter } = require('events');

const {
  AudioFrameConstructor,
  AudioFramePacketizer,
  VideoFrameConstructor,
  VideoFramePacketizer,
  CallBase,
} = require('../rtcFrame/build/Release/rtcFrame.node');

const logger = require('../logger').logger;
const cipher = require('../cipher');
// Logger
const log = logger.getLogger('WrtcConnection');

const { Connection } = require('./connection');

const { SdpInfo } = require('./sdpInfo.js');

/*
 * This class represents a filtered stream
 * with specified SVC layers.
 */
class LayerStream {
  constructor(parent, layerId, closeCb) {
    this.id = parent.wrtc.id + '-' + parent.id + '-' + layerId;
    this.wrtc = parent.wrtc;
    this.closeCb = closeCb;

    let pos = layerId.indexOf('l');
    const spatialId = (pos >= 0) ? parseInt(layerId[pos + 1]) : -1;
    pos = layerId.indexOf('t');
    const temporalId = (pos >= 0) ? parseInt(layerId[pos + 1]) : -1;
    this.videoFrameConstructor = new VideoFrameConstructor(
      parent.wrtc.callBase, parent.videoFrameConstructor,
      layerId, spatialId, temporalId);
  }

  sender() {
    let sender = null;
    if (this.videoFrameConstructor) {
      sender = this.videoFrameConstructor.source();
      sender.parent = this.videoFrameConstructor;
    } else {
      log.error('sender error');
    }
    if (sender) {
      sender.addDestination = (track, dest) => {
        sender.parent.addDestination(dest);
      };
      sender.removeDestination = (track, dest) => {
        sender.parent.removeDestination(dest);
      };
    }
    return sender;
  }

  close() {
    this.closeCb(this);
    this.videoFrameConstructor.close();
  }
}

/*
 * WrtcStream represents a stream object
 * of WrtcConnection. It has media source
 * functions (addDestination) and media sink
 * functions (receiver) which will be used
 * in connection link-up. Each rtp-stream-id
 * in simulcast refers to one WrtcStream.
 */
class WrtcStream extends EventEmitter {

  /*
   * audio: { format, ssrc, mid, midExtId }
   * video: {
   *   format, ssrcs, mid, midExtId,
   *   transportcc, red, ulpfec, scalabilityMode
   * }
   */
  constructor(id, wrtc, direction, {audio, video, owner, enableBWE}) {
    super();
    this.id = id;
    this.wrtc = wrtc;
    this.direction = direction;
    this.audioFormat = audio ? audio.format : null;
    this.videoFormat = video ? video.format : null;
    this.audio = audio;
    this.video = video;
    this.audioFrameConstructor = null;
    this.audioFramePacketizer = null;
    this.videoFrameConstructor = null;
    this.videoFramePacketizer = null;
    this.closed = false;
    this.owner = owner;
    if (video && video.scalabilityMode) {
      this.scalabilityMode = video.scalabilityMode;
      // string => LayerStream
      this.layerStreams = new Map();
    }

    if (direction === 'in') {
      wrtc.addMediaStream(id, {label: id}, true);

      if (audio) {
        this.audioFrameConstructor = new AudioFrameConstructor();
        this.audioFrameConstructor.bindTransport(wrtc.getMediaStream(id));
        wrtc.setAudioSsrc(id, audio.ssrc);
      }
      if (video) {
        this.videoFrameConstructor = new VideoFrameConstructor(
          this._onMediaUpdate.bind(this), video.transportcc, wrtc.callBase);
        if (this.scalabilityMode) {
          const spatial = parseInt(this.scalabilityMode[1]);
          const temporal = parseInt(this.scalabilityMode[3]);
          if (spatial >= 0 && temporal >= 0) {
            // this.videoFrameConstructor.setPreferredLayers(spatial, temporal);
          }
        }
        this.videoFrameConstructor.bindTransport(wrtc.getMediaStream(id));
        wrtc.setVideoSsrcList(id, video.ssrcs);
      }

    } else {
      wrtc.addMediaStream(id, {label: id}, false);

      if (audio) {
        this.audioFramePacketizer = new AudioFramePacketizer(audio.mid, audio.midExtId);
        this.audioFramePacketizer.bindTransport(wrtc.getMediaStream(id));
        if (this.owner) {
          this.audioFramePacketizer.setOwner(this.owner);
        }
      }
      if (video) {
        this.videoFramePacketizer = new VideoFramePacketizer(
          video.red, video.ulpfec, video.transportcc, video.mid,
          video.midExtId, false, wrtc.callBase, enableBWE);
        this.videoFramePacketizer.bindTransport(wrtc.getMediaStream(id));
      }
    }
  }

  close() {
    if (this.closed) {
      return;
    }
    this.closed = true;
    // Unbind transport
    if (this.audioFramePacketizer) {
      this.audioFramePacketizer.unbindTransport();
    }
    if (this.videoFramePacketizer) {
      this.videoFramePacketizer.unbindTransport();
    }
    if (this.audioFrameConstructor) {
      this.audioFrameConstructor.unbindTransport();
    }
    if (this.videoFrameConstructor) {
      this.videoFrameConstructor.unbindTransport();
    }
    // Clear layer streams
    if (this.layerStreams) {
      this.layerStreams.forEach((stream, layerId) => {
        stream.close();
      });
      this.layerStreams.clear();
    }
    // Stop media stream
    this.wrtc.removeMediaStream(this.id);
    // Close
    if (this.audioFramePacketizer) {
      this.audioFramePacketizer.close();
    }
    if (this.videoFramePacketizer) {
      this.videoFramePacketizer.close();
    }
    if (this.audioFrameConstructor) {
      this.audioFrameConstructor.close();
    }
    if (this.videoFrameConstructor) {
      // TODO: seperate call and frame constructor in node wrapper
      this.videoFrameConstructor.close();
    }
  }

  _onMediaUpdate(jsonUpdate) {
    this.emit('media-update', jsonUpdate);
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

  sender(track) {
    let sender = null;
    if (track === 'audio' && this.audioFrameConstructor) {
      sender = this.audioFrameConstructor.source();
      sender.parent = this.audioFrameConstructor;
    } else if (track === 'video' && this.videoFrameConstructor) {
      sender = this.videoFrameConstructor.source();
      sender.parent = this.videoFrameConstructor;
    } else if (!track) {
      let parent = (this.audioFrameConstructor || this.videoFrameConstructor);
      sender = parent.source();
      sender.parent = parent;
    } else {
      log.error('sender error');
    }
    if (sender) {
      sender.addDestination = (track, dest) => {
        sender.parent.addDestination(dest);
      };
      sender.removeDestination = (track, dest) => {
        sender.parent.removeDestination(dest);
      };
    }
    return sender;
  }

  receiver(track) {
    let dest = null;
    if (track === 'audio') {
      dest = this.audioFramePacketizer;
    } else if (track === 'video') {
      dest = this.videoFramePacketizer;
    } else {
      log.error('receiver error');
    }
    return dest;
  }

  ssrc(track) {
    if (track === 'audio' && this.audioFramePacketizer) {
      return this.audioFramePacketizer.ssrc();
    }
    if (track === 'video' && this.videoFramePacketizer) {
      return this.videoFramePacketizer.ssrc();
    }
    return null;
  }

  format(track) {
    if (track === 'audio') {
      return this.audioFormat;
    } else if (track === 'video') {
      return this.videoFormat;
    }
    return null;
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
        trackUpdate = true;
      }
      if (dir === 'out' && this.audioFramePacketizer) {
        this.audioFramePacketizer.enable(action === 'on');
        trackUpdate = true;
      }
    }
    if (track === 'av' || track === 'video') {
      if (dir === 'in' && this.videoFrameConstructor) {
        this.videoFrameConstructor.enable(action === 'on');
        trackUpdate = true;
      }
      if (dir === 'out' && this.videoFramePacketizer) {
        this.videoFramePacketizer.enable(action === 'on');
        trackUpdate = true;
      }
    }
    if (trackUpdate) {
      onOk();
    } else {
      onError('No track found');
    }
  }

  // Get or create a layer stream if SVC, return LayerStream | null
  getOrCreateLayerStream({spatial, temporal}, closeCb) {
    if (!this.scalabilityMode) {
      log.info('No scalabilityMode for layer stream');
      return null;
    }
    if (!(spatial >= 0) && !(temporal >= 0)) {
      log.info('Invalid layer setting:', spatial, temporal);
      return null;
    }
    // Check layer setting with scalabilityMode
    if (spatial >= 0 && spatial >= parseInt(this.scalabilityMode[1])) {
      log.info('Set spatial:', spatial, this.scalabilityMode);
      return null;
    }
    if (temporal >= 0 && temporal >= parseInt(this.scalabilityMode[3])) {
      log.info('Set temporal:', temporal, this.scalabilityMode);
      return null;
    }

    const layerId = ((spatial >= 0) ? 'l' + spatial : '') +
      ((temporal >= 0) ? 't' + temporal : '');
    if (!this.layerStreams.has(layerId)) {
      this.layerStreams.set(layerId, new LayerStream(this, layerId, closeCb));
    }
    return this.layerStreams.get(layerId);
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


module.exports = function (spec, on_status, on_track) {
  var that = {};
  var wrtcId = spec.connectionId;
  var wrtc = null;
  var threadPool = spec.threadPool;
  var ioThreadPool = spec.ioThreadPool;
  var networkInterfaces = spec.network_interfaces;
  var owner = spec.owner;

  var remoteSdp = null;
  var localSdp = null;
  // mid => { operationId, sdpDirection, type, formatPreference, rids, enabled, finalFormat }
  var operationMap = new Map();
  // composedId => WrtcStream
  var trackMap = new Map();
  // operationId => msid
  var msidMap = new Map();
  var bweTimer = null;
  var bweInterval = 3000;

  // option = {mid, type, formatPreference, scalabilityMode}
  that.addTrackOperation = function (operationId, sdpDirection, option) {
    var ret = false;
    var {mid, type, formatPreference, scalabilityMode} = option;
    if (!operationMap.has(mid)) {
      log.debug(`MID ${mid} for operation ${operationId} add`);
      const enabled = true;
      operationMap.set(mid, {operationId, type, sdpDirection, formatPreference, enabled});
      if (scalabilityMode) {
        operationMap.get(mid).scalabilityMode = scalabilityMode;
      }
      ret = true;
    } else {
      log.warn(`MID ${mid} has mapped operation ${operationMap.get(mid).operationId}`);
    }
    return ret;
  };

  that.removeTrackOperation = function (operationId) {
    var ret = false;
    operationMap.forEach((op, mid) => {
      if (op.operationId === operationId) {
        log.debug(`MID ${mid} for operation ${operationId} remove`);
        op.enabled = false;
        destroyTransport(mid);
        ret = true;
        if (localSdp) {
          localSdp.closeMedia(mid);
        }
      }
    });
    if (msidMap.has(operationId)) {
      msidMap.delete(operationId);
    }
    return ret;
  };

  that.getTrack = function (trackId) {
    return trackMap.get(trackId);
  };

  /*
   * Given a WebRtcConnection waits for the state CANDIDATES_GATHERED for set remote SDP.
   */
  var initWebRtcConnection = function (wrtc) {
    wrtc.on('status_event', (evt, status) => {
      if (evt.type === 'answer') {
        processAnswer(evt.sdp);

        const message = localSdp.toString();
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
          type: 'ready'
        });
      }
    });
    wrtc.init(wrtcId);
  };

  const composeId = function (mid, rid) {
    return mid + ':' + rid;
  };

  const decomposeId = function (trackId) {
    const parts = trackId.split(':')[0];
    return {
      mid: parts[0],
      rid: parts[1]
    };
  }

  const setupTransport = function (mid) {
    let rids = remoteSdp.rids(mid);
    const opSettings = operationMap.get(mid);
    const direction = (opSettings.sdpDirection === 'sendonly') ? 'in' : 'out';
    const simSsrcs = remoteSdp.getLegacySimulcast(mid);
    const trackSettings = remoteSdp.getMediaSettings(mid);
    const mediaType = remoteSdp.mediaType(mid);

    trackSettings.owner = owner;
    trackSettings.enableBWE = that.enableBWE;
    if (opSettings.finalFormat) {
      trackSettings[mediaType].format = opSettings.finalFormat;
      if (opSettings.finalFormat.codec === 'vp9' && simSsrcs) {
        // Legacy simulcast for VP9 SVC
        rids = null;
        trackSettings['video'].scalabilityMode = opSettings.scalabilityMode;
      }
    }

    if (rids) {
      // Simulcast
      rids.forEach((rid, index) => {
        const trackId = composeId(mid, rid);        
        if (simSsrcs) {
          // Assign ssrcs for legacy simulcast
          trackSettings[mediaType].ssrcs = [simSsrcs[index]];
        }

        if (!trackMap.has(trackId)) {
          trackMap.set(trackId, new WrtcStream(trackId, wrtc, direction, trackSettings));
          wrtc.setRemoteSdp(remoteSdp.singleMediaSdp(mid).toString(), trackId);
          // Notify new track
          on_track({
            type: 'track-added',
            track: trackMap.get(trackId),
            operationId: opSettings.operationId,
            mid: mid,
            rid: rid
          });
        } else {
          log.warn(`Conflict trackId ${trackId} for ${wrtcId}`);
        }
      });
    } else {
      // No simulcast
      if (!trackMap.has(mid)) {
        trackMap.set(mid, new WrtcStream(mid, wrtc, direction, trackSettings));
        // Set ssrc in local sdp for out direction
        if (direction === 'out') {
          const mtype = localSdp.mediaType(mid);
          const ssrc = trackMap.get(mid).ssrc(mtype);
          if (ssrc) {
            log.debug(`Add ssrc ${ssrc} to ${mtype} in SDP for ${wrtcId}`);
            const opId = opSettings.operationId;
            let msid = msidMap.get(opId);
            if (msid) {
              localSdp.setSsrcs(mid, [ssrc], msid);
            } else {
              msid = localSdp.setSsrcs(mid, [ssrc]);
              msidMap.set(opId, msid);
            }
          }
        }
        wrtc.setRemoteSdp(remoteSdp.singleMediaSdp(mid).toString(), mid);
        // Notify new track
        on_track({
          type: 'track-added',
          track: trackMap.get(mid),
          operationId: opSettings.operationId,
          mid: mid
        });
      } else {
        log.warn(`Conflict trackId ${mid} for ${wrtcId}`);
      }
    }

    return opSettings.operationId;
  };

  const destroyTransport = function (mid) {
    if (!remoteSdp) {
      return;
    }
    const rids = remoteSdp.rids(mid);
    if (rids) {
      // Simulcast
      rids.forEach((rid, index) => {
        const trackId = composeId(mid, rid);
        if (trackMap.has(trackId)) {
          // Let track be destroyed outside
          trackMap.delete(trackId);
          on_track({type: 'track-removed', trackId});
        } else {
          log.info(`destroyTransport: No trackId ${trackId} for ${wrtcId}`);
        }
      });
    } else {
      // No simulcast
      if (trackMap.has(mid)) {
        // Let track be destroyed outside
        trackMap.delete(mid);
        on_track({type: 'track-removed', trackId: mid});
      } else {
        log.info(`destroyTransport: No trackId ${mid} for ${wrtcId}`);
      }
    }
  };

  const processOfferMedia = function (mid) {
    // Check Media MID with saved operation
    if (!operationMap.has(mid)) {
      log.warn(`MID ${mid} in offer has no mapped operations (disabled)`);
      remoteSdp.closeMedia(mid);
      return;
    }
    if (operationMap.get(mid).sdpDirection !== remoteSdp.mediaDirection(mid)) {
      log.warn(`MID ${mid} in offer has conflict direction with operation`);
      return;
    }
    if (operationMap.get(mid).type !== remoteSdp.mediaType(mid)) {
      log.warn(`MID ${mid} in offer has conflict media type with operation`);
      return;
    }
    if (operationMap.get(mid).enabled && remoteSdp.isMediaClosed(mid)) {
      log.warn(`MID ${mid} in offer has conflict closed state (disabled)`);
      operationMap.get(mid).enabled = false;
      return;
    }

    // Determine media format in offer
    if (remoteSdp.mediaType(mid) === 'audio') {
      const audioPreference = operationMap.get(mid).formatPreference;
      const audioFormat = remoteSdp.filterAudioPayload(mid, audioPreference);
      operationMap.get(mid).finalFormat = audioFormat;
    } else if (remoteSdp.mediaType(mid) === 'video') {
      const videoPreference = operationMap.get(mid).formatPreference;
      const videoFormat = remoteSdp.filterVideoPayload(mid, videoPreference);
      operationMap.get(mid).finalFormat = videoFormat;
    }
  };

  const processOffer = function (sdp) {
    if (!remoteSdp) {
      // First offer
      remoteSdp = new SdpInfo(sdp);
      // Check mid
      const mids = remoteSdp.mids();
      for (const mid of mids) {
        processOfferMedia(mid);
      }
      localSdp = remoteSdp.answer();

      // Setup transport
      let opId = null;
      for (const mid of mids) {
        if (remoteSdp.getMediaPort(mid) !== 0) {
          opId = setupTransport(mid);
        }
      }
      if (opId) {
        on_track({
          type: 'tracks-complete',
          operationId: opId
        });
      }

    } else {
      // Later offer
      const laterSdp = new SdpInfo(sdp);
      const addedMids = [];
      const removedMids = [];

      for (let cmid of laterSdp.mids()) {
        const oldMedia = remoteSdp.media(cmid);
        if (!oldMedia) {
          // Add media
          const tempSdp = laterSdp.singleMediaSdp(cmid);
          remoteSdp.mergeMedia(tempSdp);
          addedMids.push(cmid);
        } else if (laterSdp.isMediaClosed(cmid)) {
          // Remove media
          remoteSdp.closeMedia(cmid);
          removedMids.push(cmid);
        } else {
          // Treat as no change
          log.debug(`MID ${cmid} no change`);
        }
      }

      let opId = null;
      for (let mid of addedMids) {
        processOfferMedia(mid);
        localSdp.mergeMedia(remoteSdp.singleMediaSdp(mid).answer());
        if (!remoteSdp.isMediaClosed(mid)) {
          opId = setupTransport(mid);
        }
      }
      if (opId) {
        on_track({
          type: 'tracks-complete',
          operationId: opId
        });
      }

      for (let mid of removedMids) {
        localSdp.closeMedia(mid);
        // Should already be destroyed by 'removeTrackOperation'
        // destroyTransport(mid);
      }
      remoteSdp.filterMedia(laterSdp.mids());
      localSdp.filterMedia(laterSdp.mids());
      remoteSdp.setBundleMids(laterSdp.bundleMids());
      localSdp.setBundleMids(laterSdp.bundleMids());
      // Produce answer
      const message = localSdp.toString();
      log.debug('Answer SDP', message);
      on_status({type: 'answer', sdp: message});
    }
  };

  const processAnswer = function (sdpMsg) {
    if (sdpMsg) {
      // First answer from native
      networkInterfaces.forEach((i) => {
        if (i.ip_address && i.replaced_ip_address) {
          sdpMsg = sdpMsg.replace(new RegExp(i.ip_address, 'g'), i.replaced_ip_address);
        }
      });

      const tempSdp = new SdpInfo(sdpMsg);
      if (tempSdp.mids().length > 0) {
        const tempMid = tempSdp.mids()[0];
        localSdp.setMsidSemantic(tempSdp.getMsidSemantic());
        localSdp.setCredentials(tempSdp.getCredentials(tempMid));
        localSdp.setCandidates(tempSdp.getCandidates(tempMid));
      } else {
        log.warn('No mid in answer', wrtcId);
      }

    }
  };

  that.close = function () {
    if (wrtc) {
      if (wrtc.getNumMediaStreams() > 0) {
        log.warn('MediaStream remaining when closing');
        trackMap.forEach(track => {
          track.close();
        });
      }
      wrtc.callBase.close();
      wrtc.wrtc.stop();
      wrtc.close();
      wrtc = undefined;
    }
  };

  that.onSignalling = function (msg, operationId) {
    var processSignalling = function () {
      if (msg.type === 'offer') {
        log.debug('on offer:', msg.sdp);
        processOffer(msg.sdp);
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

  that.enableBWE = false;

  // Libnice collects candidates on |ipAddresses| only.
  var ipAddresses = [];
  networkInterfaces.forEach((i) => {
    if (i.ip_address) {
      ipAddresses.push(i.ip_address);
    }
  });
  wrtc = new Connection(wrtcId, threadPool, ioThreadPool, { ipAddresses });
  wrtc.callBase = new CallBase();
  // wrtc.addMediaStream(wrtcId, {label: ''}, direction === 'in');

  initWebRtcConnection(wrtc);
  return that;
};
