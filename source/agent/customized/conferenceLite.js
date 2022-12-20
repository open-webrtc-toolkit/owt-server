// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const {randomUUID} = require('crypto');
const _ = require('lodash');
const log = require('./logger').logger.getLogger('ConferenceLite');
const {Publication, Subscription} = require('./stateTypes');
const dataAccess = require('./data_access');
const RpcChannel = require('./rpcChannel');
const STREAM_ENGINE = 'stream-service';

const FORMAT_PREFERENCE = {
  audio: {optional: [{codec: 'opus'}]},
  video: {optional: [{codec: 'vp8'}, {codec: 'h264'}]},
};

function hasMatchedProp(o1, o2, path) {
  const v1 = _.get(o1, path);
  const v2 = _.get(o2, path);
  if (v1 && v2 && v1 !== v2) {
    return false;
  }
  return true;
}

function isAudioSettingMatch(as1, as2) {
  if (hasMatchedProp(as1, as2, 'format.codec') &&
      hasMatchedProp(as1, as2, 'format.sampleRate') &&
      hasMatchedProp(as1, as2, 'format.channelNum')) {
    return true;
  }
  return false;
}

function isVideoSettingMatch(vs1, vs2) {
  if (hasMatchedProp(vs1, vs2, 'format.codec') &&
      hasMatchedProp(vs1, vs2, 'format.profile') &&
      hasMatchedProp(vs1, vs2, 'parameters.resolution.width') &&
      hasMatchedProp(vs1, vs2, 'parameters.resolution.height') &&
      hasMatchedProp(vs1, vs2, 'parameters.framerate') &&
      hasMatchedProp(vs1, vs2, 'parameters.bitrate') &&
      hasMatchedProp(vs1, vs2, 'parameters.keyFrameInterval')) {
    return true;
  }
  return false;
}

class AggregatedStream {
  constructor(id, type) {
    this.id = id;
    this.type = type;
    this.streams = [];
    this.processor = null;
  }
  addSetting(id, setting) {
    const stream = this.streams.find((stream) => {
      if (this.type === 'audio') {
        return isAudioSettingMatch(stream.setting, setting);
      } else {
        return isVideoSettingMatch(stream.setting, setting);
      }
    });
    if (!stream) {
      this.streams.push({id, setting});
      return true;
    }
    return false;
  }
  getSetting(setting) {
    const stream = this.streams.find((stream) => {
      if (this.type === 'audio') {
        return isAudioSettingMatch(stream.setting, setting);
      } else {
        return isVideoSettingMatch(stream.setting, setting);
      }
    });
    return stream?.id || null;
  }
  getSettings() {
    return this.streams.map((stream) => {
      return stream.setting;
    });
  }
}

// Up level controller for certain domain streams
class Conference {
  static supportedMethods = [
    'join',
    'leave',
    'publish',
    'subscribe',
    'streamControl',
    'subscriptionControl',
    'onStatus',
    'addProcessor',
    'controlStream'
  ];

  constructor(rpcClient, rpcId) {
    this.rpcId = rpcId;
    this.roomId = null;
    this.participants = new Map(); // id => participant
    this.mixedStreams = new Map();
    this.videos = new Map();
    this.audios = new Map();
    this.processors = new Map();
    this.rpcChannel = RpcChannel(rpcClient);
    this.config = null;
    this.audioOptional = null;
    this.videoOptional = null;
    this.initing = null;
    this.pendingAudioTracks = new Map();
    this.pendingVideoTracks = new Map();
  }

  // Join from portal
  // req = {id, domain}
  async join(req) {
    log.debug('join:', req);
    if (!this.roomId) {
      // Init
      this.roomId = req.domain;
      this.initing = this._init(req.domain);
    }
    await this.initing;
    if (req.domain !== this.roomId) {
      throw new Error('Invalid room');
    }
    if (this.participants.has(req.id)) {
      throw new Error('Duplicate participant join');
    }
    this.participants.set(req.id, req);
    const addedStreams = [];
    for (const [,stream] of this.mixedStreams) {
      addedStreams.push(stream.toSignalingFormat());
    }
    return {
      participant: req,
      streams: addedStreams
    };
  }

