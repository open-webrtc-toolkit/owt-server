// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const logger = require('../logger').logger;
const log = logger.getLogger('QuicTransportServer');
const addon = require('./build/Debug/quic');

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
    this.server = new addon.QuicTransportServer(port);
  }

  start() {
    this.server.start();
  }
};