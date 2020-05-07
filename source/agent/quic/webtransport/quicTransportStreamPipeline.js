// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const logger = require('../../logger').logger;
const log = logger.getLogger('QuicTransportStreamPipeline');
const addon = require('../build/Release/quic');

/* `QuicTransportStreamPipeline` is pipeline for a publication or a subscription
 * to send or receive data through a QUIC stream. The pipeline might be created
 * before a QUIC stream is created. In this case, QUIC stream will be attached
 * once it is created and associated with certain publication or subscription.
 *
 * It's intended to a *connection*, so it can be operated by `connections.js`.
 * But there is no definition for *connection*, so it's not inherited from a
 * base class, instead, it has some similar functions as `WrtcStream`,
 * `AVStreamIn`.
 */
module.exports = class QuicTransportStreamPipeline {
  constructor(contentSessionId, updateStatus) {
    // `contentSessionId` is the ID of publication or subscription.
    this._contentSessionId = contentSessionId;
    this._quicStream = null;
    this._notifiedReady = false;

    this.quicStream = function (stream) {
      this._quicStream = stream;
      if (!this._notifiedReady) {
        updateStatus({
          type: 'ready',
          audio: false,
          video: false,
          simulcast: false
        });
      }
    };
  }
};