  async _init(roomId) {
    log.debug('Init conference:', roomId);
    // Read room config
    const config = await dataAccess.room.config(roomId);
    this.config = config;
    log.debug('initializing room:', roomId, 'got config:', JSON.stringify(config));
    // Init media optional
    this.audioOptional = {format: config.mediaOut.audio};
    const videoOut = config.mediaOut.video;
    this.videoOptional = {
      format: videoOut.format,
      parameters: {
        resolution: videoOut.parameters.resolution,
        framerate: videoOut.parameters.framerate,
        bitrate: videoOut.parameters.bitrate,
        keyFrameInterval: videoOut.parameters.keyFrameInterval,
      }
    };
    // Init views
    const initProms = [];
    for (const view of config.views) {
      const label = view.label;
      // Mix stream ID is room ID followed by view label
      const mixedId = roomId + '-' + label;
      // Create a virtual stream as mixed
      const info = {
        streamType: 'mixed',
        optional: {
          audio: this.audioOptional,
          video: this.videoOptional
        }
      };
      const mixedPub = new Publication(mixedId, 'virtual', info);
      mixedPub.domain = roomId;
      mixedPub.participant = roomId;
      const rpcChannel = this.rpcChannel;
      if (view.audio) {
        const audioTrack = {
          id: mixedId,
          format: view.audio.format,
        };
        mixedPub.source.audio.push(audioTrack);
        // Init audio mixer
        const procReq = {
          type: 'audio',
          mixing: Object.assign(
            {id: 'audio-' + mixedId}, view.audio),
          domain: roomId,
          participant: roomId
        };
        const p = rpcChannel.makeRPC(
            STREAM_ENGINE, 'addProcessor', [procReq])
          .then((proc) => {
            this.audios.get(mixedId).processor = proc;
          });
        initProms.push(p);
        this.audios.set(mixedId, new AggregatedStream(mixedId, 'audio'));
      }
      if (view.video) {
        const videoTrack = {
          id: mixedId,
          format: view.video.format,
          parameters: view.video.parameters
        };
        mixedPub.source.video.push(videoTrack);
        // Init video mixer
        const procReq = {
          type: 'video',
          mixing: Object.assign(
            {id: 'video-' + mixedId}, view.video),
          domain: roomId,
          participant: roomId
        };
        const p = rpcChannel.makeRPC(
            STREAM_ENGINE, 'addProcessor', [procReq])
          .then((proc) => {
            this.videos.get(mixedId).processor = proc;
          });
        initProms.push(p);
        this.videos.set(mixedId, new AggregatedStream(mixedId, 'video'));
      }
      const pubReq = {
        id: mixedId,
        type: 'virtual',
        data: mixedPub,
        domain: roomId,
        participant: roomId
      };
      // initProms.push(rpcChannel.makeRPC(
      //   STREAM_ENGINE, 'publish', [pubReq]));
      this.mixedStreams.set(mixedId, mixedPub);
    }
    await Promise.all(initProms);
  }

  // Leave from portal
  async leave(req) {
    log.debug('leave:', req);
    if (!this.participants.has(req.id)) {
      throw new Error('Invalid leave ID');
    }
    // Clean up streams & subscriptions
    this.participants.delete(req.id);
  }

