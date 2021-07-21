// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const expect = chai.expect;

describe('WebTransport end to end tests.', function() {
  it('Write an array and check data received.', (done) => {
    // Prepare data.
    const dataLength = 10;
    const data = new Uint8Array(dataLength);
    for (let i = 0; i < dataLength; i++) {
      data[i] = Math.floor(Math.random() * 100);
    }

    let readLength = 0;  // Length of data received and checked.
    const checkData = (receivedData) => {
      if (receivedData.byteLength > dataLength - readLength) {
        done('Received too much data.');
      }
      for (let i = 0; i < receivedData.byteLength; i++) {
        chai.assert(receivedData[i], data[readLength + i]);
      }
      readLength += receivedData.byteLength;
      if (readLength == dataLength) {
        done();
      }
    };

    createToken(undefined, 'user', 'presenter', async (token) => {
      const conference = new Owt.Conference.ConferenceClient({
        webTransportConfiguration: {
          serverCertificateFingerprints:
              [{value: __karma__.config.cert_fingerprint, algorithm: 'sha-256'}]
        }
      });
      let resolveSubscribed;
      const subscribed = new Promise((resolve) => {
        resolveSubscribed = resolve;
      });
      conference.addEventListener('streamadded', async (event) => {
        const subscription = await conference.subscribe(event.stream);
        const reader = subscription.stream.readable.getReader();
        resolveSubscribed();
        while (true) {
          const {value, fin} = await reader.read();
          if (fin) {
            break;
          }
          checkData(value);
        }
      });
      await conference.join(token);
      const sendStream = await conference.createSendStream();
      const localStream = new Owt.Base.LocalStream(
          sendStream,
          new Owt.Base.StreamSourceInfo(undefined, undefined, true));
      const publication = await conference.publish(localStream);
      const writer = sendStream.writable.getWriter();
      await writer.ready;
      await subscribed;
      await writer.write(data);
      writer.releaseLock();
    }, 'http://localhost:3001');
  });
});