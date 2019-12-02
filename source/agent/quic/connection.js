// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

'use strict';
const { EventEmitter } = require('events');
const path = require('path');

const addon = require('./build/Release/quic');

const cipher = require('../cipher');
const logger = require('../logger').logger;
logger.objectToLog = JSON.stringify;
const log = logger.getLogger('Connection');

const CONN_INITIAL        = 101;
const CONN_STARTED        = 102;
const CONN_GATHERED       = 103;
const CONN_READY          = 104;
const CONN_FINISHED       = 105;
const CONN_CANDIDATE      = 201;
const CONN_SDP            = 202;
const CONN_SDP_PROCESSED  = 203;
const CONN_FAILED         = 500;
const WARN_BAD_CONNECTION = 502;

const mediaConfig = require('./mediaConfig');

class Connection extends EventEmitter {
  constructor (id, threadPool, ioThreadPool, options = {}) {
    super();
    log.info(`message: Connection, id: ${id}`);
    this.id = id;
    this.threadPool = threadPool;
    this.ioThreadPool = ioThreadPool;
    this.mediaConfiguration = 'default';
    this.mediaStreams = new Map();
    this.initialized = false;
    this.options = options;
    this.ipAddresses = options.ipAddresses || '';
    this.trickleIce = options.trickleIce || false;
    this.metadata = this.options.metadata || {};
    this.isProcessingRemoteSdp = false;
    this.ready = false;
    this.wrtc = this._createWrtc();
  }

  _getMediaConfiguration(mediaConfiguration = 'default') {
    if (mediaConfig && mediaConfig.default) {
        return JSON.stringify(mediaConfig.default);
    } else {
      log.warn(
        'message: Bad media config file. You need to specify a default codecConfiguration.'
      );
      return JSON.stringify({});
    }
  }

  _createWrtc() {
    var wrtc = new addon.WebRtcConnection(
      this.threadPool, this.ioThreadPool, this.id,
      global.config.webrtc.stunserver,
      global.config.webrtc.stunport,
      global.config.webrtc.minport,
      global.config.webrtc.maxport,
      false, //this.trickleIce,
      this._getMediaConfiguration(this.mediaConfiguration),
      false,
      '', // turnserver,
      '', // turnport,
      '', //turnusername,
      '', //turnpass,
      '', //networkinterface
      this.ipAddresses
    );

    return wrtc;
  }

  _createMediaStream(id, options = {}, isPublisher = true) {
    log.debug(`message: _createMediaStream, connectionId: ${this.id}, ` +
              `mediaStreamId: ${id}, isPublisher: ${isPublisher}`);
    const mediaStream = new addon.MediaStream(this.threadPool, this.wrtc, id,
      options.label, this._getMediaConfiguration(this.mediaConfiguration), isPublisher);
    mediaStream.id = id;
    mediaStream.label = options.label;
    if (options.metadata) {
      // mediaStream.metadata = options.metadata;
      // mediaStream.setMetadata(JSON.stringify(options.metadata));
    }
    mediaStream.onMediaStreamEvent((type, message) => {
      this._onMediaStreamEvent(type, message, mediaStream.id);
    });
    return mediaStream;
  }

  _onMediaStreamEvent(type, message, mediaStreamId) {
    let streamEvent = {
      type: type,
      mediaStreamId: mediaStreamId,
      message: message,
    };
    this.emit('media_stream_event', streamEvent);
  }

  _maybeSendAnswer(evt, streamId, forceOffer = false) {
    if (this.isProcessingRemoteSdp) {
      return;
    }
    if (!this.alreadyGathered && !this.trickleIce) {
      return;
    }
    if (!this.latestSdp) {
      return;
    }

    const info = {type: this.options.createOffer || forceOffer ? 'offer' : 'answer', sdp: this.latestSdp};
    log.debug(`message: _maybeSendAnswer sending event, type: ${info.type}, streamId: ${streamId}`);
    this.emit('status_event', info, evt, streamId);
  }