  // Publish from portal
  async publish(req) {
    log.debug('publish:', req);
    if (req.type === 'virtual') {
      return req;
    }
    // Validate request
    if (!this.participants.has(req.participant) &&
        req.participant !== this.roomId) {
      throw new Error('Invalid participant ID');
    }
    req.id = req.id || randomUUID().replace(/-/g, '');
    if (req.type !== 'video' && req.type !== 'audio') {
      req.info = Object.assign(req.info || {}, {
        owner: this.participant,
        type: req.type,
        attributes: req.attributes,
        formatPreference: {
          audio: {optional: this.config.mediaIn.audio},
          video: {optional: this.config.mediaIn.video},
        },
        origin: {domain: this.domain},
        optional: {
          audio: this.audioOptional,
          video: this.videoOptional
        }
      });
    }
    return req;
  }
  // Subscribe from portal
  async subscribe(req) {
    log.debug('subscribe:', req);
    // Validate request
    if (!this.participants.has(req.participant) &&
        req.participant !== this.roomId) {
      throw new Error('Invalid pariticpant ID');
    }
    req.id = req.id || randomUUID().replace(/-/g, '');
    const formatPreference = {};
    req.info = {
      owner: this.participant,
      type: req.type,
      origin: {domain: this.domain},
      formatPreference
    };
    // Check subscribe source
    if (req.type === 'webrtc') {
      // Save track from for later linkup
      for (const track of req.media.tracks) {
        const tmpTrackId = track.id ?? req.id;
        if (track.type === 'audio') {
          log.debug('Pending audio add:', tmpTrackId, track.from);
          this.pendingAudioTracks.set(tmpTrackId, track.from);
          if (!formatPreference.audio) {
            const stream = this.audios.get(track.from);
            const sourceSetting = stream?.getSettings?.()?.[0];
            formatPreference.audio = {
              preferred: sourceSetting?.format,
              optional: sourceSetting?.optional || this.audioOptional.format
            }
          }
        } else if (track.type === 'video') {
          log.debug('Pending video add:', tmpTrackId, track.from);
          this.pendingVideoTracks.set(tmpTrackId, track.from);
          if (!formatPreference.video) {
            const stream = this.videos.get(track.from);
            const sourceSetting = stream?.getSettings?.()?.[0];
            formatPreference.video = {
              preferred: sourceSetting?.format,
              optional: sourceSetting?.optional || this.videoOptional.format
            }
          }
        }
        delete track.from;
      }
    }
    return req;
  }
  // StreamControl from portal
  async streamControl(req) {
    log.debug('streamControl:', req);
    // Validate request
    if (!this.participants.has(req.participant) &&
        req.participant !== this.roomId) {
      throw new Error('Invalid pariticpant ID');
    }
    return req;
  }
  // SubscriptionControl from portal
  async subscriptionControl(req) {
    log.debug('subscriptionControl:', req);
    // Validate request
    if (!this.participants.has(req.participant) &&
        req.participant !== this.roomId) {
      throw new Error('Invalid pariticpant ID');
    }
    return req;
  }

