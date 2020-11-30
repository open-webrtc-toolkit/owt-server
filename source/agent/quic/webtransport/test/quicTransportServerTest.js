/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

'use strict';

const assert = require('chai').use(require('chai-as-promised')).assert;
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
       assert.equal(server.createSendStream(transportid2), undefined);
       done();
     });

  it('UUID convertions.', (done) => {
    const uuidStrings = [
      '00000000000000000000000000000000', 'd8bc24a9360b4320bf42704353d906db',
      'e6fc9b708b96418ab996c50bed964eb2'
    ];
    const uuidArrays = [
      new Uint8Array([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
      new Uint8Array([
        216, 188, 36, 169, 54, 11, 67, 32, 191, 66, 112, 67, 83, 217, 6, 219
      ]),
      new Uint8Array([
        230, 252, 155, 112, 139, 150, 65, 138, 185, 150, 197, 11, 237, 150, 78,
        178
      ])
    ];
    assert.equal(uuidStrings.length, uuidArrays.length);
    for (let i = 0; i < uuidStrings.length; i++) {
      assert.deepEqual(
          uuidArrays[i], server._uuidStringToUint8Array(uuidStrings[i]));
      assert.deepEqual(
          uuidStrings[i], server._uuidBytesToString(uuidArrays[i]));
    }
    done();
  });

  it('Correctly identify transport ID and content session ID.', (done) => {
    const connection1 = {close: sinon.stub()};
    const connection2 = {close: sinon.stub()};
    const stream10 = {};
    const stream20 = {};
    const transportId = '0123456789';
    const unauthenticatedTransportId = '1234567890';
    server.addTransportId(transportId);
    fakeAddonServer.onconnection(connection1);
    connection1.onincomingstream(stream10);
    stream10.oncontentsessionid(
        new Uint8Array([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
    stream10.ondata(Buffer.from(transportId, 'utf8'));
    assert.equal(connection1.transportId, transportId);
    assert.equal(stream10.transportId, transportId);
    fakeAddonServer.onconnection(connection2);
    connection2.onincomingstream(stream20);
    stream20.oncontentsessionid(
        new Uint8Array([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
    stream20.ondata(Buffer.from(unauthenticatedTransportId, 'utf8'));
    sinon.assert.notCalled(connection1.close);
    // connection2 doesn't have an authenticated transport ID. Thus, it should be closed.
    sinon.assert.calledOnce(connection2.close);
    done();
  });
});