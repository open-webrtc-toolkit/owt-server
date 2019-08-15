// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var log = require('./logger').logger.getLogger('V11Client');
var V10Client = require('./v10Client');

var convertStream = function (stream) {
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
};

var V11Client = function(clientId, sigConnection, portal) {
  var client = V10Client(clientId, sigConnection, portal);
  var legacyJoin = client.join;
  var legacyNotify = client.notify;

  client.join = (token) => {
    return legacyJoin(token).then((data) => {
      if (data && data.room && data.room.streams) {
        data.room.streams.forEach((stream) => {
          convertStream(stream);
        });
      }
      log.debug('converted join data:', JSON.stringify(data));
      return data;
    });
  };

  client.notify = (evt, data) => {
    if (evt === 'stream') {
      if (data.status === 'add') {
        convertStream(data.data);
      }
      if (data.status === 'update' && data.data.field === '.') {
        convertStream(data.data.value);
      }
      log.debug('converted stream data:', JSON.stringify(data));
    }
    legacyNotify(evt, data);
  };

  return client;
};

module.exports = V11Client;
