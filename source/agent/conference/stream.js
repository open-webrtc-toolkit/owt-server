// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
/*
 * {
 *   id: string(StreamId),
 *   type: 'forward' | 'mixed',
 *   media: {
 *     tracks: {
*       type: 'audio' | 'video',
 *       status: 'active' | 'inactive' | undefined,
 *       source: 'mic' | 'camera' | screen-cast' | 'raw-file' | 'encoded-file' | 'streaming',
 *       format: object(VideoFormat) | object(AudioFormat),
 *       parameters: {
 *         resolution: object(Resolution) | undefined,
 *         framerate: number(FramerateFPS) | undefined,
 *         bitrate: number(Kbps) | undefined,
 *         keyFrameInterval: number(Seconds) | undefined,
 *         } | undefined
 *       optional: {
 *         format: [object(VideoFormat)] | [object(AudioFormat)] | undefined,
 *         parameters: {
 *           resolution: [object(Resolution)] | undefined,
 *           framerate: [number(FramerateFPS)] | undefined,
 *           bitrate: [number(Kbps] | string('x' + Multiple) | undefined,
 *           keyFrameRate: [number(Seconds)] | undefined
 *         } | undefined
 *       } | undefined
 *     } | undefined
 *   },
 *   info: object(PublicationInfo):: {
 *     owner: string(ParticipantId),
 *     type: 'webrtc' | 'streaming' | 'sip' | 'selecting',
 *     inViews: [string(ViewLabel)],
 *     attributes: object(ExternalDefinedObj)
 *   } | object(ViewInfo):: {
 *     label: string(ViewLabel),
 *     activeInput: 'unknown' | string(StreamId),
 *     layout: [{
 *       stream: string(StreamId),
 *       region: object(Region)
 *     }]
 *   }
 * }
 */

'use strict';

const {
  isAudioFmtEqual,
  isVideoFmtEqual,
  isAudioFmtAcceptable,
  isVideoFmtAcceptable,
  calcResolution,
} = require('./formatUtil');

const isResolutionEqual = (res1, res2) => {
  if (res1 && res2) {
    return res1.width === res2.width && res1.height === res2.height;
  }
  return false;
};

const log = require('./logger').logger.getLogger('Stream');

const DEFAULT_VIDEO_PARAS = {
  resolution: { width: 640, height: 480 },
  bitrate: 500,
  framerate: 30,
  keyFrameInterval: 100
};

/* Room configuration Object */
var config = {};

/* Stream object in conference */
class Stream {

  constructor(id, type, media, data, info) {
    this.id = id;
    this.type = type;
    this.info = info;
    this.media = this._constructMedia(media);
    this.data = data;
  }

  _upgradeMediaIfNeeded(media) {
    if (!media.tracks) {
      /*
       * Early version media format: {
       *   audio: { codec, sampleRate, channelNum },
       *   video: {
       *     codec, profile,
       *     resolution, framerate, bitrate, keyFrameInterval
       *   }
       * }
       */
      log.debug('Upgrade stream media:', JSON.stringify(media));
      const m = { tracks: [] };
      if (media.audio) {
        const {codec, sampleRate, channelNum} = media.audio;
        m.tracks.push({
          type: 'audio',
          format: {codec, sampleRate, channelNum},
          status: (media.audio.status || 'active'),
          source: media.audio.source
        });
      }
      if (media.video) {
        const {codec, profile} = media.video;
        const {
          resolution,
          framerate,
          bitrate,
          keyFrameInterval
        } = media.video;
        m.tracks.push({
          type: 'video',
          format: {codec, profile},
          parameters: {
            resolution,
            framerate,
            bitrate,
            keyFrameInterval
          },
          status: (media.video.status || 'active'),
          source: media.video.source
        });
      }
      return m;
    } else {
      // Filter valid tracks
      media.tracks = media.tracks.filter(track => {
        return (track.type && track.format);
      });
      // Set default status
      media.tracks.forEach(track => {
        track.status = 'active';
      });
    }
    return media;
  }

  _addTrackOptional(track) {
    // Add optional
    log.debug('_addTrackOptional:', JSON.stringify(track));
    const mediaOut = config.mediaOut;
    track.optional = {};
    if (track.type === 'audio') {
      if (config.transcoding.audio) {
        track.optional.format = mediaOut.audio.filter(fmt =>
          !isAudioFmtEqual(fmt, track.format));
      }
    } else if (track.type === 'video') {
      if (config.transcoding.video.format) {
        track.optional.format = mediaOut.video.format.filter(fmt =>
          !isVideoFmtEqual(fmt, track.format));
      }
      const trackParameters = track.parameters || {};
      const baseParameters = Object.assign({}, DEFAULT_VIDEO_PARAS, trackParameters);
      log.debug('baseParameters:', JSON.stringify(baseParameters));

      if (config.transcoding.video.parameters.resolution) {
        track.optional.parameters = track.optional.parameters || {};
        track.optional.parameters.resolution =
          mediaOut.video.parameters.resolution
          .map(x => calcResolution(x, baseParameters.resolution))
          .filter((reso, pos, self) => (
            reso.width < baseParameters.resolution.width
            && reso.height < baseParameters.resolution.height)
            && (self.findIndex(r =>
              r.width === reso.width && r.height === reso.height) === pos))
      }
      if (config.transcoding.video.parameters.bitrate) {
        track.optional.parameters = track.optional.parameters || {};
        track.optional.parameters.bitrate = mediaOut.video.parameters.bitrate;
      }
      if (config.transcoding.video.parameters.framerate) {
        track.optional.parameters = track.optional.parameters || {};
        track.optional.parameters.framerate =
          mediaOut.video.parameters.framerate
          .filter(x => (x < baseParameters.framerate));
      }
      if (config.transcoding.video.parameters.keyFrameInterval) {
        track.optional.parameters = track.optional.parameters || {};
        track.optional.parameters.keyFrameInterval =
          mediaOut.video.parameters.keyFrameInterval
      }
    }
    return track;
  }

