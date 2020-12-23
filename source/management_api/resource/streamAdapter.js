// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
/*
 * Adapter for old version stream data
 */
'use strict';

// Convert from latest conference stream format to v1.1
function convertToV11Stream(stream) {
  if (stream && stream.media && stream.media.tracks) {
    const audioInfo = stream.media.tracks.find(t => t.type === 'audio');
    if (audioInfo) {
      stream.media.audio = {
        status: audioInfo.status,
        source: audioInfo.source,
        format: audioInfo.format,
        optional: audioInfo.optional,
      };
    }
    const videoInfo = stream.media.tracks.find(t => t.type === 'video');
    if (videoInfo) {
      stream.media.video = {
        status: videoInfo.status,
        source: videoInfo.source,
        original: [{
          format: videoInfo.format,
          parameters: videoInfo.parameters,
        }],
        optional: videoInfo.optional,
      };
      if (videoInfo.rid) {
        // Add simulcast data
        const simulcastGroup = stream.media.tracks.filter(t => t.mid ===
          videoInfo.mid);
        stream.media.video.original = simulcastGroup.map(t => ({
          format: t.format,
          parameters: t.parameters,
          simulcastRid: t.rid,
        }));
      }
    }
    delete stream.media.tracks;
  }
  return stream;
}

// Convert from latest conference stream format to v1.0
function convertToV10Stream(stream) {
  stream = convertToV11Stream(stream);
  if (stream && stream.media && stream.media.video &&
    stream.media.video.original && stream.media.video.original[0]) {
    stream.media.video.format = stream.media.video.original[0].format;
    stream.media.video.parameters = stream.media.video.original[0].parameters;
    delete stream.media.video.original;
  }
  return stream;
}

module.exports = {
  convertToV11Stream,
  convertToV10Stream,
};
