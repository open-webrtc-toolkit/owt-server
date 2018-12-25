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
<<<<<<< HEAD
  var client = V11Client(clientId, sigConnection, portal);
  var latestJoin = client.join;
  var latestNotify = client.notify;
=======
  var that = {
    id: clientId,
    connection: sigConnection
  };

  const sendMsg = (evt, data) => {
    that.connection.sendMessage(evt, data);
  };

  const onError = (method, callback) => {
    return (err) => {
      const err_message = getErrorMessage(err);
      log.error(method + ' failed:', err_message);
      safeCall(callback, 'error', err_message);
    };
  };

  const idPattern = /^[0-9a-zA-Z\-]+$/;
  const validateId = (type, id) => {
    if ((typeof id === 'string') && idPattern.test(id)) {
      return Promise.resolve(id);
    } else {
      return Promise.reject('Invalid ' + type);
    }
  };

  const validateTextReq = (textReq) => {
    if (textReq.to === '' || typeof textReq.to !== 'string') {
      return Promise.reject('Invalid receiver');
    } else {
      return Promise.resolve(textReq);
    }
  };

  const validatePubReq = (pubReq) => {
    return requestData.validate('publication-request', pubReq);
  };

  const validateStreamCtrlReq = (stCtrlReq) => {
    return validateId('stream id', stCtrlReq.id)
      .then(() => {
        return requestData.validate('stream-control-info', stCtrlReq);
      });
  };

  const validateSubReq = (subReq) => {
    if (!subReq.media || !(subReq.media.audio || subReq.media.video)) {
      return Promise.reject('Bad subscription request');
    }
    return requestData.validate('subscription-request', subReq);
  };

  const validateSubscriptionCtrlReq = (subCtrlReq) => {
    return validateId('subscription id', subCtrlReq.id)
      .then(() => {
        return requestData.validate('subscription-control-info', subCtrlReq);
      });
  };

  const validateSOAC = (SOAC) => {
    return validateId('session id', SOAC.id)
      .then(() => {
        if (SOAC.signaling.type === 'offer'
            || SOAC.signaling.type === 'answer'
            || SOAC.signaling.type === 'candidate'
            || SOAC.signaling.type === 'removed-candidates'
            || SOAC.signaling.type === 'ice-parameters') {
          return Promise.resolve(SOAC);
        } else {
          return Promise.reject('Invalid signaling type');
        }
      });
  };

  const listenAt = (socket) => {
    socket.on('text', function(textReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      return validateTextReq(textReq)
        .then((req) => {
          return portal.text(clientId, req.to, req.message);
        }).then((result) => {
          safeCall(callback, 'ok');
        }).catch(onError('text', callback));
    });

    socket.on('publish', function(pubReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }
>>>>>>> Add RTCIceTransport and RTCIceCandidate.

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
