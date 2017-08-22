/* global require */
'use strict';

var log = require('./logger').logger.getLogger('V10Client');

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

var V10Client = function(clientId, sigConnection, portal) {
  var that = {
    id: clientId,
    connection: sigConnection
  };

  const sendMsg = (evt, data) => {
    that.connection.sendMessage(evt, data);
  };

  const listenAt = (socket) => {
    socket.on('text', function(textReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if (textReq.to === '' || typeof textReq.to !== 'string') {
        return safeCall(callback, 'error', 'Invalid receiver');
      }

      return portal.text(clientId, textReq.to, textReq.message)
        .then((result) => {
          safeCall(callback, 'ok');
        }).catch((err) => {
          const err_message = getErrorMessage(err);
          log.info('portal.text failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('publish', function(pubReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }
      //FIXME: Add requestData validation here.

      var stream_id = Math.round(Math.random() * 1000000000000000000) + '';
      return portal.publish(clientId, stream_id, pubReq)
        .then((result) => {
          safeCall(callback, 'ok', {id: stream_id});
        }).catch((err) => {
          const err_message = getErrorMessage(err);
          log.info('portal.publish failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('unpublish', function(unpubReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }
      //FIXME: Add requestData validation here.

      return portal.unpublish(clientId, unpubReq.id)
        .then((result) => {
          safeCall(callback, 'ok');
        }).catch((err) => {
          const err_message = getErrorMessage(err);
          log.info('portal.unpublish failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('stream-control', function(streamCtlReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }
      //FIXME: Add requestData validation here.

      return portal.streamControl(clientId, streamCtlReq.id, {operation: streamCtlReq.operation, data: streamCtlReq.data})
        .then((result) => {
          safeCall(callback, 'ok', result);
        }).catch((err) => {
          const err_message = getErrorMessage(err);
          log.info('portal.streamControl failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('subscribe', function(subReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }
      //FIXME: Add requestData validation here.
      if (!subReq.media || !(subReq.media.audio || subReq.media.video)) {
        return safeCall(callback, 'error', 'Bad subscription request');
      }

      var subscription_id = Math.round(Math.random() * 1000000000000000000) + '';
      return portal.subscribe(clientId, subscription_id, subReq)
        .then((result) => {
          safeCall(callback, 'ok', {id: subscription_id});
        }).catch((err) => {
          const err_message = getErrorMessage(err);
          log.info('portal.subscribe failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('unsubscribe', function(unsubReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }
      //FIXME: Add requestData validation here.

      return portal.unsubscribe(clientId, unsubReq.id)
        .then((result) => {
          safeCall(callback, 'ok');
        }).catch((err) => {
          const err_message = getErrorMessage(err);
          log.info('portal.unsubscribe failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('subscription-control', function(subCtlReq, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }
      //FIXME: Add requestData validation here.

    });

    socket.on('soac', function(SOAC, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }
      //FIXME: Add requestData validation here.

      return portal.onSessionSignaling(clientId, SOAC.id, SOAC.signaling)
        .then((result) => {
          safeCall(callback, 'ok');
        }).catch((err) => {
          const err_message = getErrorMessage(err);
          log.info('portal.onSessionSignaling failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('set-permission', function(Permission, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }
      //FIXME: Add requestData validation here.

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
        return result.data;
      });
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


module.exports = V10Client;

