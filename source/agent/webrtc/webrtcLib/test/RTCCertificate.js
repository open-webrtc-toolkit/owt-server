/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

'use strict';

const expect = require('chai').use(require('chai-as-promised')).expect;
const addon = require('../build/Debug/webrtc-quic');

describe('Test RTCCertificate.', () => {
  it('Create RTCCertificate.', () => {
    const cert = addon.RTCCertificate.generateCertificate();
    expect(cert).is.an.instanceof(addon.RTCCertificate);
  });

  xit('new operator is not allowed.',()=>{
    expect(new addon.RTCCertificate()).to.throw();
  });
});
