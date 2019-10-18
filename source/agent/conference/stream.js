// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
/*
 * {
 *   id: string(StreamId),
 *   type: 'forward' | 'mixed',
 *   media: {
 *     audio: {
 *       status: 'active' | 'inactive' | undefined,
 *       source: 'mic' | 'screen-cast' | 'raw-file' | 'encoded-file' | 'streaming',
 *       format: object(AudioFormat),
 *       optional: {
 *         format: [object(AudioFormat)] | undefined
 *       } | undefined
 *     } | undefined,
 *     video: {
 *       status: 'active' | 'inactive' | undefined,
 *       source: 'camera' | screen-cast' | 'raw-file' | 'encoded-file' | 'streaming',
 *       format: object(VideoFormat),
 *       parameters: {
 *         resolution: object(Resolution) | undefined,
 *         framerate: number(FramerateFPS) | undefined,
 *         bitrate: number(Kbps) | undefined,
 *         keyFrameInterval: number(Seconds) | undefined,
 *         } | undefined
 *       optional: {
 *         format: [object(VideoFormat)] | undefined,
 *         parameters: {
 *           resolution: [object(Resolution)] | undefined,
 *           framerate: [number(FramerateFPS)] | undefined,
 *           bitrate: [number(Kbps] | string('x' + Multiple) | undefined,
 *           keyFrameRate: [number(Seconds)] | undefined
 *         } | undefined
 *       } | undefined
 *       alternative: [{
 *         resolution: object(Resolution) | undefined,
 *         framerate: number(FramerateFPS) | undefined,
 *         bitrate: number(Kbps) | undefined,
 *         keyFrameInterval: number(Seconds) | undefined,
 *       }] | undefined
 *     } | undefined
 *   },
 *   info: object(PublicationInfo):: {
 *     owner: string(ParticipantId),
 *     type: 'webrtc' | 'streaming' | 'sip',
 *     inViews: [string(ViewLabel)],
 *     attributes: object(ExternalDefinedObj)
 *   } | object(ViewInfo):: {
 *     label: string(ViewLabel),
 *     layout: [{
 *       stream: string(StreamId),
 *       region: object(Region)
 *     }]
 *   }
 * }
 */

'use strict';

const isAudioFmtEqual = (fmt1, fmt2) => {
  return (fmt1.codec === fmt2.codec) &&
      (fmt1.sampleRate === fmt2.sampleRate) &&
      (fmt1.channelNum === fmt2.channelNum);
};

const isVideoFmtEqual = (fmt1, fmt2) => {
  return (fmt1.codec === fmt2.codec &&
      fmt1.profile === fmt2.profile);
};

const NamedResolution = {
  'xga': {width: 1024, height: 768},
  'svga': {width: 800, height: 600},
  'vga': {width: 640, height: 480},
  'hvga': {width: 480, height: 320},
  'cif': {width: 352, height: 288},
  'qvga': {width: 320, height: 240},
  'qcif': {width: 176, height: 144},
  'hd720p': {width: 1280, height: 720},
  'hd1080p': {width: 1920, height: 1080}
};

const calcResolution = (x, baseResolution) => {
  if (NamedResolution[x]) {
    return NamedResolution[x];
  }
  if (x.indexOf('r') === 0 && x.indexOf('x') > 0) {
    // resolution 'r{width}x{height}'
    const xpos = x.indexOf('x');
    const width = parseInt(x.substr(1, xpos - 1)) || 65536;
    const height = parseInt(x.substr(xpos + 1)) || 65536;
    return {width, height};
  }
  if (x.indexOf('x') === 0 && x.indexOf('/') > 0) {
    // multiplier 'x{d}/{n}'
    const spos = x.indexOf('/');
    const d = parseInt(x.substr(1, spos - 1)) || 1;
    const n = parseInt(x.substr(spos + 1)) || 1;
    const floatToSize = (n) => {
      var x = Math.floor(n);
      return (x % 2 === 0) ? x : (x - 1);
    };
    return {
      width: floatToSize(baseResolution.width * d / n),
      height: floatToSize(baseResolution.height * d / n)
    };
  }

  return {width: 65536, height: 65536};
};

