#!/usr/bin/env node
'use strict';

var HOME = process.env.WOOGEEN_HOME;
if (!HOME) {
  throw 'WOOGEEN_HOME not found';
}

var path = require('path');
var readline = require('readline').createInterface({
  input: process.stdin,
  output: process.stdout
});
var EventEmitter = require('events').EventEmitter;
var ev = new EventEmitter();
var cipher = require('../common/cipher');
var collection;
var keystore = path.join(HOME, 'cert/.woogeen.keystore');
var allComps = ['erizo', 'nuve', 'erizoController', 'sample'];
var comps = (function (comps) {
  var components = [];
  comps = comps.split(',');
  for (var c in comps) {
    if (comps[c] === 'all') {
      return allComps;
    }
    if (allComps.indexOf(comps[c]) !== -1) {
      components.push(comps[c]);
    }
  }
  return components;
}(process.env.CERT_COMPS));

cipher.unlock(cipher.k, keystore, function cb (err, obj) {
  if (err) {
    collection = {};
  } else {
    collection = obj;
  }
  var len = comps.length;
  ev.once('done', function () {
    readline.close();
    cipher.lock(cipher.k, collection, keystore, function cb (err) {
      console.log(err||'done!');
    });
  });
  comps.map(function (component) {
    readline.question('Enter passphrase of certificate for '+component+': ', function (res) {
      collection[component] = res;
      len--;
      if (len <= 0) {
        ev.emit('done');
      }
    });
  });
});

