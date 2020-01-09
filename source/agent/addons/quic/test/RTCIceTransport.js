/*
 * Copyright (C) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


'use strict';

const expect = require('chai').use(require('chai-as-promised')).expect;
const addon = require('../build/Debug/quic');

describe('Test RTCIceTransport with licode and libnice.', () => {
  let iceTransport;
  beforeEach(() => {
    iceTransport = new addon.RTCIceTransport();
  });
  afterEach(() => {
    iceTransport.stop();
  });
  it('Create an RTCIceTransport.', () => {
    const iceTransport2 = new addon.RTCIceTransport();
    expect(iceTransport).to.be.an.instanceof(addon.RTCIceTransport);
    iceTransport2.stop();
  });

  it('Fire statechange and get local candidates on RTCIceTransport', (done) => {
    iceTransport.onstatechange = () => {
      done();
    }
    iceTransport.gather();
  });

  it('Fire icecandidate on RTCIceTransport', (done) => {
    let calledFirstTime = false;
    iceTransport.onicecandidate = (event) => {
      if (calledFirstTime) {
        console.log(event);
      } else {
        calledFirstTime = true;
        done();
      }
    }
    iceTransport.gather();
    iceTransport.start({usernameFragment: 'qlkT', password: '0/MLO8kD6QgBOgQvzX1l8pmO'});
    const candidate = new addon.RTCIceCandidate({
      sdpMid: 0,
      candidate: '4278134664 0 udp 2122063616 192.168.1.8 62630 typ host generation 0 ufrag qlkT network-id 2'
    });
    iceTransport.addRemoteCandidate(candidate);
  });

  it('Add remote candidates and start.',(done)=>{
    iceTransport.gather();
    iceTransport.start({usernameFragment: 'qlkT', password: '0/MLO8kD6QgBOgQvzX1l8pmO'});
    const candidate = new addon.RTCIceCandidate({
      sdpMid: 0,
      candidate: '4278134664 0 udp 2122063616 192.168.1.8 62630 typ host generation 0 ufrag qlkT network-id 2'
    });
    iceTransport.addRemoteCandidate(candidate);
    done();
  });

  it('Get local parameters.',(done)=>{
    iceTransport.gather();
    const parameters=iceTransport.getLocalParameters();
    expect(parameters.usernameFragment).to.be.a('string').that.is.not.empty;
    expect(parameters.password).to.be.a('string').that.is.not.empty;
    done();
  });
});