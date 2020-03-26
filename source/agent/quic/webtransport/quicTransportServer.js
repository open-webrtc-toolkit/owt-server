// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const logger = require('../logger').logger;
const log = logger.getLogger('QuicTransportServer');
const addon = require('./build/Debug/quic');

module.exports = class QuicTransportServer {
  constructor(port){
    this.server=new addon.QuicTransportServer(port);
  }

  start(){
    this.server.start();
  }
};