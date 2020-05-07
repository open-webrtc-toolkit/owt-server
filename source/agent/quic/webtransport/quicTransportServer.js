// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const EventEmitter = require('events');
const logger = require('../../logger').logger;
const log = logger.getLogger('QuicTransportServer');
const addon = require('../build/Release/quic');

/* Every QUIC agent is a QuicTransportServer which accepts QuicTransport
 * connections from clients.
 *
 * Events:
 * ended - Server is ended. All Connections will be stopped.
 * streamadded - A new stream is added, and ready to be attached to a stream
 * pipeline. Argument: a string for publication or subscription ID.
 */
module.exports = class QuicTransportServer extends EventEmitter {
  constructor(port) {
    super();
    this._server = new addon.QuicTransportServer(port);
    this._connections = new Map(); // Key is user ID.
    this._streams = new Map(); // Key is content session ID.
    this._unAuthenticatedConnections = []; // When it's authenticated, it will be moved to this.connections.
    this._unAssociatedStreams=[];  // No content session ID assgined to them.
    this._server.onconnection = (connection) => {
      this._unAuthenticatedConnections.push(connection);
      connection.onincomingstream = (stream) => {
        this._unAssociatedStreams.push(stream);
        stream.oncontentsessionid = (id) => {
          console.log('New stream for session ' + this._uuidBytesToString(new Uint8Array(id)));
          stream.contentSessionId=this._uuidBytesToString(new Uint8Array(id));
          this.emit('streamadded', stream);
        }
      }
    }
  }

  start() {
    this._server.start();
  }

  _uuidBytesToString(uuidBytes) {
    if (uuidBytes.length != 16) {
      log.error('Invalid UUID.');
      return;
    }
    let s = '';
    for (let hex of uuidBytes) {
      const str=hex.toString(16);
      s += str.padStart(2, '0');
    }
    return s;
  };
};