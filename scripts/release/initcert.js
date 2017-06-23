#!/usr/bin/env node
'use strict';

(function () {
  var readline = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout
  });
  var cipher = require('./cipher');
  var keystore = require('path').resolve(__dirname, 'cert/.woogeen.keystore');
  readline.question('Enter passphrase of certificate: ', function (res) {
    readline.close();
    cipher.lock(cipher.k, res, keystore, function cb (err) {
      console.log(err||'done!');
    });
  });
})();
