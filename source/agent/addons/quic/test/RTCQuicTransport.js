/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

'use strict';

const expect = require('chai').use(require('chai-as-promised')).expect;
const addon = require('../build/Debug/quic');
const http = require('http');
const util = require('util');
const serverHostname = 'jianjunz-nuc-ubuntu.sh.intel.com';

describe('Test RTCQuicTransport.', () => {
  let iceTransport;
  let quicTransport;

  beforeEach(() => {
    iceTransport = new addon.RTCIceTransport();
    quicTransport = new addon.RTCQuicTransport(iceTransport);
  });

  afterEach(() => {
    iceTransport.stop();
  });

  xit('Create an RTCQuicTransport.', (done) => {
    expect(quicTransport).to.be.an.instanceof(addon.RTCQuicTransport);
    done();
  });

  xit(
    'Create an RTCQuicTransport without RTCIceTransport should throw error.',
    (done) => {
      expect(function () { new addon.RTCQuicTransport() }).to.throw(TypeError);
      done();
    });

  xit(
    'getCertificates returns exactly the same certificates provided in ctor.',
    () => {
      const certificates = [addon.RTCCertificate.generateCertificate(),
        addon.RTCCertificate.generateCertificate()
      ];
      console.log('Created certificates.');
      let quicTransport1 = new addon.RTCQuicTransport(iceTransport,
        certificates);
      console.log(quicTransport1.getLocalParameters());
      expect(quicTransport1.getCertificates()).to.equal(certificates);
      quicTransport1 = new addon.RTCQuicTransport(iceTransport,
        certificates.slice(0, 1));
      expect(quicTransport1.getCertificates()).to.equal(certificates.slice(
        0, 1));
    });

  xit('getLocalParameters returns local parameters.', () => {
    const localParameters = quicTransport.getLocalParameters();
    expect(localParameters).to.have.property('role');
    //expect(localParameters).to.have.property('fingerprints');
    //expect(localParameters.fingerprints).to.be.an('array');
  });

  xit('Start an RTCQuicTransport.', (done) => {
    const remoteParameters = {
      role: 'client',
      fingerprints: ['fingerprint']
    };
    quicTransport.start(remoteParameters);
    setTimeout(()=>{done();},1000);
  });

  // This case is intended to be tested with https://github.com/jianjunz/owt-client-javascript/blob/quicsample/src/samples/conference/public/quic.html.
  it('Create an RTCQuicTransport and listen.', (done)=>{
    const candidates=[];
    quicTransport.onbidirectionalstream=(stream)=>{
      console.log('onbidirectionalstream '+stream);
      stream.ondata=(data)=>{
        console.log('OnData: '+data[0]+' '+data[1]);
        stream.write({data:new Uint8Array([42])});
      };
    };
    iceTransport.gather();
    iceTransport.onicecandidate=((candidate)=>{
      console.log('Can: '+JSON.stringify(candidate));
      const requestOptions = {
        hostname: serverHostname,
        port: 7081,
        path: '/rest/server',
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json'
        }
      };
      candidates.push(candidate.candidate.candidate);
      const request=http.request(requestOptions,res=>{
      });
      const content=JSON.stringify({iceParameters:localParameters,iceCandidates:candidates});
      console.log('Write '+content);
      request.write(content);
      request.end();
    });
    const localParameters = iceTransport.getLocalParameters();
    const requestOptions = {
      hostname: serverHostname,
      port: 7081,
      path: '/rest/server',
      method: 'PUT',
      headers: {
        'Content-Type': 'application/json'
      }
    };
    const request=http.request(requestOptions,res=>{
    });
    request.write(JSON.stringify({iceParameters:localParameters}));
    request.end();
    console.log('Upload local parameters.');
    http.get({
          headers: {
            'Cache-Control': 'no-cache'
          },
          hostname: serverHostname,
          port: 7081,
          path: '/rest/client',
          method: 'GET',
        }, (resp) => {
          console.log('onresp');
          let data = '';
          resp.on('data', (chunk) => {
            console.log('Get data.');
            data += chunk;
          });
      resp.on('end',()=>{
        const iceInfo = JSON.parse(data);
        const encoder = new util.TextEncoder();
        const keyBinary = Buffer.from(iceInfo.quicKey);
        console.log('Start ICE transport.');
        iceTransport.start({
          usernameFragment: iceInfo.parameters.usernameFragment,
          password: iceInfo.parameters.password
        });
        for (const iceCandidate of iceInfo.candidates) {
          if (!iceCandidate) {
            continue;
          }
          const candidate = new addon.RTCIceCandidate({
            sdpMid: 0,
            candidate: iceCandidate
          });
          iceTransport.addRemoteCandidate(candidate);
        }
        let keyString='Key:';
        for (let i = 0; i < keyBinary.length; i++) {
          keyString = keyString.concat(' ' + keyBinary[i]);
        }
        console.log(keyString);
        quicTransport.listen(keyBinary);
      });
    });
    console.log('After get.');
    setTimeout(()=>{done();},1000000);
  });
});