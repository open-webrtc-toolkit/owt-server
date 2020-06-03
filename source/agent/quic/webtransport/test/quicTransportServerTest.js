/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

'use strict';

const expect = require('chai').use(require('chai-as-promised')).expect;
const QuicTransportServer = require('../quicTransportServer.js');
const sinon = require('sinon');

// Prepare stubs.
const fakeAddonServer = {
  start: sinon.stub(),
  stop: sinon.stub(),
  reset: () => {
    fakeAddonServer.start.resetHistory();
    fakeAddonServer.stop.resetHistory();
  }
};
const fakeAddon = {
  QuicTransportServer: function() {
    return fakeAddonServer;
  }
};

describe('Test QuicTransportServer.', () => {
  const port = 7000;
  const server = new QuicTransportServer(fakeAddon, port);
  beforeEach((done) => {
    server.start();
    done();
  });
  afterEach((done) => {
    // server.stop();
    fakeAddonServer.reset();
    done();
  });

  it('Start server on specified port.', (done) => {
    sinon.assert.calledOnce(fakeAddonServer.start);
    done();
  });

  it('Create send stream with unauthroized transport ID returns undefined.',
     (done) => {
       const transportid1 = 'transportid1';
       const transportid2 = 'transportid2';
       server.addTransportId(transportid1);
       expect(server.createSendStream(transportid2)).to.be.undefined;
       done();
     });

  it('Test UUID convertions.',
     (done) => {
       const uuidString0='';
       const uuidString1='';
       const uuidString2='';
       const uuidArray0='';
       const uuidArray1='';
       const uuidArray2='';
       done();
     });
});