// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var path = require('path');
var log = require('./logger').logger.getLogger('SocketIOServer');
var crypto = require('crypto');
var vsprintf = require("sprintf-js").vsprintf;

var LegacyClient = require('./legacyClient');
var Client = require('./client');

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

var Connection = function(spec, socket, reconnectionKey, portal, dock) {
  var that = {
    socket: socket,
  };

  // With value of (undefined | 'initialized' | 'connecting' | 'connected' | 'waiting_for_reconnecting')
  var state;
  var client_id;
  var protocol_version;
  var waiting_for_reconnecting_timer = null;
  var pending_messages = [];

  let reconnection = {
    enabled: false
  };

  const validateUserAgent = function(ua){
    if(!ua||!ua.sdk||!ua.sdk.version||!ua.sdk.type||!ua.runtime||!ua.runtime.version||!ua.runtime.name||!ua.os||!ua.os.version||!ua.os.name){
      return Promise.reject('User agent info is incorrect');
    }
    return Promise.resolve(ua.sdk.type === 'Objective-C' || ua.sdk.type === 'C++' || ua.sdk.type === 'Android' || ua.sdk.type == 'JavaScript')
  };

  const calculateSignature = function(ticket) {
    const to_sign = vsprintf('%s,%s,%s', [
      ticket.participantId,
      ticket.notBefore,
      ticket.notAfter
    ]);
    const signed = crypto.createHmac('sha256', reconnectionKey)
                     .update(to_sign)
                     .digest('hex');
    return (new Buffer(signed)).toString('base64');
  };

  const generateReconnectionTicket = function() {
    const now = Date.now();
    reconnection.ticket = {
      participantId: client_id,
      ticketId: Math.random().toString(36).substring(2),
      notBefore: now,
      // Unit for reconnectionTicketLifetime is second.
      notAfter: now + spec.reconnectionTicketLifetime * 1000
    };
    reconnection.ticket.signature = calculateSignature(reconnection.ticket);
    return (new Buffer(JSON.stringify(reconnection.ticket))).toString('base64');
  };

  const validateReconnectionTicket = function(ticket) {
    if(!reconnection.enabled) {
      return Promise.reject('Reconnection is not allowed.');
    }
    if(ticket.participantId!==client_id){
      return Promise.reject('Participant ID is not matched.');
    }
    let signature = calculateSignature(ticket);
    if(signature!=ticket.signature){
      return Promise.reject('Invalid reconnection ticket signature');
    }
    const now = Date.now();
    if(now<ticket.notBefore||now>ticket.notAfter){
      return Promise.reject('Ticket is expired.');
    }
    if(disconnect_timeout){
      clearTimeout(disconnect_timeout);
      disconnect_timeout=undefined;
    }
    disconnected = true;
    socket.disconnect(true);
    return Promise.resolve();
  };

  const drainPendingMessages = () => {
    for(let message of pending_messages){
      if (message && message.event) {
        that.sendMessage(message.event, message.data);
      }
    }
    pending_messages=[];
  };

  const forceClientLeave = () => {
    log.debug('forceClientLeave, client_id:', client_id);
    if (client_id) {
      const client = dock.getClient(client_id)
      if (client && client.connection === that) {
        client.leave();
        dock.onClientLeft(client_id);
        state = 'initialized';
        client_id = undefined;
      }
    }
  };

  const okWord = () => {
    return protocol_version === 'legacy' ? 'success' : 'ok';
  };

  const run = () => {
    state = 'initialized';
    socket.on('login', function(login_info, callback) {
      if (state !== 'initialized') {
        return safeCall(callback, 'error', 'Connection is in service');
      }
      state = 'connecting';

      client_id = socket.id + '';
      var client;
      if (login_info.protocol === undefined) {
        protocol_version = 'legacy';
        client = new LegacyClient(client_id, that, portal);
      } else if (login_info.protocol === '1.0' ||
          login_info.protocol === '1.1' ||
          login_info.protocol === '1.2') {
        //FIXME: Reject connection from 3.5 client
        if (login_info.userAgent && login_info.userAgent.sdk &&
            login_info.userAgent.sdk.version === '3.5') {
          safeCall(callback, 'error', 'Deprecated client version');
          return socket.disconnect();
        }
        protocol_version = login_info.protocol;
        client = new Client(client_id, that, portal, protocol_version);
      } else {
        safeCall(callback, 'error', 'Unknown client protocol');
        return socket.disconnect();
      }

      return validateUserAgent(login_info.userAgent)
        .then((reconnEnabled) => {
          reconnection.enabled = reconnEnabled;
          return new Promise(function(resolve){
            resolve(JSON.parse((new Buffer(login_info.token, 'base64')).toString()));
          });
        }).then((token) => {
          return client.join(token);
        }).then((result) => {
          if (state === 'connecting') {
            if(reconnection.enabled){
              result.reconnectionTicket = generateReconnectionTicket();
            }
            state = 'connected';
            dock.onClientJoined(client_id, client);
            safeCall(callback, okWord(), result);
          } else {
            client.leave(client_id);
            state = 'initialized';
            safeCall(callback, 'error', 'Participant early left');
            log.info('Login failed:', 'Participant early left');
            socket.disconnect();
          }
        }).catch((err) => {
          state = 'initialized';
          const err_message = getErrorMessage(err);
          safeCall(callback, 'error', err_message);
          log.info('Login failed:', err_message);
          socket.disconnect();
        });
    });

    socket.on('relogin', function(reconnectionTicket, callback) {
      if (state !== 'initialized') {
        return safeCall(callback, 'error', 'Connection is in service');
      }
      state = 'connecting';

      var client;
      var reconnection_ticket;
      new Promise((resolve) => {
        resolve(JSON.parse((new Buffer(reconnectionTicket, 'base64')).toString()));
      }).then((ticket) => {
        var now = Date.now();
        if (ticket.notBefore > now || ticket.notAfter < now) {
          return Promise.reject('Ticket is expired');
        } else if (ticket.signature !== calculateSignature(ticket)) {
          return Promise.reject('Invalid reconnection ticket signature');
        } else {
          reconnection_ticket = ticket;
          return dock.getClient(ticket.participantId);
        }
      }).then((clt) => {
        client = clt;
        if (!client) {
          return Promise.reject('Client does NOT exist');
        }
        return client.connection.reconnect();
      }).then((connectionInfo) => {
        if (!connectionInfo.reconnection.enabled) {
          return Promise.reject('Reconnection is not allowed');
        } else if (connectionInfo.reconnection.ticket.participantId !== reconnection_ticket.participantId) {
          return Promise.reject('Participant ID is not matched');
        } else {
          client_id = connectionInfo.clientId + '';
          protocol_version = connectionInfo.protocolVersion + '';
          pending_messages = connectionInfo.pendingMessages;
          reconnection.enabled = true;
          return client.resetConnection(that);
        }
      }).then(() => {
        let ticket = generateReconnectionTicket();
        state = 'connected';
        safeCall(callback, okWord(), ticket);
        drainPendingMessages();
      }).catch((err) => {
        state = 'initialized';
        const err_message = getErrorMessage(err);
        log.info('Relogin failed:', err_message);
        safeCall(callback, 'error', err_message);
        forceClientLeave();
        socket.disconnect();
      });
    });

    socket.on('refreshReconnectionTicket', function(callback){
      if(state !== 'connected') {
        return safeCall(callback, 'error', 'Illegal request');
      }

      if(!reconnection.enabled) {
        return safeCall(callback,'error','Reconnection is not enabled.');
      }

      let ticket = generateReconnectionTicket();
      safeCall(callback, okWord(), ticket);
    });

    socket.on('logout', function(callback){
      reconnection.enabled = false;
      state = 'initialized';
      if (client_id) {
        forceClientLeave();
        safeCall(callback, okWord());
      } else {
        return safeCall(callback, 'error', 'Illegal request');
      }
    });

    socket.on('disconnect', function(reason) {
      log.debug('socket.io disconnected, reason: '+reason);

      if (state === 'connected' && reconnection.enabled) {
        state = 'waiting_for_reconnecting';
        socket.disconnect(true);
        waiting_for_reconnecting_timer = setTimeout(() => {
          log.info(client_id + ' waiting for reconnecting timeout.');
          forceClientLeave();
        }, spec.reconnectionTimeout * 1000);
      } else {
        if (state === 'connecting' || state === 'connected') {
          forceClientLeave();
        }
      }
    });
  };

  that.isInService = () => {
    return state && (state !== 'initialized');
  };

  that.reconnect = () => {
    log.debug('client reconnect', client_id);
    if (waiting_for_reconnecting_timer) {
      clearTimeout(waiting_for_reconnecting_timer);
      waiting_for_reconnecting_timer = null;
    }

    return {
      pendingMessages: pending_messages,
      clientId: client_id,
      protocolVersion: protocol_version,
      reconnection: reconnection
    };
  };

  that.sendMessage = function (event, data) {
    log.debug('sendMessage, event:', event, 'data:', data);
    if (state === 'connected') {
      try {
        socket.emit(event, data);
      } catch (e) {
        log.error('socket.emit error:', e.message);
      }
    } else {
      pending_messages.push({event: event, data: data});
    }
  };

  that.close = (ifLeave) => {
    log.debug('close it, client_id:', client_id);
    ifLeave && forceClientLeave();

    waiting_for_reconnecting_timer && clearTimeout(waiting_for_reconnecting_timer);
    waiting_for_reconnecting_timer = null;

    try {
      socket.disconnect();
    } catch (e) {
      log.error('socket.emit error:', e.message);
    }

    reconnection.enabled = false;
  };

  run();
  return that;
};


