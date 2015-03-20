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
    } else {
      console.error('WARN: unrecogonized component', comps[c]);
    }
  }
  return components;
}(process.env.CERT_COMPS));

if (comps.length > 0) {
  console.log('will install certificates for' + comps.join(' '));
} else {
  console.error('no certificates need to be installed');
  process.exit();
}

cipher.unlock(cipher.k, keystore, function cb (err, obj) {
  if (err) {
    collection = {};
  } else {
    collection = obj;
  }

  function done () {
    readline.close();
    cipher.lock(cipher.k, collection, keystore, function cb (err) {
      console.log(err||'done!');
    });
  }

  function ask(components, end) {
    var component = components.splice(0, 1)[0];
    if (component) {
      readline.question('Enter passphrase of certificate for '+component+': ', function (res) {
        collection[component] = res;
        ask(components, end);
      });
    } else {
      end();
    }
  }

  ask(comps, done);
});

