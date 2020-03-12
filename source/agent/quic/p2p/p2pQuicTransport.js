// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const logger = require('../logger').logger;
const log = logger.getLogger('P2PQuicConnection');
const addon = require('./build/Release/quic');
const P2PQuicStreams = require('./p2pQuicStream.js');

module.exports = class P2PQuicTransport {
  // `senderCallback` is a signaling sender.
  constructor(id, sendCallback) {
    Object.defineProperty(this, 'id', {
      configurable: false,
      writable: false,
      value: id
    });
    this._iceTransport = new addon.RTCIceTransport();
    this._quicTransport = new addon.RTCQuicTransport(this._iceTransport);
    this._send = sendCallback;
    this._isFirstCandidate = true;
    this._quicStream = null;
    this._p2pQuicStreams = new Map(); // P2PQuicStreams used by agent.

    this._iceTransport.onicecandidate = (event) => {
      if (this._isFirstCandidate) {
        this._sendIceParameters();
        this._isFirstCandidate = false;
      }
      this._send({
        type: 'candidate',
        candidate: event.candidate
      });
    };
    this._quicTransport.onbidirectionalstream = (stream) => {
      // QuicStream doesn't expose its ID as a property to JavaScript layer. We
      // signal it as the first message of a stream.
      log.debug('quicTransport onbidirectionalstream. '+ JSON.stringify(stream));
      stream.ondata = data => {
        if (typeof this.ondata === 'function') {
          this.ondata.apply(this, [data]);
        }
      }
      if (typeof this.onbidirectionalstream === 'function') {
        this.onbidirectionalstream.apply(this, [stream]);
      }
    }
    log.debug('Construct a QuicConnection.');
  };

  onSignalling(message) {
    log.debug('On signaling message to QUIC connection: ' + JSON.stringify(
      message));
    switch (message.type) {
      case 'quic-p2p-parameters':
        log.debug('Setting remote ICE parameters.');
        // Move gather() to constructor could speed up ICE gathering. But candidates may be sent to client side before the ack of publish/subscribe.
        this._iceTransport.gather();
        this._iceTransport.start({
          usernameFragment: message.clientTransportParameters.iceParameters.usernameFragment,
          password: message.clientTransportParameters.iceParameters.password
        });
        this._quicTransport.listen(Buffer.from(message.clientTransportParameters.quicKey));
        this._send({
          type: 'ready',
          audio: false,
          video: false
        });
        break;
      case 'candidate':
        log.debug('Setting remote candidate.');
        this._iceTransport.addRemoteCandidate(new addon.RTCIceCandidate({
          sdpMid: message.candidate.sdpMid,
          sdpMLineIndex: message.candidate.sdpMLineIndex,
          candidate: message.candidate.candidate,
          usernameFragment: message.candidate.usernameFragment
        }));
        break;
      default:
        log.warn('Unrecognized message type: ' + message.type);
        return;
    }
  }

  close() {
    log.error('Not implemented.');
  }

  receiver() {
    return this;
  }

  getOrCreateQuicStream(streamId) {
    if (this._p2pQuicStreams.has(streamId)) {
      return this._p2pQuicStreams.get(streamId);
    }
    log.error('Create a new QUIC stream at server side is not implemented.');
  }

  _sendIceParameters() {
    log.info('Send ICE parameters.');
    this._send({
      type: 'quic-p2p-parameters',
      serverTransportParameters: {
        iceParameters: this._iceTransport.getLocalParameters()
      }
    });
  }
};