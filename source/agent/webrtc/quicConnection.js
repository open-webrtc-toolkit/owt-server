// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*global require*/
'use strict';
const logger = require('../logger').logger;
const log = logger.getLogger('QuicConnection');
const addon = require('./build/Release/webrtc-quic');

module.exports = class QuicConnection {
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
      this._quicStream = stream;
      stream.ondata = data => {
        if (typeof this.ondata === 'function') {
          this.ondata.apply(this, [data]);
        }
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
      case 'quic-p2p-client-parameters':
        log.debug('Setting remote ICE parameters.');
        // Move gather() to constructor could speed up ICE gathering. But candidates may be sent to client side before the ack of publish/subscribe.
        this._iceTransport.gather();
        this._iceTransport.start({
          usernameFragment: message.iceParameters.usernameFragment,
          password: message.iceParameters.password
        });
        this._quicTransport.listen(Buffer.from(message.quicKey));
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

  close(){
    log.error('Not implemented.');
  }

  _sendIceParameters() {
    log.info('Send ICE parameters.');
    this._send({
      type: 'quic-p2p-server-parameters',
      iceParameters: this._iceTransport.getLocalParameters()
    });
  }
};
