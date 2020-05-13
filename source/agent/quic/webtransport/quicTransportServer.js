// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const EventEmitter = require('events');
const logger = require('../../logger').logger;
const log = logger.getLogger('QuicTransportServer');
const addon = require('../build/Release/quic');
const zeroUuid = '00000000000000000000000000000000';

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
    this._unAssociatedStreams = []; // No content session ID assgined to them.
    this._server.onconnection = (connection) => {
      this._unAuthenticatedConnections.push(connection);
      connection.onincomingstream = (stream) => {
        stream.oncontentsessionid = (id) => {
          const streamId = this._uuidBytesToString(new Uint8Array(id))
          stream.contentSessionId = streamId;
          if (streamId === zeroUuid) {
            // Signaling stream. Connect to portal in the future. For now, we use it for authentication.
          } else {
            this.emit('streamadded', stream);
          }
        }
        stream.ondata=(data)=>{
          if (stream.contentSessionId === zeroUuid) {
            connection.userId = data.toString('utf8');
            this._connections.set('id', connection);
            log.info('User ID: ' + connection.userId);
          }
        }
      }
    }
  }

  start() {
    this._server.start();
  }

  createSendStream(userId, contentSessionId) {
    userId = 'id';
    if (!this._connections.has(userId)) {
      log.error('No QUIC transport for '+userId);
      return;
    }
    const stream = this._connections.get(userId).createBidirectionalStream();
    log.info('Stream: ' + JSON.stringify(stream));
    const uuidBytes=this._uuidStringToUint8Array(contentSessionId);
    log.info('Data: '+uuidBytes);
    stream.write(uuidBytes, uuidBytes.length);
    this._streams.set(contentSessionId, stream);
    return stream;
  }

  _authenticate(){
    // TODO: Send token to nuve for authentication.
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

  _uuidStringToUint8Array(uuidString) {
    if (uuidString.length != 32) {
      log.error('Invalid UUID.');
      return;
    }
    const uuidArray = new Uint8Array(16);
    for (let i = 0; i < 16; i++) {
      uuidArray[i] = parseInt(uuidString.substring(i * 2, i * 2 + 2), 16);
    }
    console.log(uuidArray);
    return uuidArray;
  }
};