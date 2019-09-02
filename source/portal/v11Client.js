// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var log = require('./logger').logger.getLogger('V11Client');
var requestData = require('./requestDataValidator');

var idPattern = /^[0-9a-zA-Z\-]+$/;
function isValidIdString(str) {
  return (typeof str === 'string') && idPattern.test(str);
}

function safeCall () {
  var callback = arguments[0];
  if (typeof callback === 'function') {
    var args = Array.prototype.slice.call(arguments, 1);
    callback.apply(null, args);
  }
}

const getErrorMessage = function (err) {
  if (typeof err === 'string') {
    return err;
  } else if (err && err.message) {
    return err.message;
  } else {
    log.debug('Unknown error:', err);
    return 'Unknown';
  }
};

var V11Client = function(clientId, sigConnection, portal) {
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
            || SOAC.signaling.type === 'removed-candidates') {
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

      var stream_id = Math.round(Math.random() * 1000000000000000000) + '';
      return validatePubReq(pubReq)
        .then((req) => {
          req.type = 'webrtc';//FIXME: For backend compatibility with v3.4 clients.
          return portal.publish(clientId, stream_id, req);
        }).then((result) => {
          safeCall(callback, 'ok', {id: stream_id});
        }).catch(onError('publish', callback));
    });

    socket.on('unpublish', function(unpubReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      return validateId('stream id', unpubReq.id)
        .then((streamId) => {
          return portal.unpublish(clientId, streamId);
        }).then((result) => {
          safeCall(callback, 'ok');
        }).catch(onError('unpublish', callback));
    });

    socket.on('stream-control', function(streamCtrlReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      return validateStreamCtrlReq(streamCtrlReq)
        .then((req) => {
          return portal.streamControl(clientId, req.id, {operation: req.operation, data: req.data});
        }).then((result) => {
          safeCall(callback, 'ok', result);
        }).catch(onError('stream-control', callback));
    });

    socket.on('subscribe', function(subReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      var subscription_id = Math.round(Math.random() * 1000000000000000000) + '';
      return validateSubReq(subReq)
        .then((req) => {
          req.type = 'webrtc';//FIXME: For backend compatibility with v3.4 clients.
          return portal.subscribe(clientId, subscription_id, req);
        }).then((result) => {
          safeCall(callback, 'ok', {id: subscription_id});
        }).catch(onError('subscribe', callback));
    });

    socket.on('unsubscribe', function(unsubReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      return validateId('subscription id', unsubReq.id)
        .then((subscriptionId) => {
          return portal.unsubscribe(clientId, subscriptionId);
        }).then((result) => {
          safeCall(callback, 'ok');
        }).catch(onError('unsubscribe', callback));
    });

    socket.on('subscription-control', function(subCtrlReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      return validateSubscriptionCtrlReq(subCtrlReq)
        .then((req) => {
          return portal.subscriptionControl(clientId, req.id, {operation: req.operation, data: req.data});
        }).then((result) => {
          safeCall(callback, 'ok');
        }).catch(onError('subscription-control', callback));
    });

    socket.on('soac', function(SOAC, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      return validateSOAC(SOAC)
        .then((soac) => {
          return portal.onSessionSignaling(clientId, soac.id, soac.signaling);
        }).then((result) => {
          safeCall(callback, 'ok');
        }).catch(onError('soac', callback));
    });
  };

  that.notify = (evt, data) => {
    sendMsg(evt, data);
  };

  that.join = (token) => {
    return portal.join(clientId, token)
      .then(function(result){
        that.inRoom = result.data.room.id;
        that.tokenCode = result.tokenCode;
        result.data.id = that.id;
        return result.data;
      })
  };

  that.leave = () => {
    if(that.inRoom) {
      return portal.leave(that.id).catch(() => {
        that.inRoom = undefined;
        that.tokenCode = undefined;
      });
    }
  };

  that.resetConnection = (sigConnection) => {
    that.connection.close(false);
    that.connection = sigConnection;
    listenAt(that.connection.socket);
    return Promise.resolve('ok');
  };

  that.drop = () => {
    that.connection.sendMessage('drop');
    that.leave();
    that.connection.close(true);
  };

  listenAt(that.connection.socket);
  return that;
};


module.exports = V11Client;