function createMixStream(roomId, viewSettings, mediaOut, avCapability) {
  var mixedStreamId = roomId + '-' + viewSettings.label;
  var defaultAudiofmt = null;
  var defaultVideofmt = null;

  var hasAudioEncodeCapability = function (fmt) {
    return avCapability.audio.findIndex(
        f => isAudioFmtEqual(defaultAudiofmt, f)) >= 0;
  };
  var hasVideoEncodeCapability = function (fmt) {
    return avCapability.video.encode.findIndex(
        f => isVideoFmtEqual(fmt, f)) >= 0;
  };

  if (viewSettings.audio) {
    defaultAudiofmt = viewSettings.audio.format;
    if (!hasAudioEncodeCapability(defaultAudiofmt)) {
      defaultAudiofmt = null;
      for (const fmt of mediaOut.audio) {
        if (hasAudioEncodeCapability(fmt)) {
          defaultAudiofmt = fmt;
          break;
        }
      }
    }
  }
  if (!defaultAudiofmt) {
    log.error('No capable audio format for view: ' + viewSettings.label);
    return null;
  }

  if (viewSettings.video) {
    defaultVideofmt = viewSettings.video.format;
    if (!hasVideoEncodeCapability(defaultVideofmt)) {
      defaultVideofmt = null;
      for (const fmt of mediaOut.video.format) {
        if (hasVideoEncodeCapability(fmt)) {
          defaultVideofmt = fmt;
          break;
        }
      }
    }
    if (!defaultVideofmt) {
      log.error('No capable video format for view: ' + viewSettings.label);
      return null;
    }
  }

  var mixedStreamInfo = {
    id: mixedStreamId,
    type: 'mixed',
    media: {
      audio: (viewSettings.audio) ? {
        format: defaultAudiofmt,
        optional: {
          format: mediaOut.audio.filter(fmt => (
              hasAudioEncodeCapability(fmt)
              && !isAudioFmtEqual(fmt, defaultAudiofmt)))
        }
      } : undefined,
      video: (viewSettings.video) ? {
        format: defaultVideofmt,
        optional: {
          format: mediaOut.video.format.filter(fmt => (
              hasVideoEncodeCapability(fmt)
              && !isVideoFmtEqual(fmt, defaultVideofmt))),
          parameters: {
            resolution: mediaOut.video.parameters.resolution
                .map(x => calcResolution(x, viewSettings.video.parameters.resolution))
                .filter(reso => (reso.width < viewSettings.video.parameters.resolution.width
                    && reso.height < viewSettings.video.parameters.resolution.height)),
            framerate: mediaOut.video.parameters.framerate
                .filter(x => (x < viewSettings.video.parameters.framerate)),
            bitrate: mediaOut.video.parameters.bitrate,
            keyFrameInterval: mediaOut.video.parameters.keyFrameInterval
                .filter(x => (x < viewSettings.video.parameters.keyFrameInterval))
          }
        },
        parameters: {
          resolution: viewSettings.video.parameters.resolution,
          framerate: viewSettings.video.parameters.framerate,
          bitrate: viewSettings.video.parameters.bitrate,
          keyFrameInterval: viewSettings.video.parameters.keyFrameInterval,
        }
      } : undefined
    },
    info: {
      label: viewSettings.label,
      activeInput: 'unknown',
      layout: []
    }
  };

  return mixedStreamInfo;
}

