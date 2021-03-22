#!/usr/bin/env node
// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

(function() {
  const readline = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout,
  });
  const cipher = require('./cipher');
  const dirName =
    !process.pkg ? __dirname : require('path').dirname(process.execPath);
  const keystore = require('path').resolve(dirName, 'cert/' + cipher.kstore);
  readline.question('Enter passphrase of certificate: ', function(res) {
    readline.close();
    cipher.lock(cipher.k, res, keystore, function cb(err) {
      console.log(err||'done!');
    });
  });
})();