  _constructMedia(media) {
    media = this._upgradeMediaIfNeeded(media);
    media.tracks = media.tracks.map(t => {
      const track = this._addTrackOptional(t);
      return track;
    });
    return media;
  }

  toPortalFormat() {
    const forwardInfo = {
      owner: this.info.owner,
      type: this.info.type,
      inViews: this.info.inViews,
      attributes: this.info.attributes,
      activeInput: this.info.activeInput,
    };
    const portalFormat = {
      id: this.id,
      type: this.type,
      media: {
        // Pick track properties
        tracks: this.media.tracks.map(track => ({
          id: track.id,
          type: track.type,
          source: track.source,
          format: track.format,
          parameters: track.parameters,
          optional: track.optional,
          status: track.status,
          mid: track.mid,
          rid: track.rid,
        }))
      },
      data: this.data,
      info: (this.type === 'mixed') ? this.info : forwardInfo,
    };
    return portalFormat;
  }
}

class ForwardStream extends Stream {

  constructor(id, media, data, info, locality) {
    info.inViews = [];
    super(id, 'forward', media, data, info);
    this.locality = locality;
  }

  checkMediaError() {
    const mediaIn = config.mediaIn;
    let err = '';
    for (let track of this.media.tracks) {
      if (track.type === 'audio') {
        if (!isAudioFmtAcceptable(track.format, mediaIn.audio)) {
          err = 'Audio format unacceptable';
          break;
        }
      } else if (track.type === 'video') {
        if (!isVideoFmtAcceptable(track.format, mediaIn.video)) {
          err = 'Video format unacceptable';
          break;
        }
      } else {
        log.error(`Unexpected track type: ${track.type}`);
      }
    }
    if (!this.data && this.media.tracks.length === 0) {
      err = 'No valid tracks in stream';
    }
    return err;
  }

  // To arguments for RoomController.publish [{owner, id, locality, media, type}]
  toRoomCtrlPubArgs() {
    if (this.info.type === 'webrtc') {
      return this.media.tracks.map(track => {
        const media = { origin: this.info.origin };
        media[track.type] = Object.assign({}, track.format, track.parameters);
        return {
          owner: this.info.owner,
          id: track.id, // Use track ID for webrtc publication
          locality: this.locality,
          type: this.info.type,
          media
        };
      });
    } else {
      const media = { origin: this.info.origin };
      this.media.tracks.forEach(track => {
        if (!media[track.type]) {
          media[track.type] = Object.assign({}, track.format, track.parameters);
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

  update(info) {
    let updateTrack;
    let updateParameters;
    if (info.id) {
      updateTrack = this.media.tracks.find(t => t.id === info.id);
      updateParameters = info.video ? info.video.parameters : null;
    } else {
      // Legacy update
      updateTrack = this.media.tracks.find(t => t.type === 'video');
      updateParameters = info.video ? info.video.parameters : null;
    }

    if (updateParameters && updateParameters.resolution) {
      if (!updateTrack.parameters ||
          !isResolutionEqual(updateTrack.parameters.resolution,
            updateParameters.resolution)) {
        updateTrack.parameters = updateTrack.parameters || {};
        updateTrack.parameters.resolution = updateParameters.resolution;
        // Update optional
        this._addTrackOptional(updateTrack);
        return true;
      }
    }
    return false;
  }
}

class MixedStream extends Stream {

  constructor(id, viewLabel) {
    const view = config.views.find(v => v.label === viewLabel);
    const info = {label: viewLabel, activeInput: 'unknown', layout: []};
    const media = {};
    if (view.audio) {
      media.audio = Object.assign({}, view.audio.format);
    }
    if (view.video) {
      media.video = Object.assign({}, view.video.format, view.video.parameters);
    }
    super(id, 'mixed', media, null, info);
  }
}

class SelectedStream extends ForwardStream {

  constructor(id) {
    const media = {
      tracks: [{type:'audio', format: {codec: 'unknown'}}],
    };
    const info = {
      owner: 'unknown',
      activeInput: 'unknown',
      type: 'selecting',
    };
    super(id, media, null, info, null);
  }
}

function StreamConfigure(roomConfig) {
  // Set room configuration
  config = roomConfig;
}

module.exports = {
  ForwardStream,
  MixedStream,
  SelectedStream,
  StreamConfigure,
};
