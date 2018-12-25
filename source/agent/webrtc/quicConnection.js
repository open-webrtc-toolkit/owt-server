/*global require*/
'use strict';
require = require('module')._load('./AgentLoader');
const webrtc = require('./webrtcLib/build/Release/webrtc');
const logger = require('./logger').logger;
const log = logger.getLogger('QuicConnection');

module.exports = class QuicConnection {
  constructor(id, sendCallback) {
    Object.defineProperty(this, 'id', {
      configurable: false,
      writable: false,
      value: id
    });
    this._iceTransport = new webrtc.RTCIceTransport();
    this._send = sendCallback;
    this._isFirstCandidate = true;

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
    log.debug('Construct a QuicConnection.');
  };

  onSignalling(message) {
    log.debug('On signaling message to QUIC connection: ' + JSON.stringify(message));
    switch (message.type) {
      case 'ice-parameters':
        log.debug('Setting remote ICE parameters.');
        // Move gather() to constructor could speed up ICE gathering. But candidates may be sent to client side before the ack of publish/subscribe.
        this._iceTransport.gather();
        this._iceTransport.start({
          usernameFragment: message.parameters.usernameFragment,
          password: message.parameters.password
        });
        break;
      case 'candidate':
        log.debug('Setting remote candidate.');
        this._iceTransport.addRemoteCandidate(new webrtc.RTCIceCandidate({
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

  _sendIceParameters() {
    this._send({
      type: 'ice-parameters',
      parameters: this._iceTransport.getLocalParameters()
    });
  }
};
