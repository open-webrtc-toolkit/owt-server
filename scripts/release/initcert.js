#!/usr/bin/env node
// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

(function () {
  var readline = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout
  });
  var cipher = require('./cipher');
  var dirName = !process.pkg ? __dirname : require('path').dirname(process.execPath);
  var keystore = require('path').resolve(dirName, 'cert/.woogeen.keystore');
  readline.question('Enter passphrase of certificate: ', function (res) {
    readline.close();
    cipher.lock(cipher.k, res, keystore, function cb (err) {
      console.log(err||'done!');
    });
  });
})();
