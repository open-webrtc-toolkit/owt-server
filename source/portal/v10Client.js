// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var log = require('./logger').logger.getLogger('V10Client');
var V11Client = require('./v11Client');

// Convert to 1.0 format
var convertStream = function (stream) {
  if (stream && stream.media && stream.media.video) {
    const videoInfo = stream.media.video;
    if (videoInfo.original && videoInfo.original[0]) {
      videoInfo.format = videoInfo.original[0].format;
      videoInfo.parameters = videoInfo.original[0].parameters;
      delete videoInfo.original;
    }
  }
};

var V10Client = function(clientId, sigConnection, portal) {
  var client = V11Client(clientId, sigConnection, portal);
  var latestJoin = client.join;
  var latestNotify = client.notify;

  client.join = (token) => {
    return latestJoin(token).then((data) => {
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
    latestNotify(evt, data);
  };

  return client;
};

module.exports = V10Client;