  // Status of publication/subscription
  /*
  req = {
    type: 'publication/subscription',
    status: 'update|add|remove',
    data: publication | subscription
  }
  */
  async onStatus(req) {
    log.debug('onStatus:', req);
    try {
    if (req.type === 'publication') {
      if (req.status === 'add') {
        const pub = Publication.from(req.data)
        for (const track of pub.source.audio) {
          const audioStream = new AggregatedStream(track.id, pub.type);
          audioStream.addSetting(track.id, track);
          if (this.audios.has(audioStream.id)) {
            log.warn('Audio ID already exists:', audioStream.id);
            continue;
          }
          this.audios.set(audioStream.id, audioStream);
          if (!this.audios.has(pub.id)) {
            // Default track for publication ID
            this.audios.set(pub.id, audioStream);
          }
        }
        for (const track of pub.source.video) {
          const videoStream = new AggregatedStream(track.id, pub.type);
          videoStream.addSetting(track.id, track);
          if (this.videos.has(videoStream.id)) {
            log.warn('Video ID already exists:', videoStream.id);
            continue;
          }
          this.videos.set(videoStream.id, videoStream);
          if (!this.videos.has(pub.id)) {
            // Default track for publication ID
            this.videos.set(pub.id, videoStream);
          }
        }
      } else if (req.status === 'remove') {
        const pubId = req.data.id;
      }
    } else if (req.type === 'subscription') {
      if (req.status === 'add') {
        const sub = Subscription.from(req.data);
        let hasUpdate = false;
        // Update audio from
        for (const track of sub.sink.audio) {
          let from = null;
          if (this.pendingAudioTracks.has(track.id)) {
            from = this.pendingAudioTracks.get(track.id);
            this.pendingAudioTracks.delete(track.id);
          } else if (this.pendingAudioTracks.has(sub.id)) {
            from = this.pendingAudioTracks.get(sub.id);
            this.pendingAudioTracks.delete(sub.id);
          }
          if (from && this.audios.has(from)) {
            log.debug('Pending link from:', track.id, from);
            const stream = this.audios.get(from);
            const mappedId = stream.getSetting(track);
            if (mappedId) {
              track.from = mappedId;
            } else {
              // Generate audio
              if (!stream.processor) {
                if (this.mixedStreams.has(from)) {
                  log.error('Audio mixer is not ready');
                  throw new Error('Audio mixer is not ready');
                }
                const txReq = {
                  type: 'audio',
                  id: this.roomId + '-' + from,
                  transcoding: {id: from},
                  domain: this.roomId,
                  participant: this.roomId
                };
                stream.processor = {id: txReq.id};
                stream.processor = await this.rpcChannel.makeRPC(
                  STREAM_ENGINE, 'addProcessor', [txReq]);
                // Add input
                const audioOption = {
                  from,
                  format: stream.getSettings()[0].format,
                }
                const inputReq = {
                  type: 'audio',
                  id: randomUUID().replace(/-/g, ''),
                  media: {audio: audioOption},
                  processor: stream.processor.id,
                  participant: this.roomId,
                  info: {owner: ''}
                };
                await this.rpcChannel.makeRPC(
                  STREAM_ENGINE, 'subscribe', [inputReq]);
              }
              const genReq = {
                type: 'audio',
                id: randomUUID().replace(/-/g, ''),
                media: {audio: {format: track.format}},
                processor: stream.processor.id,
                participant: this.roomId,
                info: {owner: '', hidden: true}
              };
              const outputId = await this.rpcChannel.makeRPC(
                STREAM_ENGINE, 'publish', [genReq]);
              stream.addSetting(outputId.id, {format: track.format});
              track.from = outputId.id;
              log.debug('Update track audio from:', track.id, track.from);
            }
            hasUpdate = true;
          }
        }
        // Update video from
        for (const track of sub.sink.video) {
          let from = null;
          if (this.pendingVideoTracks.has(track.id)) {
            from = this.pendingVideoTracks.get(track.id);
            this.pendingVideoTracks.delete(track.id);
          } else if (this.pendingVideoTracks.has(sub.id)) {
            from = this.pendingVideoTracks.get(sub.id);
            this.pendingVideoTracks.delete(sub.id);
          }
          if (from && this.videos.has(from)) {
            const stream = this.videos.get(from);
            const mappedId = stream.getSetting(track);
            if (mappedId) {
              track.from = mappedId;
            } else {
              // Generate video
              if (!stream.processor) {
                if (this.mixedStreams.has(from)) {
                  log.error('Video mixer is not ready');
                  throw new Error('Video mixer is not ready');
                }
                const txReq = {
                  type: 'video',
                  id: this.roomId + '-' + from,
                  transcoding: {id: from},
                  domain: this.roomId,
                  participant: this.roomId,
                };
                stream.processor = {id: txReq.id};
                stream.processor = await this.rpcChannel.makeRPC(
                  STREAM_ENGINE, 'addProcessor', [txReq]);
                // Add input
                const videoOption = {
                  from,
                  format: stream.getSettings()[0].format,
                  parameters: stream.getSettings()[0].parameters,
                }
                const inputReq = {
                  type: 'video',
                  id: randomUUID().replace(/-/g, ''),
                  media: {video: videoOption},
                  processor: stream.processor.id,
                  participant: this.roomId,
                  info: {owner: ''}
                };
                await this.rpcChannel.makeRPC(
                  STREAM_ENGINE, 'subscribe', [inputReq]);
              }
              const videoSetting = {
                format: track.format,
                parameters: track.parameters,
              };
              const genReq = {
                type: 'video',
                id: randomUUID().replace(/-/g, ''),
                media: {video: videoSetting},
                processor: stream.processor.id,
                participant: this.roomId,
                info: {owner: '', hidden: true}
              };
              const outputId = await this.rpcChannel.makeRPC(
                STREAM_ENGINE, 'publish', [genReq]);
              stream.addSetting(outputId.id, videoSetting);
              track.from = outputId.id;
              log.debug('Update track video from:', track.id, track.from);
            }
            hasUpdate = true;
          }
        }
        // Update subscription from
        if (hasUpdate) {
          const ctrlReq = {
            type: sub.type,
            id: sub.id,
            operation: 'update',
            data: sub,
            participant: this.roomId,
          };
          // Control subscription
          await this.rpcChannel.makeRPC(
            STREAM_ENGINE, 'subscriptionControl', [ctrlReq]);
        }
      }
    } else if (req.type === 'processor') {
      //
    }

  } catch (e) {
    log.debug('Catched:', e, e.stack);
  }
  }
  // Add processor request
  async addProcessor(req) {
    req.id = req.id || randomUUID().replace(/-/g, '');
    return req;
  }
}

module.exports.Conference = Conference;