var SocketIOServer = function(spec, portal, observer) {
  var that = {};
  var io;
  var clients = {};
  // A Socket.IO server has a unique reconnection key. Client cannot reconnect to another Socket.IO server in the cluster.
  var reconnection_key = require('crypto').randomBytes(64).toString('hex');
  var sioOptions = {};
  if (spec.pingInterval) {
    sioOptions.pingInterval = spec.pingInterval * 1000;
  }
  if (spec.pingTimeout) {
    sioOptions.pingTimeout = spec.pingTimeout * 1000;
  }

  var startInsecure = function(port, cors) {
    var server = require('http').createServer().listen(port);
    io = require('socket.io').listen(server, sioOptions);
    io.origins((origin, callback) => {
      if (cors.indexOf(origin) < 0 && cors.indexOf('*') < 0) {
        return callback('origin not allowed', false);
      }

      callback(null, true);
    });
    run();
    return Promise.resolve('ok');
  };

  var startSecured = function(port, cors, keystorePath, forceTlsv12) {
    return new Promise(function(resolve, reject) {
      var cipher = require('./cipher');
      var keystore = path.resolve(path.dirname(keystorePath), cipher.kstore);
      cipher.unlock(cipher.k, keystore, function(err, passphrase) {
        if (!err) {
          var option = {pfx: require('fs').readFileSync(keystorePath), passphrase: passphrase};
          if (forceTlsv12) {
            var constants = require('constants');
            option.secureOptions = (constants.SSL_OP_NO_TLSv1 | constants.SSL_OP_NO_TLSv1_1);
          }
          var server = require('https').createServer(option).listen(port);
          io = require('socket.io').listen(server, sioOptions);
          io.origins((origin, callback) => {
            if (cors.indexOf(origin) < 0 && cors.indexOf('*') < 0) {
              return callback('origin not allowed', false);
            }
            callback(null, true);
          });
          run();
          resolve('ok');
        } else {
          reject(err);
        }
      });
    });
  };

  var run = function() {
    io.sockets.on('connection', function(socket) {
      var conn = Connection(spec, socket, reconnection_key, portal, that);

      setTimeout(() => {
        if (!conn.isInService()) {
          conn.close();
        }
      }, 3 * 60 * 1000);
    });
  };

  that.onClientJoined = (id, client) => {
    log.debug('onClientJoined, id:', id, 'client.tokenCode:', client.tokenCode);
    clients[id] = client;
    observer.onJoin(client.tokenCode);
  };

  that.onClientLeft = (id) => {
    log.debug('onClientLeft, id:', id);
    if (clients[id]) {
      observer.onLeave(clients[id].tokenCode);
      delete clients[id];
    }
  };

  that.getClient = (id) => {
    if (clients[id]) {
      return clients[id];
    } else {
      return null;
    }
  };

  that.start = function() {
    if (!spec.ssl) {
      return startInsecure(spec.port, spec.cors);
    } else {
      return startSecured(spec.port, spec.cors, spec.keystorePath, spec.forceTlsv12);
    }
  };

  that.stop = function() {
    for(var pid in clients) {
      clients[pid].drop();
    }
    clients = {};
    io && io.close();
    io = undefined;
  };

  that.notify = function(participantId, event, data) {
    log.debug('notify participant:', participantId, 'event:', event, 'data:', data);
    if (clients[participantId]) {
      clients[participantId].notify(event, data);
      return Promise.resolve('ok');
    } else {
      return Promise.reject('participant does not exist');
    }
  };

  that.drop = function(participantId) {
    if (participantId === 'all') {
      for(var pid in clients) {
        clients[pid].drop();
      }
    } else if (clients[participantId]) {
      clients[participantId].drop();
    } else {
      log.debug('user not in room', participantId);
    }
  };

  return that;
};

module.exports = SocketIOServer;