function createForwardStream(id, media, info, roomConfig) {
  var result = {};

  const extractAudioFormat = (audioInfo) => {
    var result = {codec: audioInfo.codec};
    audioInfo.sampleRate && (result.sampleRate = audioInfo.sampleRate);
    audioInfo.channelNum && (result.channelNum = audioInfo.channelNum);
    return result;
  };

  const extractVideoFormat = (videoInfo) => {
    var result = {codec: videoInfo.codec};
    videoInfo.profile && (result.profile = videoInfo.profile);
    return result;
  };

  if (media.audio) {
    result.audio = {
      status: 'active',
      format: extractAudioFormat(media.audio)
    };

    media.audio.source && (result.audio.source = media.audio.source);

    if (roomConfig.transcoding.audio) {
      result.audio.optional = {format: []};
      roomConfig.mediaOut.audio.forEach((fmt) => {
        if ((fmt.codec !== result.audio.format.codec)
            || (fmt.sampleRate !== result.audio.format.sampleRate)
            || (fmt.channelNum !== result.audio.format.channelNum)) {
          result.audio.optional.format.push(fmt);
        }
      });
    }
  }

  if (media.video) {
    result.video = {
      status: 'active',
      format: extractVideoFormat(media.video)
    };

    //FIXME: sometimes, streaming send invalid resolution { width: 0, height: 0}
    if (media.video.resolution && (!media.video.resolution.height && !media.video.resolution.width)) {
      delete media.video.resolution;
    }

    media.video.source && (result.video.source = media.video.source);
    media.video.resolution && (media.video.resolution.width !== 0) && (media.video.resolution.height !== 0) && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.resolution = media.video.resolution);
    media.video.framerate && (media.video.framerate !== 0) && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.framerate = Math.floor(media.video.framerate));
    media.video.bitrate && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.bitrate = media.video.bitrate);
    media.video.keyFrameInterval && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.keyFrameInterval = media.video.keyFrameInterval);

    if (!media.video.simulcast) {
      if (roomConfig.transcoding.video && roomConfig.transcoding.video.format) {
        result.video.optional = {format: []};
        roomConfig.mediaOut.video.format.forEach((fmt) => {
          if ((fmt.codec !== result.video.format.codec)
              || (fmt.profile !== result.video.format.profile)) {
            result.video.optional.format.push(fmt);
          }
        });
      }

      if (roomConfig.transcoding.video && roomConfig.transcoding.video.parameters) {
        if (roomConfig.transcoding.video.parameters.resolution) {
          result.video.optional = (result.video.optional || {});
          result.video.optional.parameters = (result.video.optional.parameters || {});

          if (result.video.parameters && result.video.parameters.resolution) {
            result.video.optional.parameters.resolution = roomConfig.mediaOut.video.parameters.resolution.map((x) => {return calcResolution(x, result.video.parameters.resolution)}).filter((reso, pos, self) => {return ((reso.width < result.video.parameters.resolution.width) && (reso.height < result.video.parameters.resolution.height)) && (self.findIndex((r) => {return r.width === reso.width && r.height === reso.height;}) === pos);});
          } else {
            result.video.optional.parameters.resolution = roomConfig.mediaOut.video.parameters.resolution
              .filter((x) => {return (x !== 'x3/4') && (x !== 'x2/3') && (x !== 'x1/2') && (x !== 'x1/3') && (x !== 'x1/4');})//FIXME: is auto-scaling possible?
              .map((x) => {return calcResolution(x)});
          }
        }

        if (roomConfig.transcoding.video.parameters.framerate) {
          result.video.optional = (result.video.optional || {});
          result.video.optional.parameters = (result.video.optional.parameters || {});

          if (result.video.parameters && result.video.parameters.framerate) {
            result.video.optional.parameters.framerate = roomConfig.mediaOut.video.parameters.framerate.filter((fr) => {return fr < result.video.parameters.framerate;});
          } else {
            result.video.optional.parameters.framerate = roomConfig.mediaOut.video.parameters.framerate;
          }
        }

        if (roomConfig.transcoding.video.parameters.bitrate) {
          result.video.optional = (result.video.optional || {});
          result.video.optional.parameters = (result.video.optional.parameters || {});

          result.video.optional.parameters.bitrate = roomConfig.mediaOut.video.parameters.bitrate.filter((x) => {return Number(x.substring(1)) < 1;});
        }

        if (roomConfig.transcoding.video.parameters.keyFrameInterval) {
          result.video.optional = (result.video.optional || {});
          result.video.optional.parameters = (result.video.optional.parameters || {});

          result.video.optional.parameters.keyFrameInterval = roomConfig.mediaOut.video.parameters.keyFrameInterval;
        }
      }
    } else {
      result.video.optional = {
        format: [],
        parameters: {
          resolution: [],
          framerate: [],
          bitrate: [],
          keyFrameInterval: [],
        }
      };
    }
  }

  var forwardStreamInfo = {
    id,
    type: 'forward',
    media: result,
    info
  };
  return forwardStreamInfo;
}

