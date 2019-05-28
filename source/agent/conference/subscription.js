// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
/*
 * {
 *   id: string(SubscriptionId),
 *   locality: {agent: string(AgentRpcId), node: string(NodeRpcId)}
 *   media: {
 *     tracks: {
 *       type: 'audio' | 'video' | 'data',
 *       from: string(StreamId) | string(TrackId),
 *       source: object(Stream),
 *       status: 'active' | 'inactive' | undefined,
 *       format: object(AudioFormat) | object(VideoFormat),
 *       parameters: { resolution, framerate, bitrate, keyFrameInterval }
 *     }
 *   },
 *   info: object(SubscriptionInfo):: {
 *     owner: string(ParticipantId),
 *     type: 'webrtc' | 'streaming' | 'recording' | 'sip' | 'analytics' | 'quic',
 *     location: {host: string(HostIPorDN), path: string(FileFullPath)} | undefined,
 *     url: string(URLofStreamingOut) | undefined
 *   }
 * }
 */

'use strict';

const log = require('./logger').logger.getLogger('Subscription');

class Subscription {

  constructor(id, media, data, locality, info) {
    this.id = id;
    this.info = info;
    this.locality = locality;
    this.media = this._upgradeMediaIfNeeded(media);
    this.data = data;
    this.origin = null;
  }

  _upgradeMediaIfNeeded(media) {
    if (!media.tracks) {
      /*
       * Early version media format: {
       *   audio: {
       *       from: string(StreamId),
       *       status: 'active' | 'inactive' | undefined,
       *       format: object(AudioFormat),
       *     } | undefined,
       *     video: {
       *       from: string(StreamId),
       *       status: 'active' | 'inactive' | undefined,
       *       format: object(VideoFormat),
       *       parameters: {
       *         resolution: object(Resolution) | undefined,
       *         framerate: number(FramerateFPS) | undefined,
       *         bitrate: number(Kbps) | undefined,
       *         keyFrameInterval: number(Seconds) | undefined,
       *         } | undefined
       *     } | undefined
       * }
       */
      log.debug('Upgrade subscription media:', JSON.stringify(media));
      const m = { tracks: [] };
      if (media.audio) {
        const {codec, sampleRate, channelNum} = media.audio;
        m.tracks.push({
          type: 'audio',
          from: media.audio.from,
          format: media.audio.format,
          status: (media.audio.status || 'active'),
        });
      }
      if (media.video) {
        m.tracks.push({
          type: 'video',
          from: media.video.from,
          format: media.video.format,
          parameters: media.video.parameters,
          status: (media.video.status || 'active'),
        });
      }
      return m;
    }
    // Set default status
    media.tracks.forEach(track => {
      track.status = 'active';
    });
    return media;
  }

  // Array of unique StreamId or TrackId of subscription
  froms() {
    return this.media.tracks.map(t => t.from)
      .filter((t, i, self) => self.findIndex(v => v.id === t.id) === i);
  }

  // Merge source format and parameters
  setSource(sourceId, stream) {
    this.origin = (this.origin || stream.info.origin);
    this.media.tracks.forEach(track => {
      if (track.from === sourceId) {
        // Extract source track from stream
        const source = stream.id === sourceId ?
          stream.media.tracks.find(t => t.type === track.type) :
          stream.media.tracks.find(t => t.id === sourceId);

        log.debug('Subscription source:', JSON.stringify(source));
        track.format = (track.format || source.format);
        if (source.id) {
          // Save trackId for subscription
          track.source = source.id;
        }
        if (track.type === 'video') {
          if (stream.type === 'mixed') {
            if (track.parameters) {
              const sourceBitrate = source.parameters.bitrate;
              const sourceResolution = source.parameters.resolution; 
              const sourceFramerate = source.parameters.framerate;
              const sourceKFI = source.parameters.keyFrameInterval;

              if (typeof track.parameters.bitrate === 'string') {
                track.parameters.bitrate = source.parameters.bitrate *
                  Number(track.parameters.bitrate.substring(1));
              } else {
                if (!track.parameters.resolution && !track.parameters.framerate) {
                  track.parameters.bitrate = track.parameters.bitrate || sourceBitrate;
                } else if (sourceBitrate) {
                  let resoRatio = 1.0;
                  if (track.parameters.resolution) {
                    resoRatio = (track.parameters.resolution.width * track.parameters.resolution.height) /
                      (sourceResolution.width * sourceResolution.height);
                  }
                  let frRatio = 1.0;
                  if (track.parameters.framerate) {
                    frRatio = track.parameters.framerate / sourceFramerate;
                  }
                  track.parameters.bitrate = track.parameters.bitrate || sourceBitrate * resoRatio * frRatio;
                }
              }
              if (source.parameters) {
                track.parameters.resolution = (track.parameters.resolution || sourceResolution);
                track.parameters.framerate = (track.parameters.framerate || sourceFramerate);
                track.parameters.keyFrameInterval = (track.parameters.keyFrameInterval || sourceKFI);
              }
            } else {
              track.parameters = source.parameters;
            }
          } else if (stream.type === 'forward') {
            if (track.parameters) {
              if (typeof track.parameters.bitrate === 'string') {
                track.parameters.bitrate = source.parameters.bitrate *
                  Number(track.parameters.bitrate.substring(1));
              }
            }
          }
        }
      }
    });
  }

  _toCtrlParameters(param) {
    const srcParam = (param || {});
    const ctrlParam = {
      bitrate: (srcParam.bitrate || 'unspecified'),
      resolution: (srcParam.resolution || 'unspecified'),
      framerate: (srcParam.framerate || 'unspecified'),
      keyFrameInterval: (srcParam.keyFrameInterval || 'unspecified'),
    };
    return ctrlParam;
  }

  // To arguments for RoomController.subscribe [{owner, id, locality, media, type}]
  toRoomCtrlSubArgs() {
    if (this.info.type === 'webrtc') {
      return this.media.tracks.map(track => {
        const media = { origin: this.origin };
        media[track.type] = {
          format: track.format,
          parameters: this._toCtrlParameters(track.parameters),
          from: (track.source || track.from),
        };
        return {
          owner: this.info.owner,
          id: track.id, // Use track ID for webrtc publication
          locality: this.locality,
          type: this.info.type,
          media
        };
      });
    } else {
      const media = { origin: this.origin };
      this.media.tracks.forEach(track => {
        if (!media[track.type]) {
          media[track.type] = {
            format: track.format,
            parameters: this._toCtrlParameters(track.parameters),
            from: (track.source || track.from),
          };
        }
      });
      return [{
        owner: this.info.owner,
        id: this.id,
        locality: this.locality,
        type: this.info.type,
        media,
        data: this.data
      }];
    }
  }
}

module.exports = {
  Subscription,
};
