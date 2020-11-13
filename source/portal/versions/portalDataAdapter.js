// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Portal's request/response data adapter for different versions
 */

'use strict';

const log = require('../logger').logger.getLogger('PortalDataAdapter');
const ReqType = require('./requestType');

// Adatper from v1.0 to v1.1
const AdatperV1_0 = {
  upgradeRequest: function (type, data) {
    return data;
  },
  downgradeResponse: function (type, data) {
    if (type === ReqType.Join) {
      if (data && data.room && data.room.streams) {
        data.room.streams.forEach((stream) => {
          AdatperV1_0._convertStream(stream);
        });
      }
    }
    return data;
  },
  downgradeNotification: function (evt, data) {
    if (evt === 'stream') {
      if (data.status === 'add') {
        AdatperV1_0._convertStream(data.data);
      }
      if (data.status === 'update' && data.data.field === '.') {
        AdatperV1_0._convertStream(data.data.value);
      }
      log.debug('converted stream data:', JSON.stringify(data));
    }
    return {evt, data};
  },
  _convertStream: function (stream) {
    if (stream && stream.media && stream.media.video) {
      const videoInfo = stream.media.video;
      if (videoInfo.original && videoInfo.original[0]) {
        videoInfo.format = videoInfo.original[0].format;
        videoInfo.parameters = videoInfo.original[0].parameters;
        delete videoInfo.original;
      }
    }
  },
};

// Adapter from v1.1 to v1.2
const AdatperV1_1 = {
  upgradeRequest: function (type, data) {
    if (type === ReqType.Pub) {
      const pubReq = {
        media: { tracks: [] },
        attributes: data.attributes,
        transportId: null,
        legacy: true
      };
      if (data.media.audio) {
        pubReq.media.tracks.push({
          type: 'audio',
          source: data.media.audio.source,
          mid: pubReq.media.tracks.length.toString(),
        });
      }
      if (data.media.video) {
        pubReq.media.tracks.push({
          type: 'video',
          source: data.media.video.source,
          mid: pubReq.media.tracks.length.toString(),
        });
      }
      return pubReq;

    } else if (type === ReqType.Sub) {
      const subReq = {
        media: { tracks: [] },
        transportId: null,
        legacy: true
      };
      if (data.media.audio) {
        subReq.media.tracks.push({
          type: 'audio',
          mid: subReq.media.tracks.length.toString(),
          from: data.media.audio.from
        });
      }
      if (data.media.video) {
        subReq.media.tracks.push({
          type: 'video',
          mid: subReq.media.tracks.length.toString(),
          from: data.media.video.from,
          parameters: data.media.video.parameters,
        });
      }
      return subReq;
    }
    return data;
  },
  downgradeResponse: function (type, data) {
    if (type === ReqType.Join) {
      if (data && data.room && data.room.streams) {
        data.room.streams.forEach((stream) => {
          AdatperV1_1._convertStream(stream);
        });
      }
    }
    return data;
  },
  downgradeNotification: function (evt, data) {
    if (evt === 'stream') {
      if (data.status === 'add') {
        AdatperV1_1._convertStream(data.data);
      }
      if (data.status === 'update' && data.data.field === '.') {
        AdatperV1_1._convertStream(data.data.value);
      }
      log.debug('converted stream data:', JSON.stringify(data));
    }
    if (evt === 'progress') {
      if (data.sessionId && data.status === 'ready') {
        return {};
      }
    }
    return {evt, data};
  },
  _convertStream: function (stream) {
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
          const simulcastGroup = stream.media.tracks.filter(t => t.mid === videoInfo.mid);
          stream.media.video.original = simulcastGroup.map(t => ({
            format: t.format,
            parameters: t.parameters,
            simulcastRid: t.rid,
          }));
        }
      }
      delete stream.media.tracks;
    }
  },
};

module.exports = function (version) {
  const requestData = require('./requestDataValidator')(version);
  return {
    translateReq: function (type, req) {
      return requestData.validate(type, req)
        .then(data => {
          if (version === '1.0') {
            data = AdatperV1_0.upgradeRequest(type, data);
            data = AdatperV1_1.upgradeRequest(type, data);
          } else if (version === '1.1') {
            data = AdatperV1_1.upgradeRequest(type, data);
          }
          return data;
        });
    },
    translateResp: function (type, resp) {
      if (version === '1.0') {
        resp = AdatperV1_1.downgradeResponse(type, resp);
        resp = AdatperV1_0.downgradeResponse(type, resp);
      } else if (version === '1.1') {
        resp = AdatperV1_1.downgradeResponse(type, resp);
      }
      return resp;
    },
    translateNotification: function (evt, data) {
      if (version === '1.0') {
        ({ evt, data } = AdatperV1_1.downgradeNotification(evt, data));
        ({ evt, data } = AdatperV1_0.downgradeNotification(evt, data));
      } else if (version === '1.1') {
        ({ evt, data } = AdatperV1_1.downgradeNotification(evt, data));
      }
      return { evt, data };
    }
  }
};
