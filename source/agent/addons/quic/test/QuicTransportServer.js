/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

'use strict';
const expect = require('chai').use(require('chai-as-promised')).expect;
const addon = require('../build/Debug/quic');

describe('Test QuicTransportServer.', () => {
  beforeEach(()=>{
    console.log('Before each.');
  })
  afterEach(() => {
    console.log('After each.');
  });

  it('Start QuicTransportServer and wait for connections.', (done) => {
    const server = new addon.QuicTransportServer(7000);
    const connections=[];
    const streams=[];
    server.onconnection=(connection)=>{
      console.log('New connection.');
      connections.push(connection);
      connection.onincomingstream=(stream)=>{
        streams.push(stream);
        stream.oncontentsessionid=(id)=>{
          console.log('New stream for session '+id);
        }
        console.log('New incoming stream.');
      }
    }
    expect(server).to.be.an.instanceof(addon.QuicTransportServer);
    server.start();
    setTimeout(()=>{
      server.stop();
      done();
    },1000000);
  });
});