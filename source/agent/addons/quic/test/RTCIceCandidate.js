/*
 * Copyright (C) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

'use strict';

const expect = require('chai').use(require('chai-as-promised')).expect;
const addon = require('../build/Release/quic');

describe('Test RTCIceCandidate.', () => {
  it('Create an RTCIceCandidate.', () => {
    const candidate = new addon.RTCIceCandidate({
      sdpMid: 0,
      candidate: '2634527916 1 udp 2122194687 10.239.158.168 64275 typ host generation 0 ufrag l/WL network-id 2'
    });
    expect(candidate).to.be.an.instanceof(addon.RTCIceCandidate);
  });

  it('Serialize and deserialize candidates.', () => {
    const candidateStrings = [
      'candidate:0 1 UDP 2122252543 172.17.77.241 58123 typ host',
      'candidate:2634527916 1 udp 2122194687 10.239.158.168 64275 typ host generation 0 ufrag l/WL network-id 2',
      'candidate:1853887674 1 udp 1518280447 47.61.61.61 36768 typ srflx raddr 192.168.0.196 rport 36768 generation 0',
      'candidate:750991856 1 udp 25108223 237.30.30.30 58779 typ relay raddr 47.61.61.61 rport 54761 generation 0'
    ]
    for (const can of candidateStrings) {
      const candidate = new addon.RTCIceCandidate({
        sdpMid: 0,
        candidate: can
      });
      if (can.indexOf('ufrag') > 0) {
        // Ignore ufrag and network-id for now.
        expect(candidate.candidate).to.equal(can.substring(0, can.indexOf(
          'ufrag') - 1));
      } else {
        expect(candidate.candidate).to.equal(can);
      }
    }
  });
});