function updateForwardStream(stream, info, roomConfig) {
  if (stream && stream.type === 'forward' && info) {
    if (info.video && stream.media.video) {
      if (info.video.parameters) {
        if (info.video.parameters.resolution) {
          stream.media.video.parameters = (stream.media.video.parameters || {});
          if (!stream.media.video.parameters.resolution ||
              (stream.media.video.parameters.resolution.width !== info.video.parameters.resolution.width) ||
              (stream.media.video.parameters.resolution.height !== info.video.parameters.resolution.height)) {
            stream.media.video.parameters.resolution = info.video.parameters.resolution;
            if (roomConfig.transcoding.video &&
                roomConfig.transcoding.video.parameters &&
                roomConfig.transcoding.video.parameters.resolution &&
                stream.media.video.rid === undefined) {
              stream.media.video.optional = (stream.media.video.optional || {});
              stream.media.video.optional.parameters = (stream.media.video.optional.parameters || {});
              stream.media.video.optional.parameters.resolution =
                roomConfig.mediaOut.video.parameters.resolution
                  .map(x => calcResolution(x, stream.media.video.parameters.resolution))
                  .filter(reso => (reso.width < stream.media.video.parameters.resolution.width &&
                    reso.height < stream.media.video.parameters.resolution.height));
            }
            return true;
          }
        }
      }
    } else if (info.rid) {
      const simInfo = info.info;
      if (simInfo && simInfo.video && stream.media.video) {
        let alternative = stream.media.video.alternative || [];
        let pos = alternative.findIndex((item) => item.rid === (info.rid));
        if (pos < 0) {
          pos = alternative.length;
          alternative.push({rid: info.rid});
          alternative[pos].parameters = stream.media.video.parameters;
        }
        if (simInfo.video.parameters) {
          alternative[pos].parameters = Object.assign({},
            alternative[pos].parameters,
            simInfo.video.parameters);
        }
        stream.media.video.alternative = alternative;
      }
      return true;
    } else if (info.firstrid) {
      if (stream.media.video) {
        stream.media.video.rid = info.firstrid;
        stream.media.video.optional.parameters.resolution = [];
      }
      return true;
    }
  }
  return false;
}

function toPortalFormat (internalStream) {
  var stream = JSON.parse(JSON.stringify(internalStream));
  if (stream && stream.media && stream.media.video) {
    const videoInfo = stream.media.video;
    if (!videoInfo.original) {
      videoInfo.original = [{}];
      if (videoInfo.format) {
        videoInfo.original[0].format = videoInfo.format;
        delete videoInfo.format;
      }
      if (videoInfo.parameters) {
        videoInfo.original[0].parameters = videoInfo.parameters;
        delete videoInfo.parameters;
      }
      if (videoInfo.rid) {
        videoInfo.original[0].simulcastRid = videoInfo.rid;
        delete videoInfo.rid;
      }
    }
    if (videoInfo.alternative) {
      videoInfo.alternative.forEach((alt) => {
        if (!alt.format) {
          alt.format = videoInfo.original[0].format;
        }
        alt.simulcastRid = alt.rid;
        delete alt.rid;
        videoInfo.original.push(alt);
      });
      delete videoInfo.alternative;
    }
  }
  return stream;
};

module.exports = {
  createMixStream,
  createForwardStream,
  updateForwardStream,
  toPortalFormat
};
