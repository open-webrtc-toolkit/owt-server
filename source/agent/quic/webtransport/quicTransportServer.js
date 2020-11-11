// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const EventEmitter = require('events');
const logger = require('../../logger').logger;
const log = logger.getLogger('QuicTransportServer');
const zeroUuid = '00000000000000000000000000000000';

/* Every QUIC agent is a QuicTransportServer which accepts QuicTransport
 * connections from clients.
 *
 * Events:
 * ended - Server is ended. All Connections will be stopped.
 * streamadded - A new stream is added, and ready to be attached to a stream
 * pipeline. Argument: a string for publication or subscription ID.
 */

// Connection will be closed if no transport ID is received after
// `authenticationTimeout` seconds.
const authenticationTimeout = 10;

module.exports = class QuicTransportServer extends EventEmitter {
  constructor(addon, port, pfxPath, password, validateTokenCallback) {
    super();
    this._server = new addon.QuicTransportServer(port, pfxPath, password);
    this._connections = new Map(); // Key is transport ID.
    this._streams = new Map(); // Key is content session ID.
    this._unAuthenticatedConnections = []; // When it's authenticated, it will be moved to this.connections.
    this._unAssociatedStreams = []; // No content session ID assgined to them.
    this._assignedTransportIds = []; // Transport channels assigned to this server.
    this._validateTokenCallback = validateTokenCallback;
    this._server.onconnection = (connection) => {
      this._unAuthenticatedConnections.push(connection);
      setTimeout(() => {
        // Must be authenticated in `authenticationTimeout` seconds.
        if (!connection.transportId) {
          connection.close();
        }
      }, authenticationTimeout * 100000);
      connection.onincomingstream = (stream) => {
        log.info('New incoming stream.');
        stream.oncontentsessionid = (id) => {
          const streamId = this._uuidBytesToString(new Uint8Array(id))
          stream.contentSessionId = streamId;
          stream.transportId = connection.transportId;
          if (streamId === zeroUuid) {
            // Signaling stream. Waiting for transport ID.
            log.info('Zero content session ID.');
          } else if (!connection.transportId) {
            log.error(
                'Stream ' + streamId +
                ' added on unauthroized transport. Close connection.');
          } else {
            log.debug(
                'A new stream ' + streamId + ' is created on transport ' +
                connection.transportId);
            this.emit('streamadded', stream);
          }
        };
        stream.ondata = (data) => {
          if (stream.contentSessionId === zeroUuid) {
            // Please refer
            // https://github.com/open-webrtc-toolkit/owt-server/blob/20e8aad216cc446095f63c409339c34c7ee770ee/doc/design/quic-transport-payload-format.md#signaling-session
            // for the format of data received on this stream.

            // Size of data received, but haven't been read.
            let unreadSize = data.length;
            let nextReadSize = stream.frameSize ? stream.frameSize : 0;
            while (unreadSize != 0) {
              if (nextReadSize == 0) {  // Starts a new frame.
                // 32 bit indicates the length of next frame.
                const lengthSize = 4;
                if (data.length >= lengthSize) {
                  for (let i = 0; i < lengthSize; i++) {
                    nextReadSize += (data[i] << 8 * i);
                  }
                  unreadSize -= lengthSize;
                } else {
                  log.error('Not implemented.');
                  return;
                }
              } else {
                nextReadSize = stream.frameSize - stream.unreadData.length;
              }
              const startIndex = data.length - unreadSize;
              if (unreadSize >= nextReadSize) {
                let frame = data.slice(startIndex, startIndex + nextReadSize);
                if (stream.unreadData) {
                  frame = stream.unreadData + frame;
                  stream.unreadData = null;
                  stream.frameSize = 0;
                }
                const message = frame.toString('utf8');
                unreadSize -= nextReadSize;
                // Currently, Signaling over WebTransport is not supported, so
                // only one message (WebTransport token) will be sent on this
                // stream. We process the message and discard all other
                // messages.
                let token;
                try {
                  token = JSON.parse(Buffer.from(message, 'base64').toString());
                } catch (error) {
                  log.error('Invalid token.');
                  connection.close();
                  return;
                }
                this._validateTokenCallback(token).then(result => {
                  if (result !== 'ok') {
                    log.error('Authentication failed.');
                    connection.close();
                    return;
                  }
                  connection.transportId = token.transportId;
                  stream.transportId = token.transportId;
                  this._connections.set(connection.transportId, connection);
                });
                return;
              } else {  // Store the data.
                stream.unreadData = data.slice(startIndex);
                stream.frameSize = nextReadSize;
              }
            }
            return;
          }
        }
      }
    }
  }

  start() {
    this._server.start();
    // TODO: Return error if server is not started, e.g.: port is not available.
  }

  addTransportId(id) {
    if (!(id in this._assignedTransportIds)) {
      this._assignedTransportIds.push(id);
    }
  }

  // Create a send stream for specfied QuicTransport. If specified QuicTransport
  // doesn't exist, no stream will be created.
  createSendStream(transportId, contentSessionId) {
    log.debug(
        'Create send stream ' + contentSessionId + ' on transport ' +
        transportId);
    if (!this._connections.has(transportId)) {
      // TODO: Waiting for transport to be created, and create stream for it.
      // It's a common case that subscribe request is received by QUIC agent
      // before client creates QuicTransport.
      log.error('No QUIC transport for ' + transportId);
      return;
    }
    const stream = this._connections.get(transportId).createBidirectionalStream();
    const uuidBytes=this._uuidStringToUint8Array(contentSessionId);
    stream.write(uuidBytes, uuidBytes.length);
    this._streams.set(contentSessionId, stream);
    return stream;
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
    return uuidArray;
  }

  _validateToken(tokenString) {
    const token = JSON.parse(Buffer.from(tokenString, 'base64').toString());
    return this._validateTokenCallback(token);
  }
};