// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const logger = require('../../logger').logger;
const log = logger.getLogger('P2PQuicStream');
const addon = require('../build/Release/quic');

module.exports = class P2PQuicStream {
  // `senderCallback` is a signaling sender.
  constructor(id, transport) {
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
    this._source = null;  // Notify |_source| when a outbound stream is created. Assuming mutiplexing is not enabled, so there is only one outbound stream.
    this._destination = null;

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
      log.debug('quicTransport onbidirectionalstream. '+ JSON.stringify(stream));
      this._quicStream = stream;
      if (this._destination) {
        const dest = this._destination;
        this._destination = null;
        this.addDestination(undefined, dest);
      }
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

  write(data) {
    if (this._quicStream) {
      this._quicStream.write({data: data});
    } else {
      log.error('Stream is not created.');
    }
  }

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

  // Assumimg mutiplexing is not enabled.
  // This implemention looks ugly, and it needs to be refined in the future. The main reason is stream may not ready when this method is called.
  addDestination(track, dest) {
    // connections.js calls this API syn
    log.debug('addDestination.');
    // Ignore |track| because this connection is a data channel.
    if (this._quicStream) {
      log.debug('Quic stream add Destination ' + JSON.stringify(dest));
      if(dest._quicStream){
        this._quicStream.addDestination(dest._quicStream);
        log.debug('Added destination.');
      }
    } else {
      this._destination = dest;
      log.debug('this._quicStream is null.');
    }
    dest.onbidirectionalstream = (stream) => {
      log.debug('dest.onbidirectionalstream');
      this.addDestination(undefined, dest);
    };
  }

  addSource(source) {
    this._source = source;
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