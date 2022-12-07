// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const EventEmitter = require('events');
const logger = require('./logger').logger;
const log = logger.getLogger('QuicTransportServer');
var quicCas = require('./quicCascading/build/Release/quicCascading.node');

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
  constructor(port, cf, kf) {
    super();
    this._server = new quicCas.in(port, cf, kf);
  }

  start() {
    this._server.addEventListener('newstream', async (msg) => {
      var event = JSON.parse(msg);
      this.emit('newstream', event);
    });

    this._server.addEventListener('roomevents', async (msg) => {
      var event = JSON.parse(msg);
      this.emit('roomevents', event);
    });
    // TODO: Return error if server is not started, e.g.: port is not available.
  }

  send(session, stream, data) {
    this._server.send(session, stream, data);
  }
};