  init(streamId) {
    if (this.initialized) {
      return false;
    }
    const firstStreamId = streamId;
    this.initialized = true;
    log.debug(`message: Init Connection, connectionId: ${this.id} `+
              `${logger.objectToLog(this.options)}`);
    this.sessionVersion = 0;

    this.wrtc.init((newStatus, mess, streamId) => {
      log.info('message: WebRtcConnection status update, ' +
               'id: ' + this.id + ', status: ' + newStatus +
                ', ' + logger.objectToLog(this.metadata) + mess);
      switch(newStatus) {
        case CONN_INITIAL:
          this.emit('status_event', {type: 'started'}, newStatus);
          break;

        case CONN_SDP_PROCESSED:
          this.isProcessingRemoteSdp = false;
          // this.latestSdp = mess;
          // this._maybeSendAnswer(newStatus, streamId);
          break;

        case CONN_SDP:
          this.latestSdp = mess;
          this._maybeSendAnswer(newStatus, streamId);
          break;

        case CONN_GATHERED:
          this.alreadyGathered = true;
          this.latestSdp = mess;
          this._maybeSendAnswer(newStatus, firstStreamId);
          break;

        case CONN_CANDIDATE:
          mess = mess.replace(this.options.privateRegexp, this.options.publicIP);
          this.emit('status_event', {type: 'candidate', candidate: mess}, newStatus);
          break;

        case CONN_FAILED:
          log.warn('message: failed the ICE process, ' + 'code: ' + WARN_BAD_CONNECTION +
                   ', id: ' + this.id);
          this.emit('status_event', {type: 'failed', sdp: mess}, newStatus);
          break;

        case CONN_READY:
          log.debug('message: connection ready, ' + 'id: ' + this.id +
                    ', ' + 'status: ' + newStatus + ' ' + mess + ',' + streamId);
          if (!this.ready) {
            this.ready = true;
            this.emit('status_event', {type: 'ready'}, newStatus);
          } else if (mess && streamId) {
            log.debug('message: simulcast, rid: ', streamId, mess);

            const ssrc = parseInt(mess);
            if (ssrc > 0) {
              if (this.simulcastInfo) {
                log.debug('sdp simulcast:', JSON.stringify(this.simulcastInfo));

                if (!this.firstRid) {
                  // first RID stream
                  this.firstRid = streamId;
                  this.wrtc.setVideoSsrcList('', [ssrc]);
                  this.wrtc.setRemoteSdp(this.latestSdp, this.id);
                  this.emit('status_event', {type: 'firstrid', rid: streamId, ssrc}, newStatus);
                } else {
                  // create stream
                  this.addMediaStream(streamId, {label: streamId}, true);
                  this.wrtc.setVideoSsrcList(streamId, [ssrc]);
                  this.wrtc.setRemoteSdp(this.latestSdp, streamId);
                  this.emit('status_event', {type: 'rid', rid: streamId, ssrc}, newStatus);
                }
              } else {
                log.warn('No simulcast info RID:', streamId);
              }
            }
          }
          break;
      }
    });
    if (this.options.createOffer) {
      log.debug('message: create offer requested, id:', this.id);
      const audioEnabled = this.options.createOffer.audio;
      const videoEnabled = this.options.createOffer.video;
      const bundle = this.options.createOffer.bundle;
      this.wrtc.createOffer(videoEnabled, audioEnabled, bundle);
    }
    this.emit('status_event', {type: 'initializing'});
    return true;
  }

  addMediaStream(id, options, isPublisher) {
    log.info(`message: addMediaStream, connectionId: ${this.id}, mediaStreamId: ${id}`);
    if (this.mediaStreams.get(id) === undefined) {
      const mediaStream = this._createMediaStream(id, options, isPublisher);
      this.wrtc.addMediaStream(mediaStream);
      this.mediaStreams.set(id, mediaStream);
    }
  }

  removeMediaStream(id) {
    if (this.mediaStreams.get(id) !== undefined) {
      this.wrtc.removeMediaStream(id);
      this.mediaStreams.get(id).close();
      this.mediaStreams.delete(id);
      log.debug(`removed mediaStreamId ${id}, remaining size ${this.getNumMediaStreams()}`);
      this._maybeSendAnswer(CONN_SDP, id, true);
    } else {
      log.error(`message: Trying to remove mediaStream not found, id: ${id}`);
    }
  }

  setRemoteSdp(sdp, streamId) {
    this.wrtc.setRemoteSdp(sdp, this.id);
  }

  setSimulcastInfo(simulcastInfo) {
    this.simulcastInfo = simulcastInfo;
  }

  setRemoteSsrc(audioSsrc, videoSsrcList, label) {
    this.wrtc.setAudioSsrc(label, audioSsrc);
    this.wrtc.setVideoSsrcList(label, videoSsrcList);
  }

  addRemoteCandidate(candidate) {
    this.wrtc.addRemoteCandidate(candidate.sdpMid, candidate.sdpMLineIndex, candidate.candidate);
  }

  removeRemoteCandidates(candidates) {
    candidates.forEach(val => {
      this.wrtc.removeRemoteCandidate('', 0, val.candidate);
    });
    this.wrtc.removeRemoteCandidate('', -1, '');
  }

  getMediaStream(id) {
    return this.mediaStreams.get(id);
  }

  getNumMediaStreams() {
    return this.mediaStreams.size;
  }

  close() {
    log.info(`message: Closing connection ${this.id}`);
    log.info(`message: WebRtcConnection status update, id: ${this.id}, status: ${CONN_FINISHED}, ` +
            `${logger.objectToLog(this.metadata)}`);
    this.mediaStreams.forEach((mediaStream, id) => {
      log.debug(`message: Closing mediaStream, connectionId : ${this.id}, `+
        `mediaStreamId: ${id}`);
      mediaStream.close();
    });
    this.wrtc.close();
    delete this.mediaStreams;
    delete this.wrtc;
  }

}
exports.Connection = Connection;
