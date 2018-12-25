'use strict';

const expect = require('chai').use(require('chai-as-promised')).expect;
const addon = require('../build/Release/webrtc');

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
        done();
      } else {
        calledFirstTime = true;
      }
    }
    iceTransport.gather();
  });

  it('Add remote candidates and start.',(done)=>{
    iceTransport.gather();
    const candidate = new addon.RTCIceCandidate({
      sdpMid: 0,
      candidate: '2634527916 1 udp 2122194687 10.239.158.168 64275 typ host generation 0 ufrag l/WL network-id 2'
    });
    iceTransport.addRemoteCandidate(candidate);
    iceTransport.start({usernameFragment: 'user', password: 'password'});
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